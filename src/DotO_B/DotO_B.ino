//Dvorak on the Ocean - Buoy


// OPERATING PARAMETERS //
const boolean INITIAL_MODE = false;  //Set to true for memory cleanup - 1st time run
const int LONG_SLEEP = 5;  //in minutes - 12h = 720
const int SHORT_SLEEP = 1; //in minutes - 3h = 180
const int DEBUG_LEVEL = 2; //0 - 1 - 2 - 3


/*
    WIRES - GPIO - PIN
*/
const int  PIN_TEMP_SENSOR = 15;


/*   INCLUDES & LIB SETUP

*/

//using EEPROM for STATUS single record & SENSOR DATA

// include library to read and write from flash memory
#include <EEPROM.h>

// define the number of bytes you want to access
#define EEPROM_SIZE 4096
#define MAX_INDEX 100

//AXP Power & I2C Comm
#include <Wire.h>
#include <axp20x.h>
AXP20X_Class axp;
const uint8_t i2c_sda = 21;
const uint8_t i2c_scl = 22;

/*
   LORA
*/
//SENDER
#include <SPI.h>
#include <LoRa.h>

#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISnO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BAND  915E6

String rssi = "RSSI --";
String packSize = "--";
String packet ;

/*
   RTC
*/
#include <RTClib.h>

RTC_DS3231 rtc;

/*
   OUT TEMP SENSOR
*/
#include <OneWire.h>
#include <DS18B20.h>


OneWire oneWire(PIN_TEMP_SENSOR );
DS18B20 out_temp_sensor(&oneWire);


// TASK MODE CYCLE //
// 0 - TX, 1-3 - Register data only, -1 - unknown
const byte TASK_UNKNOWN = -1;
const byte TASK_TX = 0;
const byte TASK_CYCLE_MAX = 3;

/*
   STRUCTS
*/
#include <DotO_structs.h>


/*
    GLOBAL STATUS & DATA
*/

boolean eeprom_ok = false;
boolean axp_ok = false;
boolean lora_ok = false;
boolean rtc_ok = false;
boolean out_temp_ok = false;

doto_status_type last_status;
doto_status_type current_status;
sensor_data_type sensor_data;

// LoRa PACKET STRUCT //

// Spot PACKET STRUCT //


/*
   SENSOR DATA
*/
float get_in_temp() {
  float temp = 1e-10;
  if (axp_ok) {
    temp =  axp.getTemp();
    Serial.print("Internal Temp: ");
    Serial.print(temp);
    Serial.println("*C");
  }
  return (temp);
}

// Get internal battery voltage
float get_in_batt() {
  float batt = 1e-10;
  if (axp_ok) {
    batt =  axp.getBattVoltage();
    Serial.print("BAT Volate:");
    Serial.print(axp.getBattVoltage());
    Serial.println(" mV");
  }
  return (batt);
}


/*
   DATETIME
*/
// macros from DateTime.h
/* Useful Constants */
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24L)

/* Useful Macros for getting elapsed time */
#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)
#define numberOfMinutes(_time_) ((_time_ / SECS_PER_MIN) % SECS_PER_MIN)
#define numberOfHours(_time_) (( _time_% SECS_PER_DAY) / SECS_PER_HOUR)
#define elapsedDays(_time_) ( _time_ / SECS_PER_DAY)
DateTime millis_to_datetime(long millis_time) {
  long val = millis_time / 1000;
  int hours = numberOfHours(val);
  int minutes = numberOfMinutes(val);
  int seconds = numberOfSeconds(val);
  Serial.println("millis to datetime");
  Serial.println(val);
  Serial.println(hours);
  Serial.println(minutes);
  Serial.println(seconds);
  Serial.println(hours % 24);
  return (DateTime(2000, 1, 1, hours % 24, minutes, seconds));
}

DateTime get_date_time() {
  if (rtc_ok) {
    return (rtc.now());
  } else return (millis_to_datetime(millis()));
}

float get_out_temp() {

  if (out_temp_ok) {
    long start = millis();
    out_temp_sensor.requestTemperatures();
    while (!out_temp_sensor.isConversionComplete() & (millis()-start) < 10000);
    float temperature = out_temp_sensor.getTempC();
    return (temperature);
  } else return (-999);

}

void write_sensor_data() {
  if (eeprom_ok) {

    sensor_data.index = current_status.data_index;
    current_status.data_index++;
    if (current_status.data_index > MAX_INDEX) current_status.data_index = 0;

    unsigned long sensor_data_addr = get_sensor_data_addr(sensor_data.index);
    Serial.print("write addr:");
    Serial.println(sensor_data_addr);
    Serial.println(EEPROM.writeBytes(sensor_data_addr, &sensor_data, sizeof(sensor_data_type)));
    EEPROM.commit();
  }
}

unsigned long get_sensor_data_addr(unsigned int data_index) {
  unsigned long  sensor_data_addr = 2 * sizeof(doto_status_type) + 0 + data_index * sizeof(sensor_data_type);
  return (sensor_data_addr);
}


