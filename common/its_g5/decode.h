#pragma once
// Header-level decoding of captured ITS-G5 frames: 802.11 + LLC/SNAP, GeoNetworking (EN 302 636-4-1)
// and BTP (EN 302 636-5-1). Gives the message type (from the BTP port) and the sender's position
// vector, which GeoNetworking carries in every packet. The message bodies (CAM, DENM, SPATEM, ...)
// are ASN.1 UPER and are not decoded here.

#include <cstddef>
#include <cstdint>
#include <vector>

namespace its_g5_decode {

struct Info {
  uint16_t port{0};          // BTP destination port, 0 = none
  uint8_t station_type{0};   // ETSI station type from the GN address
  uint64_t addr{0};          // GN address (M bit, station type, MID), identifies the station
  bool secured{false};       // carried in an ETSI TS 103 097 signed packet (signature not verified)
  double lat{0}, lon{0};     // degrees
  float speed_kmh{0};
  float heading{0};          // degrees
};

inline const char *port_name(uint16_t port) {
  switch (port) {
    case 2001: return "CAM";
    case 2002: return "DENM";
    case 2003: return "MAPEM";
    case 2004: return "SPATEM";
    case 2005: return "SAEM";
    case 2006: return "IVIM";
    case 2007: return "SREM";
    case 2008: return "SSEM";
    case 2009: return "CPM";
    case 2010: return "EVCSN";
    case 2011: return "TRM";
    case 2013: return "RTCMEM";
    case 2018: return "VAM";
    default: return "other";
  }
}

inline const char *station_name(uint8_t type) {
  static const char *const NAMES[] = {"unknown",    "pedestrian",   "cyclist",    "moped",
                                      "motorcycle", "passengerCar", "bus",        "lightTruck",
                                      "heavyTruck", "trailer",      "specialVehicle", "tram"};
  if (type < sizeof(NAMES) / sizeof(NAMES[0]))
    return NAMES[type];
  return type == 15 ? "roadSideUnit" : "other";
}

inline uint16_t be16(const uint8_t *p) { return (p[0] << 8) | p[1]; }
inline int32_t be32(const uint8_t *p) {
  return static_cast<int32_t>((uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3]);
}

// Returns false for anything that is not a GeoNetworking packet it understands.
inline bool parse(const std::vector<uint8_t> &frame, Info &out) {
  const uint8_t *f = frame.data();
  const size_t n = frame.size();
  if (n < 24 || ((f[0] >> 2) & 3) != 2)  // data frames only
    return false;
  size_t i = 24;
  const bool qos = f[0] & 0x80;
  if (qos)
    i += 2;
  if ((f[1] & 3) == 3)  // ToDS + FromDS: fourth address
    i += 6;
  if (qos && (f[1] & 0x80))  // +HTC
    i += 4;

  static const uint8_t SNAP_GN[] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x89, 0x47};
  if (i + 8 + 4 > n)
    return false;
  for (size_t k = 0; k < 8; k++) {
    if (f[i + k] != SNAP_GN[k])
      return false;
  }
  i += 8;

  // GN basic header: version | next header, reserved, lifetime, remaining hop limit
  const uint8_t basic_nh = f[i] & 0x0F;
  i += 4;
  if (basic_nh == 2) {
    // ponytail: walks only the usual 1609.2 shape (signedData with an unsecuredData payload, no
    // external hash); anything else is skipped. A real OER decoder is needed for the rest.
    if (i + 7 > n || f[i] != 0x03 || f[i + 1] != 0x81 || !(f[i + 3] & 0x40) || f[i + 4] != 0x03 ||
        f[i + 5] != 0x80)
      return false;
    i += 6;
    if (f[i] & 0x80) {
      i += 1 + (f[i] & 0x7F);
    } else {
      i += 1;
    }
    out.secured = true;
  } else if (basic_nh != 1) {
    return false;
  }

  // GN common header (8 bytes): next header | reserved, header type | subtype, ...
  if (i + 8 > n)
    return false;
  const uint8_t common_nh = f[i] >> 4;
  const uint8_t ht = f[i + 1] >> 4, hst = f[i + 1] & 0x0F;
  i += 8;

  size_t pv, ext;  // offset of the source position vector, length of the extended header
  switch (ht) {
    case 1: pv = i; ext = 24; break;                          // beacon
    case 2: pv = i + 4; ext = 48; break;                      // GUC
    case 3: case 4: pv = i + 4; ext = 44; break;              // GAC, GBC
    case 5: pv = hst == 0 ? i : i + 4; ext = 28; break;       // SHB, TSB
    default: return false;                                    // location service, unknown
  }
  if (pv + 24 > n)
    return false;

  // Long position vector: GN address (8), timestamp (4), lat, lon (1/10 µdeg), PAI | speed, heading
  out.station_type = (f[pv] >> 2) & 0x1F;
  for (size_t k = 0; k < 8; k++)
    out.addr = (out.addr << 8) | f[pv + k];
  out.lat = be32(f + pv + 12) / 1e7;
  out.lon = be32(f + pv + 16) / 1e7;
  int16_t speed = be16(f + pv + 20) & 0x7FFF;
  if (speed & 0x4000)
    speed -= 0x8000;  // 15-bit signed, 0.01 m/s
  out.speed_kmh = speed * 0.036f;
  out.heading = be16(f + pv + 22) / 10.0f;

  if ((common_nh == 1 || common_nh == 2) && i + ext + 2 <= n)  // BTP-A / BTP-B
    out.port = be16(f + i + ext);
  return true;
}

struct Stats {
  uint32_t cam{0}, denm{0}, spatem{0}, mapem{0}, other{0};
  Info last;
};
inline Stats stats;

// Parse, count and log one frame; returns false if it was not decoded.
inline bool handle(const std::vector<uint8_t> &frame) {
  Info info;
  if (!parse(frame, info))
    return false;
  switch (info.port) {
    case 2001: stats.cam++; break;
    case 2002: stats.denm++; break;
    case 2004: stats.spatem++; break;
    case 2003: stats.mapem++; break;
    default: stats.other++; break;
  }
  stats.last = info;
  return true;
}

// Rate limit per station: true if addr was not let through in the last period_ms.
// ponytail: 32 slots, the oldest is evicted; with more stations in range some publish more often.
inline bool due(uint64_t addr, uint32_t now, uint32_t period_ms) {
  static struct {
    uint64_t addr;
    uint32_t ms;
  } slots[32]{};
  auto *slot = &slots[0];
  for (auto &s : slots) {
    if (s.addr == addr) {
      slot = &s;
      break;
    }
    if (now - s.ms > now - slot->ms)
      slot = &s;
  }
  if (slot->addr == addr && now - slot->ms < period_ms)
    return false;
  *slot = {addr, now};
  return true;
}

}  // namespace its_g5_decode
