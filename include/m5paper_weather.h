#pragma once
// Hourly weather forecast for mcu-m5paper: parsed from the weather.get_forecasts
// response and kept in the on-board FM24C02 EEPROM (256 B, I2C 0x50), so it
// survives the power-off between RTC wake-ups.

#include <cmath>
#include <cstdio>
#include <cstring>

#include "esphome/components/i2c/i2c_bus.h"
#include "esphome/components/json/json_util.h"
#include "esphome/core/hal.h"

namespace m5paper_weather {

static constexpr uint8_t EEPROM_ADDR = 0x50;
static constexpr uint8_t EEPROM_PAGE = 8;
static constexpr uint8_t MAGIC = 0xA5;
static constexpr uint8_t VERSION = 1;
static constexpr uint8_t MAX_HOURS = 48;
// magic, version, fetched_at (u32), first_ts (u32), count
static constexpr size_t HEADER = 11;
static constexpr size_t MAX_SIZE = HEADER + MAX_HOURS * 4;
static_assert(MAX_SIZE <= 256, "forecast does not fit into the EEPROM");

static constexpr uint8_t NO_VALUE = 0xFF;
static constexpr int8_t NO_TEMP = INT8_MIN;

struct Hour {
  uint8_t cond;   // index into CONDITIONS, NO_VALUE if unknown
  int8_t temp2;   // temperature in 0.5 °C, NO_TEMP if unknown
  uint8_t wind;   // km/h, NO_VALUE if unknown
  uint8_t pop;    // precipitation probability %, NO_VALUE if unknown
};
static_assert(sizeof(Hour) == 4, "Hour must stay 4 bytes");

struct Forecast {
  uint32_t fetched_at{0};  // UTC epoch of the download
  uint32_t first_ts{0};    // UTC epoch of hours[0]; entries are 1 h apart
  uint8_t count{0};
  Hour hours[MAX_HOURS]{};

