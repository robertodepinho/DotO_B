//Dvorak on the Ocean - Buoy

// OPERATING PARAMETERS //
const int LONG_SLEEP = 5;  //in minutes - 12h = 720
const int SHORT_SLEEP = 1; //in minutes - 3h = 180

// TASK MODE CYCLE //
const int TASK_UNKNOWN = -1;
const int TASK_TX = 0;
const int TASK_CYCLE_MAX = 3;

int current_task = TASK_UNKNOWN; // 0 - TX, 1-3 - Register data only, -1 - unknown 
int last_task = TASK_UNKNOWN;

// TX_STATUS RECORD STRUCT //

// SENSOR DATA STRUCT //

// LoRa PACKET STRUCT //

// Spot PACKET STRUCT //


void setup() {

}

void loop() {

// A - GET LAST TX_STATUS
// if(last_task = TASK_UNKNOWN) current_task = TASK_TX else current_task = last_task + 1
// if(last_tx_time > 12hs ago) current_task = TASK_TX

// B - GET SENSOR DATA

// C - SEND WITH LORA - ALL TASK CYCLE MODES
// C.1 - REGISTER TX_STATUS / SENSOR_DATA
// C.2 - SHORT_SLEEP

// D - SEND WITH SPOT ( if current_task == TASK_TX )

// E - SLEEP - if(last_task == TASK_UNKNOWN) LONG_SLEEP else SHORT_SLEEP


}
