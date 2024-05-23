#include "ads1100.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace ads1100 {

static const char *const TAG = "ads1100";
static const uint8_t ADS1100_REGISTER_CONVERSION = 0x00;
static const uint8_t ADS1100_REGISTER_CONFIG = 0x01;

static const uint8_t ADS1100_DATA_RATE_860_SPS = 0b111;

void ADS1100Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up ADS1100...");
  uint16_t value;
  if (!this->read_byte_16(ADS1100_REGISTER_CONVERSION, &value)) {
    this->mark_failed();
    return;
  }
  uint16_t config = 0;
  // Clear single-shot bit
  //        0b0xxxxxxxxxxxxxxx
  config |= 0b0000000000000000;
  // Setup multiplexer
  //        0bx000xxxxxxxxxxxx
  config |= ADS1100_MULTIPLEXER_8 << 12;

  // Setup Gain
  //        0bxxxx000xxxxxxxxx
  config |= ADS1100_GAIN_6P144 << 9;

  if (this->continuous_mode_) {
    // Set continuous mode
    //        0bxxxxxxx0xxxxxxxx
    config |= 0b0000000000000000;
  } else {
    // Set singleshot mode
    //        0bxxxxxxx1xxxxxxxx
    config |= 0b0000000100000000;
  }

  // Set data rate - 860 samples per second (we're in singleshot mode)
  //        0bxxxxxxxx100xxxxx
  config |= ADS1100_DATA_RATE_860_SPS << 5;

  // Set comparator mode - hysteresis
  //        0bxxxxxxxxxxx0xxxx
  config |= 0b0000000000000000;

  // Set comparator polarity - active low
  //        0bxxxxxxxxxxxx0xxx
  config |= 0b0000000000000000;

  // Set comparator latch enabled - false
  //        0bxxxxxxxxxxxxx0xx
  config |= 0b0000000000000000;

  // Set comparator que mode - disabled
  //        0bxxxxxxxxxxxxxx11
  config |= 0b0000000000000011;

  if (!this->write_byte_16(ADS1100_REGISTER_CONFIG, config)) {
    this->mark_failed();
    return;
  }
  this->prev_config_ = config;
}
void ADS1100Component::dump_config() {
  ESP_LOGCONFIG(TAG, "Setting up ADS1100...");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with ADS1100 failed!");
  }

  for (auto *sensor : this->sensors_) {
    LOG_SENSOR("  ", "Sensor", sensor);
    ESP_LOGCONFIG(TAG, "    Rate: %u", sensor->get_rate());
    ESP_LOGCONFIG(TAG, "    Gain: %u", sensor->get_gain());
  }
}
float ADS1100Component::request_measurement(ADS1100Sensor *sensor) {
  uint16_t config = this->prev_config_;
  // Multiplexer
  //        0bxBBBxxxxxxxxxxxx
  config &= 0b1000111111111111;
  config |= (sensor->get_multiplexer() & 0b111) << 12;

  // Gain
  //        0bxxxxBBBxxxxxxxxx
  config &= 0b1111000111111111;
  config |= (sensor->get_gain() & 0b111) << 9;

  if (!this->continuous_mode_) {
    // Start conversion
    config |= 0b1000000000000000;
  }

  if (!this->continuous_mode_ || this->prev_config_ != config) {
    if (!this->write_byte_16(ADS1100_REGISTER_CONFIG, config)) {
      this->status_set_warning();
      return NAN;
    }
    this->prev_config_ = config;

    // about 1.2 ms with 860 samples per second
    delay(2);

    // in continuous mode, conversion will always be running, rely on the delay
    // to ensure conversion is taking place with the correct settings
    // can we use the rdy pin to trigger when a conversion is done?
    if (!this->continuous_mode_) {
      uint32_t start = millis();
      while (this->read_byte_16(ADS1100_REGISTER_CONFIG, &config) && (config >> 15) == 0) {
        if (millis() - start > 100) {
          ESP_LOGW(TAG, "Reading ADS1100 timed out");
          this->status_set_warning();
          return NAN;
        }
        yield();
      }
    }
  }

  uint16_t raw_conversion;
  if (!this->read_byte_16(ADS1100_REGISTER_CONVERSION, &raw_conversion)) {
    this->status_set_warning();
    return NAN;
  }
  auto signed_conversion = static_cast<int16_t>(raw_conversion);

  float millivolts;
  switch (sensor->get_gain()) {
    case ADS1100_GAIN_6P144:
      millivolts = signed_conversion * 0.187500f;
      break;
    case ADS1100_GAIN_4P096:
      millivolts = signed_conversion * 0.125000f;
      break;
    case ADS1100_GAIN_2P048:
      millivolts = signed_conversion * 0.062500f;
      break;
    case ADS1100_GAIN_1P024:
      millivolts = signed_conversion * 0.031250f;
      break;
    case ADS1100_GAIN_0P512:
      millivolts = signed_conversion * 0.015625f;
      break;
    case ADS1100_GAIN_0P256:
      millivolts = signed_conversion * 0.007813f;
      break;
    default:
      millivolts = NAN;
  }

  this->status_clear_warning();
  return millivolts / 1e3f;
}

float ADS1100Sensor::sample() { return this->parent_->request_measurement(this); }
void ADS1100Sensor::update() {
  float v = this->parent_->request_measurement(this);
  if (!std::isnan(v)) {
    ESP_LOGD(TAG, "'%s': Got Voltage=%fV", this->get_name().c_str(), v);
    this->publish_state(v);
  }
}

}  // namespace ads1100
}  // namespace esphome
