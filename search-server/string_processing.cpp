#include <algorithm>

#include "string_processing.h"

using namespace std;

bool IsValidWord(const string_view word) {
    return none_of(word.begin(), word.end(),
                   [](char c) {return c >= '\0' && c < ' ';});  // A valid word must not contain special characters
}

vector<string_view> SplitIntoWords(string_view text) {
    vector<string_view> result;
    
    size_t space = 0;
    for (;;) {
        size_t not_space = text.find_first_not_of(' ', space);
        if (not_space == text.npos) break;
        text.remove_prefix(not_space);
        space = text.find(' ');
        if (space == text.npos) {
            result.push_back(text.substr(0));
            break;
        }
        result.push_back(text.substr(0, space));
    }

    return result;
}

vector<string_view> SplitIntoWords(const string& text) {
    return SplitIntoWords(static_cast<string_view>(text));
}