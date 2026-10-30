#pragma once
#include <string>

// Home Assistant entity state that means "no usable value": the tile is shown greyed out and disabled
inline bool ha_unavailable(const std::string &s) { return s.empty() || s == "unavailable" || s == "unknown"; }
