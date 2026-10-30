#pragma once
#include <cstdlib>
#include <cstring>
#include <string>

// Home Assistant weather condition -> MDI icon (glyphs listed in the font: config)
inline const char *weather_icon(const std::string &c) {
  static const char *const MAP[][2] = {
      {"clear-night", "\U000F0594"}, {"sunny", "\U000F0599"},       {"partlycloudy", "\U000F0595"},
      {"cloudy", "\U000F0590"},      {"fog", "\U000F0591"},         {"rainy", "\U000F0597"},
      {"pouring", "\U000F0596"},     {"snowy", "\U000F0598"},       {"snowy-rainy", "\U000F067F"},
      {"hail", "\U000F0592"},        {"lightning", "\U000F0593"},   {"lightning-rainy", "\U000F067E"},
      {"windy", "\U000F059D"},       {"windy-variant", "\U000F059E"}, {"exceptional", "\U000F05D6"}};
  for (auto &m : MAP)
    if (c == m[0]) return m[1];
  return "";
}

// One day of the HA `forecast` attribute as sent to ESPHome (Python repr or JSON of a list of dicts).
struct ForecastDay {
  std::string condition;
  float high, low;
};

// Value after `key` in s, starting at pos; works for 'key': and "key":
static inline const char *forecast_value_(const std::string &s, size_t pos, const char *key, size_t end) {
  size_t k = s.find(key, pos);
  if (k == std::string::npos || k >= end) return nullptr;
  k = s.find(':', k + std::strlen(key));
  if (k == std::string::npos || k >= end) return nullptr;
  while (++k < end && (s[k] == ' ' || s[k] == '\'' || s[k] == '"')) {
  }
  return s.c_str() + k;
}

// Fills up to n days starting at index `first`; returns how many were parsed.
inline int parse_forecast(const std::string &s, int first, ForecastDay *out, int n) {
  int index = 0, got = 0;
  size_t pos = 0;
  while (got < n && (pos = s.find('{', pos)) != std::string::npos) {
    size_t end = s.find('}', pos);
    if (end == std::string::npos) break;
    if (index++ >= first) {
      const char *cond = forecast_value_(s, pos, "condition", end);
      const char *high = forecast_value_(s, pos, "temperature", end);
      const char *low = forecast_value_(s, pos, "templow", end);
      if (cond && high && low) {
        out[got].condition.assign(cond, std::strcspn(cond, "'\","));
        out[got].high = std::strtof(high, nullptr);
        out[got].low = std::strtof(low, nullptr);
        ++got;
      }
    }
    pos = end;
  }
  return got;
}
