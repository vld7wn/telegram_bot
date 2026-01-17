#pragma once
#include <string>
#include <vector>

struct FAQEntry {
    std::string question;
    std::string answer;
};

void load_faq(const std::string& path = "faq.json");
const std::vector<FAQEntry>& get_all_faq_entries();