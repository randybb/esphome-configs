#pragma once
// Upcoming calendar events for mcu-m5paper: parsed from the calendar.get_events
// response and kept in flash (NVS), so they survive the power-off between RTC wake-ups.

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "esphome/components/json/json_util.h"
#include "esphome/core/preferences.h"
#include "esphome/core/time.h"

namespace m5paper_calendar {

static constexpr uint8_t MAX_EVENTS = 32;
static constexpr size_t SUMMARY_LEN = 44;
static constexpr uint16_t ALL_DAY = 0xFFFF;
static constexpr uint32_t PREF_HASH = 0x6D35C41A;

enum Kind : uint8_t { BIRTHDAY = 0, WASTE };

struct Event {
  uint16_t day;     // local date, days since 1970-01-01
  uint16_t minute;  // local start time in minutes after midnight, ALL_DAY if none
  uint8_t kind;
  char summary[SUMMARY_LEN + 1];
};

struct Events {
  uint32_t fetched_at{0};  // UTC epoch of the download
  uint8_t count{0};
  Event items[MAX_EVENTS]{};

  bool valid() const { return this->fetched_at != 0; }
};

// the events in RAM, loaded from flash at boot
inline Events &events() {
  static Events ev;
  return ev;
}

// days since 1970-01-01 for a proleptic Gregorian date (H. Hinnant's algorithm)
inline int32_t days_from_civil(int y, int m, int d) {
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const int yoe = y - era * 400;
  const int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}

inline int32_t local_day(const esphome::ESPTime &t) { return days_from_civil(t.year, t.month, t.day_of_month); }

// 0 = Monday
inline int weekday(int32_t day) { return (day + 3) % 7; }

// "2026-10-01" (all-day) or "2026-10-01T10:00:00+02:00" -> local day and minute; false on error
inline bool parse_start(const char *s, uint16_t &day, uint16_t &minute) {
  int y, mo, d, h, mi, sec = 0, oh = 0, om = 0;
  char sign = '+';
  int n = sscanf(s, "%d-%d-%dT%d:%d:%d%c%d:%d", &y, &mo, &d, &h, &mi, &sec, &sign, &oh, &om);
  if (n < 3)
    return false;
  if (n < 5) {
    day = days_from_civil(y, mo, d);
    minute = ALL_DAY;
    return true;
  }
  int64_t ts = int64_t(days_from_civil(y, mo, d)) * 86400 + h * 3600 + mi * 60 + sec;
  if (n >= 8 && (sign == '+' || sign == '-')) {
    int64_t offset = oh * 3600 + om * 60;
    ts += sign == '+' ? -offset : offset;
  }
  if (ts <= 0)
    return false;
  const auto t = esphome::ESPTime::from_epoch_local(ts);
  day = local_day(t);
  minute = t.hour * 60 + t.minute;
  return true;
}

// copy at most SUMMARY_LEN bytes without cutting a UTF-8 sequence
inline void copy_summary(char *dst, const char *src) {
  size_t n = strnlen(src, SUMMARY_LEN + 1);
  if (n > SUMMARY_LEN) {
    n = SUMMARY_LEN;
    while (n > 0 && (uint8_t(src[n]) & 0xC0) == 0x80)
      n--;
  }
  memcpy(dst, src, n);
  dst[n] = '\0';
}

// Parse the calendar.get_events response; Home Assistant wraps the action response as
// {"response": {"calendar.x": {"events": [...]}, ...}}; every calendar other than the waste one holds birthdays
inline bool parse(ArduinoJson::JsonObjectConst response, const char *waste, uint32_t now, Events &ev) {
  ArduinoJson::JsonObjectConst root = response["response"].is<ArduinoJson::JsonObjectConst>()
                                          ? response["response"].as<ArduinoJson::JsonObjectConst>()
                                          : response;
  if (root.size() == 0)
    return false;

  Events out;
  for (ArduinoJson::JsonPairConst cal : root) {
    const uint8_t kind = strcmp(cal.key().c_str(), waste) == 0 ? WASTE : BIRTHDAY;
    for (ArduinoJson::JsonObjectConst item : cal.value()["events"].as<ArduinoJson::JsonArrayConst>()) {
      Event e;
      if (!parse_start(item["start"] | "", e.day, e.minute))
        continue;
      e.kind = kind;
      copy_summary(e.summary, item["summary"] | "");
      // the same birthday in the contacts and the manual calendar
      if (std::any_of(out.items, out.items + out.count,
                      [&e](const Event &o) { return o.day == e.day && strcmp(o.summary, e.summary) == 0; }))
        continue;
      if (out.count < MAX_EVENTS) {
        out.items[out.count++] = e;
        continue;
      }
      // full: keep the nearest events
      Event *last = std::max_element(out.items, out.items + out.count,
                                     [](const Event &a, const Event &b) { return a.day < b.day; });
      if (e.day < last->day)
        *last = e;
    }
  }
  std::sort(out.items, out.items + out.count, [](const Event &a, const Event &b) {
    // all-day (0xFFFF wraps to 0) first
    return a.day != b.day ? a.day < b.day : uint16_t(a.minute + 1) < uint16_t(b.minute + 1);
  });
  out.fetched_at = now;
  ev = out;
  return true;
}

// battery cycle: refreshed once a day, after midnight
inline bool stale(const esphome::ESPTime &now) {
  const auto &ev = events();
  return !ev.valid() || !now.is_valid() || now.timestamp - ev.fetched_at >= 20 * 3600;
}

inline esphome::ESPPreferenceObject &pref() {
  static esphome::ESPPreferenceObject p = esphome::global_preferences->make_preference<Events>(PREF_HASH, true);
  return p;
}

inline bool load(Events &ev) {
  if (!pref().load(&ev)) {
    ev = Events{};
    return false;
  }
  ev.count = std::min(ev.count, MAX_EVENTS);
  return true;
}

// written to flash right away: the board may power off before the next preferences flush
inline bool save(const Events &ev) { return pref().save(&ev) && esphome::global_preferences->sync(); }

}  // namespace m5paper_calendar
