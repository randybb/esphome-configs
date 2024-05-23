#include "pca9554.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pca9554 {

static const char *TAG = "pca9554";

void PCA9554Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up PCA9554...");
  if (!this->read_gpio_()) {
    ESP_LOGE(TAG, "PCA9554 not available under 0x%02X", this->address_);
    this->mark_failed();
    return;
  }

  this->write_gpio_();
  this->read_gpio_();
}
void PCA9554Component::dump_config() {
  ESP_LOGCONFIG(TAG, "PCA9554:");
  LOG_I2C_DEVICE(this)
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with PCA9554 failed!");
  }
}
bool PCA9554Component::digital_read(uint8_t pin) {
  this->read_gpio_();
  return this->input_mask_ & (1 << pin);
}
void PCA9554Component::digital_write(uint8_t pin, bool value) {
  if (value) {
    this->output_mask_ |= (1 << pin);
  } else {
    this->output_mask_ &= ~(1 << pin);
  }

  this->write_gpio_();
}
void PCA9554Component::pin_mode(uint8_t pin, uint8_t mode) {
  switch (mode) {
    case PCA9554_OUTPUT:
      // Clear mode mask bit
      this->mode_mask_ &= ~(1 << pin);
      // Write GPIO to enable input mode
      this->write_gpio_();
      break;
    case PCA9554_INPUT:
      // Set mode mask bit
      this->mode_mask_ |= 1 << pin;
      break;
    default:
      break;
  }
}
bool PCA9554Component::read_gpio_() {
  if (this->is_failed())
    return false;
  bool success;
  uint8_t data[2];

  if (!success) {
    this->status_set_warning();
    return false;
  }
  this->status_clear_warning();
  return true;
}
bool PCA9554Component::write_gpio_() {
  if (this->is_failed())
    return false;

  uint16_t value = 0;
  // Pins in OUTPUT mode and where pin is HIGH.
  value |= this->mode_mask_ & this->output_mask_;
  // Pins in INPUT mode must also be set here
  value |= ~this->mode_mask_;

  uint8_t data[2];
  data[0] = value;
  data[1] = value >> 8;

  this->status_clear_warning();
  return true;
}
float PCA9554Component::get_setup_priority() const { return setup_priority::IO; }

void PCA9554GPIOPin::setup() { this->pin_mode(this->mode_); }
bool PCA9554GPIOPin::digital_read() { return this->parent_->digital_read(this->pin_) != this->inverted_; }
void PCA9554GPIOPin::digital_write(bool value) { this->parent_->digital_write(this->pin_, value != this->inverted_); }
void PCA9554GPIOPin::pin_mode(uint8_t mode) { this->parent_->pin_mode(this->pin_, mode); }
PCA9554GPIOPin::PCA9554GPIOPin(PCA9554Component *parent, uint8_t pin, uint8_t mode, bool inverted)
    : GPIOPin(pin, mode, inverted), parent_(parent) {}

}  // namespace pca9554
}  // namespace esphome
