#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/voltage_sampler/voltage_sampler.h"

namespace esphome {
namespace ads1100 {

enum ADS1100OSMode {
  ADS1100_OSMODE_SINGLE  = 0x80,
  ADS1100_OSMODE_BUSY    = 0x00,
  ADS1100_OSMODE_NOTBUSY = 0x80,
};

enum ADS1100Mode {
  ADS1100_MODE_CONTINUOUS  = 0x00,
  ADS1100_MODE_SINGLE      = 0x10,
};

enum ADS1100Rate {
  ADS1100_RATE_128 = 0x00,
  ADS1100_RATE_32  = 0x04,
  ADS1100_RATE_16  = 0x08,
  ADS1100_RATE_8   = 0x0c,
};

enum ADS1100Gain {
  ADS1100_GAIN_1  = 0x01,
  ADS1100_GAIN_2  = 0x02,
  ADS1100_GAIN_4  = 0x03,
  ADS1100_GAIN_8  = 0x04,
};

class ADS1100Sensor;

class ADS1100Component : public Component, public i2c::I2CDevice {
 public:
  void register_sensor(ADS1100Sensor *obj) { this->sensors_.push_back(obj); }
  /// Set up the internal sensor array.
  void setup() override;
  void dump_config() override;
  /// HARDWARE_LATE setup priority
  float get_setup_priority() const override { return setup_priority::DATA; }
  void set_continuous_mode(bool continuous_mode) { continuous_mode_ = continuous_mode; }

  /// Helper method to request a measurement from a sensor.
  float request_measurement(ADS1100Sensor *sensor);

 protected:
  std::vector<ADS1100Sensor *> sensors_;
  uint16_t prev_config_{0};
  bool continuous_mode_;
};

/// Internal holder class that is in instance of Sensor so that the hub can create individual sensors.
class ADS1100Sensor : public sensor::Sensor, public PollingComponent, public voltage_sampler::VoltageSampler {
 public:
  ADS1100Sensor(ADS1100Component *parent) : parent_(parent) {}
  void update() override;
  void set_rate(ADS1100Rate rate) { rate_ = rate; }
  void set_gain(ADS1100Gain gain) { gain_ = gain; }

  float sample() override;
  uint8_t get_rate() const { return rate_; }
  uint8_t get_gain() const { return gain_; }

 protected:
  ADS1100Component *parent_;
  ADS1100Rate rate_;
  ADS1100Gain gain_;
};

}  // namespace ads1100
}  // namespace esphome