void dump_all_sensor_data() {

  char buffer_record[sizeof(sensor_data_type)];
  for (int i = 0; i <= current_status.data_index; i++) {
    if (eeprom_ok) {
      Serial.print("i:");
      Serial.println(i);
      sensor_data_type sd;
      Serial.print("readaddr:");
      Serial.println(get_sensor_data_addr(i));
      EEPROM.readBytes(get_sensor_data_addr(i), &sd, sizeof(sensor_data_type));
      dump_sensor_data(sd);
    }
  }

}


/*
   STATUS
*/

void update_status() {
  current_status.previous_task = last_status.task;
  current_status.cycle_count = last_status.cycle_count + 1;
  current_status.data_index = last_status.data_index;
  // if(last_task = TASK_UNKNOWN) current_task = TASK_TX else current_task = last_task + 1
  if (last_status.task == TASK_UNKNOWN) {
    current_status.task = TASK_TX;
  } else {
    current_status.task = last_status.task + 1;
    if (current_status.task > TASK_CYCLE_MAX) current_status.task = 0;
  }
  dump_status(current_status);
}


void write_status() {
  if (eeprom_ok) {
    EEPROM.writeBytes(0, &current_status, sizeof(doto_status_type));
    EEPROM.commit();
  }

}


/*
   LORA
*/
void send_lora(doto_status_type doto_status, sensor_data_type sensor_data ) {
  Serial.print("Sending LoRa:");
  Serial.println(LoRa.beginPacket());
  Serial.println(LoRa.print("DotO_B"));
  Serial.println(
    LoRa.write((uint8_t*)&doto_status, sizeof(doto_status_type))
  );
  Serial.println(LoRa.endPacket());

}


/*
   SETUP
*/
void initial_setup() {

  doto_status_type initial_status;
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.writeBytes(0, &initial_status, sizeof(initial_status));
  EEPROM.commit();

}

void setup() {

  if (DEBUG_LEVEL > 0) {
    Serial.begin(115200);
    while (!Serial);
  }
  Serial.println();

  if (INITIAL_MODE == true) {
    initial_setup();
  }

  //Init EEPROM
  Serial.println("**** MEMORY");
  eeprom_ok = EEPROM.begin(EEPROM_SIZE);
  Serial.print("eeprom_ok:");
  Serial.println(eeprom_ok);
  Serial.print("doto_status_type:");
  Serial.println(sizeof(doto_status_type));
  Serial.print("sensor_data_type:");
  Serial.println(sizeof(sensor_data_type));
  Serial.println("**** END MEMORY INIT");
  

  //LORA
  Serial.println("LoRa Begin");
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI0);
  int try_count = 0;
  while (!LoRa.begin(BAND) & try_count < 30) {
    Serial.print(try_count);
    Serial.println(" - Starting LoRa failed!");
    try_count++;
    delay(2000);
  }
  delay(1500);
  if (try_count < 29) {
    lora_ok = true;
    Serial.println("LoRa ok");
  }


  //Init I2C
  Wire.begin(i2c_sda, i2c_scl);  //Init I2C

  // use AXP192
  int ret = axp.begin(Wire, AXP192_SLAVE_ADDRESS);
  if (ret == AXP_FAIL) {
    Serial.println("AXP Power begin failed");
    axp_ok = false;
  } else {
    axp_ok = true;
  }


  //Init RTC
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    rtc_ok = false;
  } else rtc_ok = true;

  //Init Temp Sensor
  if (out_temp_sensor.begin() == false)
  {
    Serial.println("Error: Could not find temperature sensor.");
    out_temp_ok = false;
  } else out_temp_ok = true;



  //Power on parts
  axp.setPowerOutPut(AXP192_LDO2, AXP202_ON);
  axp.setPowerOutPut(AXP192_LDO3, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
  axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON);

  

}



void loop() {

  // A - GET LAST DotO LAST STATUS & SET TASK
  //add   // if(last_tx_time > 12hs ago) current_task = TASK_TX
  if (eeprom_ok) EEPROM.readBytes(0, &last_status, sizeof(doto_status_type));
  update_status();


  // B - GET SENSOR DATA
  sensor_data.date_time = get_date_time();
  sensor_data.in_temp = get_in_temp();
  sensor_data.out_temp = get_out_temp();
  sensor_data.in_batt = get_in_batt();
  
  write_sensor_data();

  if (DEBUG_LEVEL > 1) dump_sensor_data(sensor_data);
  if (DEBUG_LEVEL > 2) dump_all_sensor_data();




  // C - SEND WITH LORAWAN - ALL TASK CYCLE MODES
  // C.1 - REGISTER TX_STATUS / SENSOR_DATA
  // C.2 - SET SHORT_SLEEP

  // D - SEND WITH DIRECT LORA - ALL TASK CYCLE MODES
  send_lora(current_status, sensor_data);
  delay(10000);
  // D.1 - WAIT FOR ACK
  // D.2 - REGISTER TX_STATUS / SENSOR_DATA
  // D.3 - SET SHORT_SLEEP

  // E - SEND WITH SPOT ( if current_task == TASK_TX )

  // F - SLEEP - if(last_task == TASK_UNKNOWN) LONG_SLEEP else SHORT_SLEEP
  write_status();


}