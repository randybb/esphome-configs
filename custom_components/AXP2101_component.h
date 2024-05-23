#include "esphome.h"

#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"

#ifdef I2C_SDA
#undef I2C_SDA
#endif

#ifdef I2C_SCL
#undef I2C_SCL
#endif

#define I2C_SDA                     (7)
#define I2C_SCL                     (6)



class AXP2101Component : public Component {
 public:
  void setup() override {
    // This will be called once to set up the component
    // think of it as the setup() call in Arduino
    ESP_LOGD("custom", "Custom component setup");
    
    if (!PMU.begin(Wire, AXP2101_SLAVE_ADDRESS, I2C_SDA, I2C_SCL)) {
        Serial.println("Failed to initialize power.....");
        while (1) {
            delay(5000);
        }
    }

    // Set VSY off voltage as 2600mV , Adjustment range 2600mV ~ 3300mV
    PMU.setSysPowerDownVoltage(2600);

    //Turn off not use power channel
    PMU.disableDC2();
    PMU.disableDC3();
    PMU.disableDC4();
    PMU.disableDC5();

    PMU.disableALDO1();
    PMU.disableALDO2();
    PMU.disableALDO3();
    PMU.disableALDO4();
    PMU.disableBLDO1();
    PMU.disableBLDO2();

    PMU.disableCPUSLDO();
    PMU.disableDLDO1();
    PMU.disableDLDO2();
    // Board 5 Pin socket 3.3V power output control
    PMU.setDC3Voltage(3100);         //Extern 3100~ 3400V
    PMU.enableDC3();

    // Camera working voltage, please do not change
    PMU.setALDO1Voltage(1500);      // CAM DVDD
    PMU.enableALDO1();

    // Camera working voltage, please do not change
    PMU.setALDO2Voltage(3000);      // CAM DVDD
    PMU.enableALDO2();

    // Camera working voltage, please do not change
    PMU.setALDO4Voltage(3000);      // CAM AVDD
    PMU.enableALDO4();

    // Pyroelectric sensor working voltage, please do not change
    PMU.setALDO3Voltage(3300);        // PIR VDD
    PMU.enableALDO3();

    // Microphone working voltage, please do not change
    PMU.setBLDO1Voltage(3300);       // MIC VDD
    PMU.enableBLDO1();

    PMU.clearIrqStatus();

    PMU.enableVbusVoltageMeasure();
    PMU.enableBattVoltageMeasure();
    PMU.enableSystemVoltageMeasure();
    PMU.disableTemperatureMeasure();

    // TS Pin detection must be disable, otherwise it cannot be charged
    PMU.disableTSPinMeasure();

    // Disable all interrupts
    PMU.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    // Clear all interrupt flags
    PMU.clearIrqStatus();
    // Enable the required interrupt function
    PMU.enableIRQ(
        XPOWERS_AXP2101_BAT_INSERT_IRQ    | XPOWERS_AXP2101_BAT_REMOVE_IRQ      |   //BATTERY
        XPOWERS_AXP2101_VBUS_INSERT_IRQ   | XPOWERS_AXP2101_VBUS_REMOVE_IRQ     |   //VBUS
        XPOWERS_AXP2101_PKEY_SHORT_IRQ    | XPOWERS_AXP2101_PKEY_LONG_IRQ       |   //POWER KEY
        XPOWERS_AXP2101_BAT_CHG_DONE_IRQ  | XPOWERS_AXP2101_BAT_CHG_START_IRQ       //CHARGE
        // XPOWERS_PKEY_NEGATIVE_IRQ | XPOWERS_PKEY_POSITIVE_IRQ   |   //POWER KEY
    );

    // TS Pin detection must be disable, otherwise it cannot be charged
    PMU.disableTSPinMeasure();
    // Set the precharge charging current
    PMU.setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_50MA);
    // Set constant current charge current limit
    PMU.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_300MA);
    // Set stop charging termination current
    PMU.setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_25MA);

    // Set charge cut-off voltage
    PMU.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V1);

    // Set the time of pressing the button to turn off
    PMU.setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S); 
  }
  private:
    XPowersPMU  PMU;
};