  bool valid() const { return this->count > 0; }
  // hour covering the given UTC epoch, nullptr if outside the forecast
  const Hour *at(uint32_t ts) const {
    if (!this->valid() || ts < this->first_ts)
      return nullptr;
    uint32_t idx = (ts - this->first_ts) / 3600;
    return idx < this->count ? &this->hours[idx] : nullptr;
  }
};

// the forecast in RAM, loaded from the EEPROM at boot
inline Forecast &forecast() {
  static Forecast fc;
  return fc;
}

// Home Assistant weather conditions; the index is the stored condition code
static const char *const CONDITIONS[] = {
    "clear-night", "cloudy", "exceptional", "fog",         "hail",  "lightning",   "lightning-rainy", "partlycloudy",
    "pouring",     "rainy",  "snowy",       "snowy-rainy", "sunny", "windy",       "windy-variant",
};
static const char *const CONDITION_NAMES[] = {
    "Jasno",  "Oblačno", "Extrémne počasie", "Hmla",           "Krupobitie", "Búrka",   "Búrka s dažďom", "Polooblačno",
    "Lejak",  "Dážď",    "Sneženie",         "Dážď so snehom", "Slnečno",    "Veterno", "Veterno, oblačno",
};
static constexpr uint8_t NUM_CONDITIONS = sizeof(CONDITIONS) / sizeof(CONDITIONS[0]);

inline uint8_t condition_code(const char *name) {
  for (uint8_t i = 0; i < NUM_CONDITIONS; i++) {
    if (strcmp(name, CONDITIONS[i]) == 0)
      return i;
  }
  return NO_VALUE;
}

inline const char *condition_name(uint8_t code) { return code < NUM_CONDITIONS ? CONDITION_NAMES[code] : "?"; }

// days since 1970-01-01 for a proleptic Gregorian date (H. Hinnant's algorithm)
inline int32_t days_from_civil(int y, int m, int d) {
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const int yoe = y - era * 400;
  const int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}

// "2026-09-27T16:00:00+00:00" -> UTC epoch, 0 on error
inline uint32_t parse_iso8601(const char *s) {
  int y, mo, d, h, mi, sec = 0, oh = 0, om = 0;
  char sign = '+';
  int n = sscanf(s, "%d-%d-%dT%d:%d:%d%c%d:%d", &y, &mo, &d, &h, &mi, &sec, &sign, &oh, &om);
  if (n < 5)
    return 0;
  int64_t ts = int64_t(days_from_civil(y, mo, d)) * 86400 + h * 3600 + mi * 60 + sec;
  if (n >= 8 && (sign == '+' || sign == '-')) {
    int64_t offset = oh * 3600 + om * 60;
    ts += sign == '+' ? -offset : offset;
  }
  return ts > 0 ? uint32_t(ts) : 0;
}

// Parse the weather.get_forecasts response for the given entity;
// Home Assistant wraps the action response as {"response": {...}}
inline bool parse(ArduinoJson::JsonObjectConst response, const char *entity, uint32_t now, Forecast &fc) {
  ArduinoJson::JsonObjectConst root = response["response"].is<ArduinoJson::JsonObjectConst>()
                                          ? response["response"].as<ArduinoJson::JsonObjectConst>()
                                          : response;
  ArduinoJson::JsonArrayConst items = root[entity]["forecast"];
  if (items.isNull() || items.size() == 0)
    return false;

  Forecast out;
  for (ArduinoJson::JsonObjectConst item : items) {
    if (out.count >= MAX_HOURS)
      break;
    if (out.count == 0) {
      out.first_ts = parse_iso8601(item["datetime"] | "");
      if (out.first_ts == 0)
        return false;
    }
    Hour &hour = out.hours[out.count++];
    hour.cond = condition_code(item["condition"] | "");
    float temp = item["temperature"] | NAN;
    hour.temp2 = std::isnan(temp) ? NO_TEMP : int8_t(std::max(-127L, std::min(127L, lroundf(temp * 2))));
    float wind = item["wind_speed"] | NAN;
    hour.wind = std::isnan(wind) ? NO_VALUE : uint8_t(std::max(0L, std::min(254L, lroundf(wind))));
    float pop = item["precipitation_probability"] | NAN;
    hour.pop = std::isnan(pop) ? NO_VALUE : uint8_t(std::max(0L, std::min(100L, lroundf(pop))));
  }
  out.fetched_at = now;
  fc = out;
  return true;
}

inline bool load(esphome::i2c::I2CBus *bus, Forecast &fc) {
  uint8_t buf[MAX_SIZE];
  const uint8_t start = 0;
  fc.count = 0;
  if (bus->write_readv(EEPROM_ADDR, &start, 1, buf, sizeof(buf)) != esphome::i2c::ERROR_OK)
    return false;
  if (buf[0] != MAGIC || buf[1] != VERSION)
    return false;
  memcpy(&fc.fetched_at, buf + 2, 4);
  memcpy(&fc.first_ts, buf + 6, 4);
  fc.count = std::min(buf[10], MAX_HOURS);
  memcpy(fc.hours, buf + HEADER, fc.count * sizeof(Hour));
  return true;
}

inline bool save(esphome::i2c::I2CBus *bus, const Forecast &fc) {
  uint8_t buf[MAX_SIZE];
  buf[0] = MAGIC;
  buf[1] = VERSION;
  memcpy(buf + 2, &fc.fetched_at, 4);
  memcpy(buf + 6, &fc.first_ts, 4);
  buf[10] = fc.count;
  memcpy(buf + HEADER, fc.hours, fc.count * sizeof(Hour));
  const size_t len = HEADER + fc.count * sizeof(Hour);

  // page writes must not cross an 8-byte page boundary; 5 ms write cycle per page
  for (size_t off = 0; off < len; off += EEPROM_PAGE) {
    uint8_t page[EEPROM_PAGE + 1];
    const size_t n = std::min<size_t>(EEPROM_PAGE, len - off);
    page[0] = uint8_t(off);
    memcpy(page + 1, buf + off, n);
    if (bus->write(EEPROM_ADDR, page, n + 1) != esphome::i2c::ERROR_OK)
      return false;
    esphome::delay(6);
  }
  return true;
}

}  // namespace m5paper_weather
