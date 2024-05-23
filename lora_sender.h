#include "esphome.h"
#include "TinyLoRa.h"
#include "SPI.h"

using namespace esphome;

class lora_sender : public PollingComponent, public text_sensor::TextSensor {
 public:
  // constructor
  lora_sender() : PollingComponent(15000) {}
  
  // Network Session Key (MSB)
  uint8_t NwkSkey[16] = { 0x31, 0x36, 0x41, 0x2E, 0xFE, 0x06, 0x84, 0x0B, 0x70, 0xB6, 0xCE, 0xBF, 0xBC, 0xAB, 0x06, 0x79 };

  // Application Session Key (MSB)
  uint8_t AppSkey[16] = { 0x93, 0x42, 0xD6, 0x50, 0xE4, 0xDE, 0x36, 0x11, 0x4E, 0x28, 0xEC, 0x31, 0xF5, 0x3A, 0xD7, 0x10 };

  // Device Address (MSB)
  uint8_t DevAddr[4] = { 0x26, 0x01, 0x17, 0xE0 };
  // Data Packet to Send to TTN
  //unsigned char loraData[11] = {"hello LoRa"};
  char loraData[11];

  // How many times data transfer should occur, in seconds
  const unsigned int sendInterval = 30;

  // Pinout for Adafruit Feather 32u4 LoRa
  // TinyLoRa lora = TinyLoRa(irq, cs, rst);
  TinyLoRa lora = TinyLoRa(18, 14, 26);

  void setup() override {
      //Serial.begin(115200);
      Serial.begin(9600);
      Serial.println("LoRa Sender");
      //LoRa.setPins(18, 14, 26);
      lora.setChannel(MULTI);
      // set datarate
      lora.setDatarate(SF7BW125);
      if (!lora.begin()) {
        Serial.println("Starting LoRa failed!");
        while (1);
        delay(1000);
      }
      Serial.println("Sending data packet...");
  }

  void update() override {
    // This will be called every "update_interval" milliseconds.
    // Publish state
    publish_state("Hello World");
    Serial.println("Sending LoRa Data...");
    strcpy(loraData,"Hello Lora");
    lora.sendData((unsigned char *)loraData, sizeof(loraData), lora.frameCounter);
    Serial.print("Frame Counter: ");Serial.println(lora.frameCounter);
    lora.frameCounter++;
  }
};

