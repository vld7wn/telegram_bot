#include "faq_manager.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include "logger.h"

static std::vector<FAQEntry> faq_data;

void load_faq(const std::string &path)
{
    std::ifstream f(path);
    if (!f.is_open())
    {
        throw std::runtime_error("Could not open faq.json file: " + path);
    }

    try
    {
        nlohmann::json data = nlohmann::json::parse(f);
        for (const auto &item : data)
        {
            faq_data.push_back({item.at("question").get<std::string>(),
                                item.at("answer").get<std::string>()});
        }
        LOG(LogLevel::INFO, "FAQ loaded successfully from " << path << ". Total entries: " << faq_data.size());
    }
    catch (const nlohmann::json::parse_error &e)
    {
        LOG(LogLevel::L_ERROR, "JSON parse error in " << path << ": " << e.what());
        throw std::runtime_error("Invalid JSON syntax in " + path);
    }
    catch (const nlohmann::json::out_of_range &e)
    {
        LOG(LogLevel::L_ERROR, "Missing required field in " << path << ": " << e.what());
        throw std::runtime_error("Missing required field in " + path);
    }
}

const std::vector<FAQEntry> &get_all_faq_entries()
{
    return faq_data;
}