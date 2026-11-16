// Self-check for decode.h: one signed SHB CAM frame from a passenger car.
// Build: g++ -std=c++20 decode_test.cpp -o /tmp/t && /tmp/t
#include "decode.h"
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace its_g5_decode;
int main() {
  std::vector<uint8_t> f = {0x88, 0x00, 0, 0, 0xff,0xff,0xff,0xff,0xff,0xff, 1,2,3,4,5,6, 0xff,0xff,0xff,0xff,0xff,0xff, 0,0,  // QoS data hdr
                            0x00, 0x00,                                                    // QoS ctrl
                            0xAA,0xAA,0x03,0,0,0,0x89,0x47,                                // SNAP GN
                            0x12, 0x00, 0x1a, 0x01,                                        // basic, NH=2 secured
                            0x03, 0x81, 0x00, 0x40, 0x03, 0x80, 0x30,                      // 1609.2 signedData/unsecuredData len
                            0x20, 0x50, 0x02, 0x80, 0x00, 0x30, 0x01, 0x00};               // common: NH=BTP-B, HT=5 HST=0
  uint8_t pv[24] = {5 << 2, 0, 1,2,3,4,5,6,  0,0,0,0,
                    0x1C,0xB3,0x8A,0x40,   // lat 481528384 -> 48.1528384
                    0x0A,0x34,0x5A,0x60,   // lon 171203168 -> 17.1203168
                    0x05,0x6E,             // 1390 * 0.01 m/s = 50.04 km/h
                    0x04,0xD2};            // 123.4 deg
  f.insert(f.end(), pv, pv + 24);
  uint8_t media[4] = {0}; f.insert(f.end(), media, media + 4);
  uint8_t btp[4] = {0x07, 0xD1, 0, 0}; f.insert(f.end(), btp, btp + 4);  // port 2001
  Info i;
  assert(parse(f, i));
  assert(i.secured && i.port == 2001 && i.station_type == 5);
  assert(std::fabs(i.lat - 48.1528384) < 1e-7 && std::fabs(i.lon - 17.1203168) < 1e-7);
  assert(std::fabs(i.speed_kmh - 50.04f) < 0.01f && std::fabs(i.heading - 123.4f) < 0.01f);
  assert(handle(f) && stats.cam == 1);
  f[30] = 0x48; assert(!parse(f, i));  // not GeoNetworking
  std::puts("ok");
}
