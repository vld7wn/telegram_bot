#include "http_server.h"
#include "database.h"
#include "config.h"
#include "logger.h"
#include "tariff_manager.h"
#include "trade_points.h"
#include "user_data_types.h"
#include "application_status.h"

#include <tgbot/tgbot.h>
#include <nlohmann/json.hpp>

// cpp-httplib is header-only
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"

using json = nlohmann::json;

// Global server instance
static HttpServer *g_http_server = nullptr;
static httplib::Server *g_svr = nullptr;

HttpServer *getHttpServer()
{
    return g_http_server;
}

void initHttpServer(TgBot::Bot &bot)
{
    if (!g_http_server)
    {
        g_http_server = new HttpServer(bot);
    }
}

HttpServer::HttpServer(TgBot::Bot &bot) : bot_(bot), port_(8080)
{
    g_http_server = this;
}

HttpServer::~HttpServer()
{
    stop();
    if (g_http_server == this)
    {
        g_http_server = nullptr;
    }
}

bool HttpServer::isRunning() const
{
    return running_;
}

void HttpServer::stop()
{
    if (running_ && g_svr)
    {
        running_ = false;
        g_svr->stop();
        if (server_thread_.joinable())
        {
            server_thread_.join();
        }
    }
}

void HttpServer::start(int port)
{
    if (running_)
    {
        LOG(LogLevel::L_WARNING, "HTTP server is already running");
        return;
    }

    port_ = port;
    running_ = true;

    server_thread_ = std::thread([this]()
                                 { serverLoop(); });

    LOG(LogLevel::INFO, "HTTP API server started on port " + std::to_string(port_));
}

void HttpServer::serverLoop()
{
    g_svr = new httplib::Server();

    // Enable CORS for all origins (for development)
    g_svr->set_default_headers({{"Access-Control-Allow-Origin", "*"},
                                {"Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS"},
                                {"Access-Control-Allow-Headers", "Content-Type, Authorization"}});

    // Handle preflight OPTIONS requests
    g_svr->Options("/(.*)", [](const httplib::Request &, httplib::Response &res)
                   {
                       res.set_header("Access-Control-Allow-Origin", "*");
                       res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
                       res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
                       res.status = 204;
                   });

    setupRoutes();

    g_svr->listen("0.0.0.0", port_);

    delete g_svr;
    g_svr = nullptr;
}

