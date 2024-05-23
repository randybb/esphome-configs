#include "esphome.h"
using namespace esphome;

class SonoffOut1 : public Component, public switch_::Switch {
 public:
  void setup() override {
    // This will be called by App.setup()
  }
  
  void write_state(bool state) override {
    Serial.write(0xA0);
    Serial.write(0x04);
    if (state)
      Serial.write(0x05);
    else
      Serial.write(0x00);
    Serial.write(0xA1);
    Serial.write('\n');
    Serial.flush();
    publish_state(state);
  }
};

class SonoffOut2 : public Component, public switch_::Switch {
 public:
  void setup() override {
    // This will be called by App.setup()
  }

  void write_state(bool state) override {
    Serial.write(0xA0);
    Serial.write(0x04);
    if (state)
      Serial.write(0x06);
    else
      Serial.write(0x00);
    Serial.write(0xA1);
    Serial.write('\n');
    Serial.flush();
    
    publish_state(state);

  }
};
