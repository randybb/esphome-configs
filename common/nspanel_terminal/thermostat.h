#pragma once
#include <string>
#include <vector>

// HA list attribute as sent to ESPHome, "['none', 'away']" or '["none", "away"]' -> items
inline std::vector<std::string> parse_list(const std::string &s) {
  std::vector<std::string> out;
  for (size_t p = 0; (p = s.find_first_of("'\"", p)) != std::string::npos;) {
    size_t end = s.find(s[p], p + 1);
    if (end == std::string::npos) break;
    out.push_back(s.substr(p + 1, end - p - 1));
    p = end + 1;
  }
  return out;
}

// Display name for a climate preset; unknown presets are shown as they come
inline std::string preset_name(const std::string &p) {
  static const char *const NAMES[][2] = {{"none", "Žiadny"},   {"home", "Doma"},       {"away", "Preč"},
                                         {"eco", "Eco"},        {"comfort", "Komfort"}, {"sleep", "Spánok"},
                                         {"activity", "Aktivita"}, {"boost", "Boost"}};
  for (auto &n : NAMES)
    if (p == n[0]) return n[1];
  return p;
}