void HttpServer::setupRoutes()
{
    // ========== HEALTH CHECK ==========
    g_svr->Get("/api/health", [](const httplib::Request &, httplib::Response &res)
               {
                   json response = {{"status", "ok"}, {"timestamp", time(nullptr)}};
                   res.set_content(response.dump(), "application/json");
               });

    // ========== BOT STATUS ==========
    g_svr->Get("/api/status", [](const httplib::Request &, httplib::Response &res)
               {
                   bool active = db_get_bot_status();
                   json response = {{"active", active}};
                   res.set_content(response.dump(), "application/json");
               });

    g_svr->Post("/api/status", [](const httplib::Request &req, httplib::Response &res)
                {
                    try
                    {
                        auto body = json::parse(req.body);
                        bool active = body.value("active", true);
                        db_set_bot_status(active);
                        json response = {{"success", true}, {"active", active}};
                        res.set_content(response.dump(), "application/json");
                    }
                    catch (const std::exception &e)
                    {
                        res.status = 400;
                        json response = {{"error", e.what()}};
                        res.set_content(response.dump(), "application/json");
                    }
                });

    // ========== APPLICATIONS ==========
    g_svr->Get("/api/applications", [](const httplib::Request &, httplib::Response &res)
               {
                   LOG(LogLevel::INFO, "API: /api/applications called");
                   auto applications = db_get_all_applications();
                   LOG(LogLevel::INFO, "API: db_get_all_applications returned " << applications.size() << " items");
                   json apps = json::array();

                   for (const auto &app : applications)
                   {
                       apps.push_back({{"id", app.id},
                                       {"userId", app.user_id},
                                       {"name", app.name},
                                       {"phone", app.phone},
                                       {"email", app.email},
                                       {"tariff", app.tariff},
                                       {"address", app.address},
                                       {"status", app.chat_status},
                                       {"date", app.timestamp},
                                       {"price", app.price}});
                   }

                   res.set_content(apps.dump(), "application/json");
               });

    g_svr->Get(R"(/api/applications/(\d+))", [](const httplib::Request &req, httplib::Response &res)
               {
                   try
                   {
                       int64_t app_id = std::stoll(req.matches[1]);
                       auto app_opt = db_get_application_by_id(app_id);

                       if (app_opt)
                       {
                           const auto &app = *app_opt;
                           json response = {
                               {"id", app.id},
                               {"userId", app.user_id},
                               {"name", app.name},
                               {"phone", app.phone},
                               {"email", app.email},
                               {"tariff", app.tariff},
                               {"address", app.address},
                               {"status", app.chat_status},
                               {"date", app.timestamp},
                               {"price", app.price}};
                           res.set_content(response.dump(), "application/json");
                       }
                       else
                       {
                           res.status = 404;
                           json response = {{"error", "Application not found"}};
                           res.set_content(response.dump(), "application/json");
                       }
                   }
                   catch (const std::exception &e)
                   {
                       res.status = 400;
                       json response = {{"error", e.what()}};
                       res.set_content(response.dump(), "application/json");
                   }
               });

    g_svr->Patch(R"(/api/applications/(\d+)/status)", [](const httplib::Request &req, httplib::Response &res)
                 {
                     try
                     {
                         int64_t app_id = std::stoll(req.matches[1]);
                         auto body = json::parse(req.body);
                         std::string status = body.value("status", "");

                         ApplicationStatus app_status;
                         if (status == "Новая")
                             app_status = ApplicationStatus::New;
                         else if (status == "В работе")
                             app_status = ApplicationStatus::InProgress;
                         else if (status == "Выполнена")
                             app_status = ApplicationStatus::Done;
                         else if (status == "Отменена")
                             app_status = ApplicationStatus::Cancelled;
                         else
                         {
                             res.status = 400;
                             json response = {{"error", "Invalid status"}};
                             res.set_content(response.dump(), "application/json");
                             return;
                         }

                         db_update_application_status(app_id, app_status);
                         json response = {{"success", true}, {"id", app_id}, {"status", status}};
                         res.set_content(response.dump(), "application/json");
                     }
                     catch (const std::exception &e)
                     {
                         res.status = 400;
                         json response = {{"error", e.what()}};
                         res.set_content(response.dump(), "application/json");
                     }
                 });

    // ========== ADMINS ==========
    g_svr->Get("/api/admins", [](const httplib::Request &, httplib::Response &res)
               {
                   auto admins = db_get_all_admins();
                   auto pending = db_get_pending_admin_requests();

                   json admins_json = json::array();
                   for (const auto &admin : admins)
                   {
                       admins_json.push_back({{"userId", admin.user_id},
                                              {"name", admin.name},
                                              {"tradePoint", admin.trade_point},
                                              {"status", "active"}});
                   }

                   json pending_json = json::array();
                   for (const auto &req : pending)
                   {
                       pending_json.push_back({{"userId", req.user_id},
                                               {"name", req.name},
                                               {"tradePoint", req.trade_point}});
                   }

                   json response = {{"admins", admins_json}, {"pending", pending_json}};
                   res.set_content(response.dump(), "application/json");
               });

    g_svr->Post("/api/admins", [](const httplib::Request &req, httplib::Response &res)
                {
                    try
                    {
                        auto body = json::parse(req.body);
                        int64_t user_id = body.value("userId", (int64_t)0);
                        std::string name = body.value("name", "");
                        std::string trade_point = body.value("tradePoint", "");

                        if (user_id == 0 || name.empty() || trade_point.empty())
                        {
                            res.status = 400;
                            json response = {{"error", "Missing required fields"}};
                            res.set_content(response.dump(), "application/json");
                            return;
                        }

                        db_add_admin_manual(user_id, name, trade_point);
                        json response = {{"success", true}, {"userId", user_id}};
                        res.set_content(response.dump(), "application/json");
                    }
                    catch (const std::exception &e)
                    {
                        res.status = 400;
                        json response = {{"error", e.what()}};
                        res.set_content(response.dump(), "application/json");
                    }
                });

    g_svr->Delete(R"(/api/admins/(\d+))", [](const httplib::Request &req, httplib::Response &res)
                  {
                      try
                      {
                          int64_t user_id = std::stoll(req.matches[1]);
                          db_delete_admin(user_id);
                          json response = {{"success", true}, {"userId", user_id}};
                          res.set_content(response.dump(), "application/json");
                      }
                      catch (const std::exception &e)
                      {
                          res.status = 400;
                          json response = {{"error", e.what()}};
                          res.set_content(response.dump(), "application/json");
                      }
                  });

    g_svr->Post(R"(/api/admins/(\d+)/approve)", [](const httplib::Request &req, httplib::Response &res)
                {
                    try
                    {
                        int64_t user_id = std::stoll(req.matches[1]);
                        db_approve_admin(user_id);
                        json response = {{"success", true}, {"userId", user_id}};
                        res.set_content(response.dump(), "application/json");
                    }
                    catch (const std::exception &e)
                    {
                        res.status = 400;
                        json response = {{"error", e.what()}};
                        res.set_content(response.dump(), "application/json");
                    }
                });

    g_svr->Post(R"(/api/admins/(\d+)/decline)", [](const httplib::Request &req, httplib::Response &res)
                {
                    try
                    {
                        int64_t user_id = std::stoll(req.matches[1]);
                        db_decline_admin_request(user_id);
                        json response = {{"success", true}, {"userId", user_id}};
                        res.set_content(response.dump(), "application/json");
                    }
                    catch (const std::exception &e)
                    {
                        res.status = 400;
                        json response = {{"error", e.what()}};
                        res.set_content(response.dump(), "application/json");
                    }
                });

    // ========== TRADE POINTS ==========
    g_svr->Get("/api/trade-points", [](const httplib::Request &, httplib::Response &res)
               {
                   auto points = get_all_trade_points();
                   json result = json::array();

                   for (const auto &point : points)
                   {
                       result.push_back({{"code", point.code},
                                         {"name", point.name},
                                         {"address", point.address}});
                   }

                   res.set_content(result.dump(), "application/json");
               });

    // ========== TARIFFS ==========
    g_svr->Get("/api/tariffs", [](const httplib::Request &, httplib::Response &res)
               {
                   // Используем глобальный вектор tariff_plans из tariff_manager.h
                   auto &tariffs = tariff_plans;
                   json result = json::array();

                   for (const auto &tariff : tariffs)
                   {
                       json speeds = json::array();
                       for (const auto &speed_opt : tariff.speeds)
                       {
                           speeds.push_back({{"speed", speed_opt.value + " " + speed_opt.unit},
                                             {"price", speed_opt.price}});
                       }

                       // Addons не поддерживаются в текущей структуре TariffPlan
                       json addons = json::array();

                       result.push_back({{"id", tariff.id},
                                         {"name", tariff.name},
                                         {"speeds", speeds},
                                         {"addons", addons},
                                         {"connectionFee", tariff.connection_fee},
                                         {"routerRental", tariff.router_rental}});
                   }

                   res.set_content(result.dump(), "application/json");
               });

    // ========== BROADCAST ==========
    g_svr->Post("/api/broadcast", [this](const httplib::Request &req, httplib::Response &res)
                {
                    try
                    {
                        auto body = json::parse(req.body);
                        std::string message = body.value("message", "");

                        if (message.empty())
                        {
                            res.status = 400;
                            json response = {{"error", "Message is required"}};
                            res.set_content(response.dump(), "application/json");
                            return;
                        }

                        // TODO: Implement actual broadcast using bot_
                        // For now, just acknowledge receipt
                        LOG(LogLevel::INFO, "Broadcast requested: " + message);

                        json response = {{"success", true}, {"message", "Broadcast queued"}};
                        res.set_content(response.dump(), "application/json");
                    }
                    catch (const std::exception &e)
                    {
                        res.status = 400;
                        json response = {{"error", e.what()}};
                        res.set_content(response.dump(), "application/json");
                    }
                });

    // ========== STATISTICS ==========
    g_svr->Get("/api/stats", [](const httplib::Request &, httplib::Response &res)
               {
                   auto applications = db_get_all_applications();
                   int newCount = 0, inProgress = 0, completed = 0;

                   for (const auto &app : applications)
                   {
                       if (app.chat_status == "Новая")
                           newCount++;
                       else if (app.chat_status == "В работе")
                           inProgress++;
                       else if (app.chat_status == "Выполнена")
                           completed++;
                   }

                   json response = {
                       {"totalApplications", applications.size()},
                       {"newToday", newCount},
                       {"inProgress", inProgress},
                       {"completed", completed},
                       {"totalAdmins", db_get_all_admins().size()},
                       {"pendingAdmins", db_get_pending_admin_requests().size()}};
                   res.set_content(response.dump(), "application/json");
               });

    LOG(LogLevel::INFO, "HTTP API routes configured");
}
