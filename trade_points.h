#pragma once
#include <string>
#include <vector>
#include <map> // Добавлено для map в get_all_trade_point_codes_with_addresses

struct TradePoint
{
    std::string code;
    std::string name;
    std::string address;
};

void load_trade_points(const std::string &path = "trade_points.json");
bool get_address_by_code(const std::string &code, std::string &address);
std::vector<std::string> get_all_trade_point_codes(); // Возвращает только коды
std::vector<TradePoint> get_all_trade_points();       // Возвращает полные данные