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
    g_svr->Options("/(.*)", [](const httplib::Request &req, httplib::Response &res)
                   {
                       LOG(LogLevel::INFO, "OPTIONS " + req.path);
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

    g_svr->Patch(R"(/api/applications/(\d+)/status)", [this](const httplib::Request &req, httplib::Response &res)
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

                         // Update in DB
                         db_update_application_status(app_id, app_status);
                         LOG(LogLevel::INFO, "API: Updated status for app " + std::to_string(app_id) + " to " + status);

                         // Send notification to user
                         auto app_opt = db_get_application_by_id(app_id);
                         if (app_opt)
                         {
                             LOG(LogLevel::INFO, "API: Found application " + std::to_string(app_id) + " for user " + std::to_string(app_opt->user_id));
                             std::string message = "🔔 Статус вашей заявки №" + std::to_string(app_id) + " изменен на: *" + status + "*";
                             try
                             {
                                 bot_.getApi().sendMessage(app_opt->user_id, message, nullptr, nullptr, nullptr, "Markdown");
                                 LOG(LogLevel::INFO, "API: Status notification sent to user " + std::to_string(app_opt->user_id));
                             }
                             catch (const std::exception &e)
                             {
                                 LOG(LogLevel::L_ERROR, "API: Failed to send status notification to user " + std::to_string(app_opt->user_id) + ": " + e.what());
                             }
                         }
                         else
                         {
                             LOG(LogLevel::L_ERROR, "API: Application " + std::to_string(app_id) + " not found in DB after update!");
                         }

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

    g_svr->Post("/api/trade-points", [](const httplib::Request &req, httplib::Response &res)
                {
                    try
                    {
                        auto body = json::parse(req.body);
                        TradePoint pt;
                        pt.code = body.value("code", "");
                        if (pt.code.empty())
                            pt.code = body.value("name", "");
                        pt.name = pt.code;
                        pt.address = body.value("address", "");
                        if (pt.code.empty() || pt.address.empty())
                        {
                            res.status = 400;
                            return;
                        }
                        update_trade_point(pt);
                        res.set_content(json({{"success", true}}).dump(), "application/json");
                    }
                    catch (...)
                    {
                        res.status = 400;
                    }
                });

    g_svr->Put(R"(/api/trade-points/(\w+))", [](const httplib::Request &req, httplib::Response &res)
               {
                   try
                   {
                       std::string code = req.matches[1];
                       auto body = json::parse(req.body);

                       TradePoint pt;
                       pt.code = code;
                       pt.name = body.value("name", "");
                       pt.address = body.value("address", "");

                       update_trade_point(pt);

                       json response = {{"success", true}, {"code", code}};
                       res.set_content(response.dump(), "application/json");
                   }
                   catch (const std::exception &e)
                   {
                       res.status = 400;
                       res.set_content(json({{"error", e.what()}}).dump(), "application/json");
                   }
               });

    // ========== TARIFFS ==========
    g_svr->Post("/api/tariffs/discount", [](const httplib::Request &req, httplib::Response &res)
                {
                    try
                    {
                        auto body = json::parse(req.body);
                        double percent = body.value("percent", 0.0);
                        std::vector<std::string> ids = body.value("tariffIds", std::vector<std::string>());

                        if (percent <= 0 || percent > 100 || ids.empty())
                        {
                            res.status = 400;
                            res.set_content(json({{"error", "Invalid parameters"}}).dump(), "application/json");
                            return;
                        }

                        apply_discount_to_tariffs(ids, percent);
                        res.set_content(json({{"success", true}}).dump(), "application/json");
                    }
                    catch (const std::exception &e)
                    {
                        res.status = 400;
                        res.set_content(json({{"error", e.what()}}).dump(), "application/json");
                    }
                });
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

                       result.push_back({{"id", tariff.id},
                                         {"name", tariff.name},
                                         {"speeds", speeds},
                                         {"services", tariff.services},
                                         {"extraDetails", tariff.extra_details},
                                         {"connectionFee", tariff.connection_fee},
                                         {"routerRental", tariff.router_rental},
                                         {"tvBoxRental", tariff.tv_box_rental},
                                         {"mobileInternetGb", tariff.mobile_internet_gb},
                                         {"mobileMinutes", tariff.mobile_minutes},
                                         {"mobileSms", tariff.mobile_sms},
                                         {"mobileIncluded", tariff.mobile_connection_included}});
                   }

                   res.set_content(result.dump(), "application/json");
               });

    g_svr->Put(R"(/api/tariffs/([\w\-]+))", [](const httplib::Request &req, httplib::Response &res)
               {
                   try
                   {
                       std::string id = req.matches[1];
                       auto body = json::parse(req.body);

                       TariffPlan tp = get_tariff_by_id(id);
                       if (tp.id.empty())
                       {
                           tp.id = id;
                       }
                       tp.name = body.value("name", tp.name);
                       tp.connection_fee = body.value("connectionFee", tp.connection_fee);
                       tp.router_rental = body.value("routerRental", tp.router_rental);
                       tp.tv_box_rental = body.value("tvBoxRental", tp.tv_box_rental);
                       tp.extra_details = body.value("extraDetails", tp.extra_details);

                       tp.mobile_connection_included = body.value("mobileIncluded", tp.mobile_connection_included);
                       tp.mobile_internet_gb = body.value("mobileInternetGb", tp.mobile_internet_gb);
                       tp.mobile_minutes = body.value("mobileMinutes", tp.mobile_minutes);
                       tp.mobile_sms = body.value("mobileSms", tp.mobile_sms);

                       if (body.contains("services") && body["services"].is_array())
                       {
                           tp.services = body["services"].get<std::vector<std::string>>();
                       }

                       if (body.contains("speeds") && body["speeds"].is_array())
                       {
                           tp.speeds.clear();
                           for (const auto &s_json : body["speeds"])
                           {
                               TariffSpeedOption so;
                               std::string speed_text = s_json.value("speed", "");
                               size_t space_pos = speed_text.find(' ');
                               if (space_pos != std::string::npos)
                               {
                                   so.value = speed_text.substr(0, space_pos);
                                   so.unit = speed_text.substr(space_pos + 1);
                               }
                               else
                               {
                                   so.value = speed_text;
                                   so.unit = "Мбит/с";
                               }
                               so.price = s_json.value("price", "0");
                               tp.speeds.push_back(so);
                           }
                       }

                       update_tariff_plan(tp);

                       json response = {{"success", true}, {"id", id}};
                       res.set_content(response.dump(), "application/json");
                   }
                   catch (const std::exception &e)
                   {
                       res.status = 400;
                       res.set_content(json({{"error", e.what()}}).dump(), "application/json");
                   }
               });

    g_svr->Post("/api/tariffs", [this](const httplib::Request &req, httplib::Response &res)
                {
                    try
                    {
                        auto body = json::parse(req.body);
                        TariffPlan tp;
                        tp.id = body.value("id", "new-tariff-" + std::to_string(time(0)));
                        tp.name = body.value("name", "Новый тариф");
                        tp.connection_fee = body.value("connectionFee", "0");
                        tp.router_rental = body.value("routerRental", "0");
                        tp.tv_box_rental = body.value("tvBoxRental", "0");
                        tp.extra_details = body.value("extraDetails", "");
                        tp.mobile_connection_included = body.value("mobileIncluded", false);
                        tp.mobile_internet_gb = body.value("mobileInternetGb", "0");
                        tp.mobile_minutes = body.value("mobileMinutes", "0");
                        tp.mobile_sms = body.value("mobileSms", "0");

                        if (body.contains("services") && body["services"].is_array())
                        {
                            tp.services = body["services"].get<std::vector<std::string>>();
                        }

                        if (body.contains("speeds") && body["speeds"].is_array())
                        {
                            for (const auto &s_json : body["speeds"])
                            {
                                TariffSpeedOption so;
                                std::string speed_text = s_json.value("speed", "");
                                size_t space_pos = speed_text.find(' ');
                                if (space_pos != std::string::npos)
                                {
                                    so.value = speed_text.substr(0, space_pos);
                                    so.unit = speed_text.substr(space_pos + 1);
                                }
                                else
                                {
                                    so.value = speed_text;
                                    so.unit = "Мбит/с";
                                }
                                so.price = s_json.value("price", "0");
                                tp.speeds.push_back(so);
                            }
                        }

                        update_tariff_plan(tp);
                        res.set_content(json({{"success", true}, {"id", tp.id}}).dump(), "application/json");
                    }
                    catch (const std::exception &e)
                    {
                        res.status = 400;
                        res.set_content(json({{"error", e.what()}}).dump(), "application/json");
                    }
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

    // ========== DIRECT MESSAGES ==========
    g_svr->Get(R"(/api/messages/(\d+))", [](const httplib::Request &req, httplib::Response &res)
               {
                   try
                   {
                       long long app_id = std::stoll(req.matches[1]);
                       LOG(LogLevel::INFO, "API: Getting chat history for app ID " + std::to_string(app_id));
                       auto history = db_get_chat_history(app_id);
                       LOG(LogLevel::INFO, "API: db_get_chat_history returned " + std::to_string(history.size()) + " messages");
                       json result = json::array();
                       for (const auto &msg : history)
                       {
                           result.push_back({{"sender", msg.sender},
                                             {"text", msg.text},
                                             {"timestamp", msg.timestamp}});
                       }
                       res.set_content(result.dump(), "application/json");
                   }
                   catch (const std::exception &e)
                   {
                       LOG(LogLevel::L_ERROR, "API: Failed to get chat history: " + std::string(e.what()));
                       res.status = 400;
                       res.set_content(json({{"error", e.what()}}).dump(), "application/json");
                   }
               });

    g_svr->Post(R"(/api/messages/(\d+))", [this](const httplib::Request &req, httplib::Response &res)
                {
                    try
                    {
                        long long app_id = std::stoll(req.matches[1]);
                        auto body = json::parse(req.body);
                        int64_t userId = body.value("userId", (int64_t)0);
                        std::string message = body.value("text", "");

                        LOG(LogLevel::INFO, "API: Admin sending message to app ID " + std::to_string(app_id) + " (User " + std::to_string(userId) + ")");

                        if (userId == 0 || message.empty())
                        {
                            res.status = 400;
                            res.set_content(json({{"error", "userId and text are required"}}).dump(), "application/json");
                            return;
                        }

                        // Send via Telegram asynchronously to avoid blocking the API response
                        std::thread([this, userId, message, app_id]()
                                    {
                                        try
                                        {
                                            bot_.getApi().sendMessage(userId, message);
                                            LOG(LogLevel::INFO, "Telegram message sent to user " + std::to_string(userId));
                                        }
                                        catch (const std::exception &e)
                                        {
                                            LOG(LogLevel::L_ERROR, "Failed to send Telegram message to user " + std::to_string(userId) + ": " + e.what());
                                        }
                                    })
                            .detach();

                        // Save to history
                        ChatMessage msg;
                        msg.sender = "admin";
                        msg.text = message;
                        db_add_chat_message(app_id, msg);

                        LOG(LogLevel::INFO, "Admin sent message to user " + std::to_string(userId) + " and saved to chat " + std::to_string(app_id));

                        res.set_content(json({{"success", true}}).dump(), "application/json");
                    }
                    catch (const std::exception &e)
                    {
                        LOG(LogLevel::L_ERROR, "API: Failed to send message: " + std::string(e.what()));
                        res.status = 500;
                        res.set_content(json({{"error", e.what()}}).dump(), "application/json");
                    }
                });

    // Резервный эндпоинт для старого API (без app_id)
    g_svr->Post("/api/messages", [this](const httplib::Request &req, httplib::Response &res)
                {
                    try
                    {
                        auto body = json::parse(req.body);
                        int64_t userId = body.value("userId", (int64_t)0);
                        std::string message = body.value("text", "");

                        if (userId == 0 || message.empty())
                        {
                            res.status = 400;
                            res.set_content(json({{"error", "userId and text are required"}}).dump(), "application/json");
                            return;
                        }

                        bot_.getApi().sendMessage(userId, message);
                        LOG(LogLevel::INFO, "Admin sent legacy message to user " + std::to_string(userId));

                        res.set_content(json({{"success", true}}).dump(), "application/json");
                    }
                    catch (const std::exception &e)
                    {
                        res.status = 500;
                        res.set_content(json({{"error", e.what()}}).dump(), "application/json");
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
