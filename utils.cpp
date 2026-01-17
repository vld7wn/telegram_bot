#include "utils.h"
#include <algorithm>
#include <cctype>
#include <regex> // <-- Add this line

bool isNotEmpty(const std::string& text) { return !text.empty(); }

bool isValidPhone(std::string& phone) {
    phone.erase(std::remove_if(phone.begin(), phone.end(), [](char c) { return !std::isdigit(c); }), phone.end());
    if (phone.length() == 11 && (phone[0] == '7' || phone[0] == '8')) { phone = phone.substr(1); }
    return phone.length() == 10;
}

bool isValidEmail(const std::string& email) {
    // Это более простое и совместимое регулярное выражение
    const std::regex pattern(R"(^([\w\.\-]+)@([\w\.\-]+)\.([\w]{2,})$)");
    return std::regex_match(email, pattern);
}