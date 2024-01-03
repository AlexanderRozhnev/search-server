#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <set>
#include <stdexcept>

bool IsValidWord(std::string_view word);

std::vector<std::string_view> SplitIntoWords(std::string_view text);
std::vector<std::string_view> SplitIntoWords(const std::string& text);

template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    
    std::set<std::string, std::less<>> non_empty_strings;
    
    for (auto str : strings) {
        if (!IsValidWord(str)) {
            throw std::invalid_argument("The presence of invalid characters (with codes from 0 to 31)");
        }
        if (!str.empty()) {
            non_empty_strings.emplace(str);
        }
    }
    return non_empty_strings;
}