#include <axp20x.h>

AXP20X_Class axp;

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, 34, 12, false, 1000);   //TBeam GPS defaults
  Wire.begin(21, 22);

  axp.begin(Wire, AXP192_SLAVE_ADDRESS);  //initialise the AXP192

  axp.setChgLEDMode (AXP20X_LED_OFF);
  //  Enable power on the TBeam.
  axp.setPowerOutPut(AXP192_LDO2, AXP202_ON);
  axp.setPowerOutPut(AXP192_LDO3, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
  axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON);
}

void loop() {
  if (Serial.available()) {      // If anything comes in from the PC,
    axp.setChgLEDMode (AXP20X_LED_LOW_LEVEL);
    Serial1.write(Serial.read());   // read it and send it out to the GPS.
    axp.setChgLEDMode (AXP20X_LED_OFF);
  }

  if (Serial1.available()) {     // If anything comes in from the GPS,
    axp.setChgLEDMode (AXP20X_LED_LOW_LEVEL);
    Serial.write(Serial1.read());   // read it and send it out to the PC.
    axp.setChgLEDMode (AXP20X_LED_OFF);
  }
  
}
