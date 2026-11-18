#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace tsl {
struct Level {
    int rank = 0;
    std::string name;
    std::string creator;
};

// Extracts level cards from the publicly rendered TSL v3 list HTML.
// This is deliberately isolated from GD/Geode so it can be regression tested.
std::vector<Level> parseListHTML(std::string_view html);
std::vector<Level> parseListText(std::string_view text);
std::string lowercase(std::string str);
}
