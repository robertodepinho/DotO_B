//Dvorak on the Ocean - Buoy


/*   INCLUDES & LIB SETUP

*/

//using EEPROM for STATUS single record & SENSOR DATA 

// include library to read and write from flash memory
#include <EEPROM.h>

// define the number of bytes you want to access
#define EEPROM_SIZE 4096

//AXP Power & I2C Comm
#include <Wire.h>
#include <axp20x.h>
AXP20X_Class axp;
const uint8_t i2c_sda = 21;
const uint8_t i2c_scl = 22;




// OPERATING PARAMETERS //
const boolean INITIAL_MODE = false;  //Set to true for memory cleanup - 1st time run
const int LONG_SLEEP = 5;  //in minutes - 12h = 720
const int SHORT_SLEEP = 1; //in minutes - 3h = 180
const int DEBUG_LEVEL = 2; //0 - 1 - 2 - 3

// TASK MODE CYCLE //
// 0 - TX, 1-3 - Register data only, -1 - unknown
const byte TASK_UNKNOWN = -1;
const byte TASK_TX = 0;
const byte TASK_CYCLE_MAX = 3;


// STATUS RECORD STRUCT //
typedef struct {
  byte previous_task = TASK_UNKNOWN ;
  byte task = TASK_UNKNOWN;
  unsigned int cycle_count = 0;
  unsigned int data_index = -1;
  //add last tx time
  //add txt success
} doto_status_type;

doto_status_type last_status;
doto_status_type current_status;

boolean eeprom_ok = false;
boolean axp_ok = false;

// SENSOR DATA STRUCT //
typedef struct {
  unsigned int index = -1;
  boolean tx_ok = false;
  boolean tx_lora = false;
  boolean tx_lorawan = false;
  boolean tx_spot = false;
  float in_temp = 1e-20;
  float in_batt = 1e-20;
} sensor_data_type;

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

void write_sensor_data() {
  if (eeprom_ok) {
    sensor_data.index = ++current_status.data_index;
    unsigned long sensor_data_addr = get_sensor_data_addr(sensor_data.index);
    Serial.print("write addr:");
    Serial.println(sensor_data_addr);
    Serial.println(EEPROM.writeBytes(sensor_data_addr, &sensor_data, sizeof(sensor_data_type)));
    EEPROM.commit();
  }
}

unsigned long get_sensor_data_addr(unsigned int data_index) {
  //add round robin logic
  unsigned long  sensor_data_addr = 2* sizeof(doto_status_type) + 0 + data_index * sizeof(sensor_data_type);
  return (sensor_data_addr);
}

void dump_sensor_data(sensor_data_type sd) {
  Serial.println("-----");
  Serial.print("Index:");
  Serial.println(sd.index);
  Serial.print("TX all, lora, lorawan, spot:");
  Serial.print(sd.tx_ok); Serial.print(sd.tx_lora); Serial.print(sd.tx_lorawan); Serial.println(sd.tx_spot);
  Serial.print("Internal Temp:");
  Serial.println(sd.in_temp);
  Serial.print("Internal Batt:");
  Serial.println(sd.in_batt);
  Serial.println("-----");
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
void dump_status(doto_status_type  st) {
  byte previous_task;
  byte task;
  unsigned int cycle_count;
  unsigned int data_index;
  Serial.println("**** STATUS ****");
  Serial.print("Previous:");
  Serial.println(st.previous_task);
  Serial.print("Task:");
  Serial.println(st.task);
  Serial.print("Cycle count:");
  Serial.println(st.cycle_count);
  Serial.print("Data index:");
  Serial.println(st.data_index);
  Serial.println(TASK_UNKNOWN);



}

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
   SETUP
*/
void initial_setup() {

  doto_status_type initial_status;
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.writeBytes(0, &initial_status, sizeof(initial_status));
  EEPROM.commit();

}

void setup() {

  Serial.begin(115200);
  
  

  if (INITIAL_MODE == true) {
    initial_setup();
  }

  //Init EEPROM
  Serial.print("**** MEMORY");
  eeprom_ok = EEPROM.begin(EEPROM_SIZE);
  Serial.print("eeprom_ok:");
  Serial.println(eeprom_ok);
  Serial.print("doto_status_type:");
  Serial.println(sizeof(doto_status_type));
  Serial.print("sensor_data_type:");
  Serial.println(sizeof(sensor_data_type));
  
   
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


}


void loop() {

  // A - GET LAST DotO LAST STATUS & SET TASK
  //add   // if(last_tx_time > 12hs ago) current_task = TASK_TX
  if (eeprom_ok) EEPROM.readBytes(0, &last_status, sizeof(doto_status_type));
  update_status();


  // B - GET SENSOR DATA
  sensor_data.in_temp = get_in_temp();
  sensor_data.in_batt = get_in_batt();
  write_sensor_data();
  
  if(DEBUG_LEVEL>1) dump_sensor_data(sensor_data);
  if(DEBUG_LEVEL>2) dump_all_sensor_data();
  
  
  delay(20000);

  // C - SEND WITH LORAWAN - ALL TASK CYCLE MODES
  // C.1 - REGISTER TX_STATUS / SENSOR_DATA
  // C.2 - SET SHORT_SLEEP

  // D - SEND WITH DIRECT LORA - ALL TASK CYCLE MODES
  // D.1 - WAIT FOR ACK
  // D.2 - REGISTER TX_STATUS / SENSOR_DATA
  // D.3 - SET SHORT_SLEEP

  // E - SEND WITH SPOT ( if current_task == TASK_TX )

  // F - SLEEP - if(last_task == TASK_UNKNOWN) LONG_SLEEP else SHORT_SLEEP
  write_status();


}
