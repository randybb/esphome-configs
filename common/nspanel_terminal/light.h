#pragma once
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>

// HA brightness 0..255 -> slider percent; NaN (light off) -> 0
inline int brightness_pct(float b) { return std::isnan(b) ? 0 : int(std::lround(b * 100 / 255)); }

// Hue from the hs_color attribute, "(30.0, 80.0)" or "[30.0, 80.0]"; -1 if missing
inline int parse_hue(const std::string &s) {
  size_t p = s.find_first_of("([");
  if (p == std::string::npos) return -1;
  char *end;
  float h = std::strtof(s.c_str() + p + 1, &end);
  return end == s.c_str() + p + 1 ? -1 : int(std::lround(h));
}
