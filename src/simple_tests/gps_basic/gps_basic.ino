/*****************************************
* ESP32 GPS VKEL 9600 Bds
******************************************/

#include <TinyGPS++.h>                       

TinyGPSPlus gps;                            
HardwareSerial Serial_here(1);                 
int count = 0;

void setup()
{
  Serial.begin(115200);
  Serial_here.begin(9600, SERIAL_8N1, 34, 12);   //17-TX 18-RX
}

void loop()
{
  count++;
  Serial.print("Count  : ");
  Serial.println(count);
  Serial.print("Latitude  : ");
  Serial.println(gps.location.lat(), 5);
  Serial.print("Longitude : ");
  Serial.println(gps.location.lng(), 4);
  Serial.print("Satellites: ");
  Serial.println(gps.satellites.value());
  Serial.print("Altitude  : ");
  Serial.print(gps.altitude.feet() / 3.2808);
  Serial.println("M");
  Serial.print("Time      : ");
  Serial.print(gps.time.hour());
  Serial.print(":");
  Serial.print(gps.time.minute());
  Serial.print(":");
  Serial.println(gps.time.second());
  Serial.println("**********************");

  smartDelay(1000);                                      

  if (millis() > 5000 && gps.charsProcessed() < 10)
    Serial.println(F("No GPS data received: check wiring"));

   delay(2000);
}

static void smartDelay(unsigned long ms)                
{
  unsigned long start = millis();
  do
  {
    while (Serial_here.available())
      gps.encode(Serial_here.read());
  } while (millis() - start < ms);
}
