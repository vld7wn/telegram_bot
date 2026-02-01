#include "trade_points.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <vector>
#include "logger.h"

static nlohmann::json trade_points_data;

void load_trade_points(const std::string &path)
{
    std::ifstream f(path);
    if (!f.is_open())
    {
        throw std::runtime_error("Could not open trade_points.json file: " + path);
    }

    try
    {
        trade_points_data = nlohmann::json::parse(f);
        LOG(LogLevel::INFO, "Trade points loaded successfully from " << path);
    }
    catch (const nlohmann::json::parse_error &e)
    {
        LOG(LogLevel::L_ERROR, "JSON parse error in " << path << ": " << e.what());
        throw std::runtime_error("Invalid JSON syntax in " + path);
    }
}

bool get_address_by_code(const std::string &code, std::string &address)
{
    for (const auto &point : trade_points_data)
    {
        if (point.at("code").get<std::string>() == code)
        {
            address = point.at("address").get<std::string>();
            return true;
        }
    }
    return false;
}

std::vector<std::string> get_all_trade_point_codes()
{
    std::vector<std::string> codes;
    for (const auto &point : trade_points_data)
    {
        codes.push_back(point.at("code").get<std::string>());
    }
    return codes;
}

std::vector<TradePoint> get_all_trade_points()
{
    std::vector<TradePoint> points;
    for (const auto &point : trade_points_data)
    {
        points.push_back({point.at("code").get<std::string>(),
                          point.value("name", ""),
                          point.at("address").get<std::string>()});
    }
    return points;
}

void save_trade_points(const std::string &path)
{
    std::ofstream f(path);
    if (!f.is_open())
    {
        throw std::runtime_error("Could not open trade_points.json for writing: " + path);
    }
    f << trade_points_data.dump(4);
    LOG(LogLevel::INFO, "Trade points saved successfully to " << path);
}

void update_trade_point(const TradePoint &point)
{
    bool found = false;
    for (auto &tp : trade_points_data)
    {
        if (tp["code"] == point.code)
        {
            tp["name"] = point.name;
            tp["address"] = point.address;
            found = true;
            break;
        }
    }
    if (!found)
    {
        trade_points_data.push_back({{"code", point.code}, {"name", point.name}, {"address", point.address}});
    }
    save_trade_points();
}