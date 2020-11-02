/*
   DotO_Server
*/


//add log on computer file

/*
   OPERATION PARAMETERS
*/

const char* ssid = "";
const char* password = "";


/*
   INCLUDES & LIBS
*/


#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>

#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BAND    915E6

String rssi = "RSSI --";
String packSize = "--";
String packet ;

/*
   WebServer
*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>


WebServer server(80);

/*
   STRUCTS
*/
#include <DotO_structs.h>



/*
   LoRa comm
*/
const int SENSOR_DATA_NUM = 4;

struct doto_packet {
  doto_status_type doto_status;
  sensor_data_type sensor_data[SENSOR_DATA_NUM];
};


void loraData() {
  Serial.println(rssi);
  Serial.println(packet);
}

void cbk(int packetSize) {

  if (packetSize == sizeof(doto_packet)) {
    packet = "DotO packet";
    LoRa.readBytes((uint8_t *) &doto_packet, packetSize);
  } else {
    packet = "";
    packSize = String(packetSize, DEC);
    for (int i = 0; i < packetSize; i++) {
      packet += (char) LoRa.read();
    }

  }
  rssi = "RSSI " + String(LoRa.packetRssi(), DEC) ;
  loraData();
}


void handleRoot() {
  //server.send(200, "text/plain", packet);

  char temp[400];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  snprintf(temp, 400, "<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>ESP32 Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP32!</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
  </body>\
</html>", hr, min % 60, sec % 60);
  server.send(200, "text/html", temp);

}


void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup(void) {
  Serial.begin(115200);
  while (!Serial);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");



  Serial.begin(115200);
  while (!Serial);
  Serial.println();
  Serial.println("LoRa Receiver Callback");
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  //LoRa.onReceive(cbk);
  LoRa.receive();
  Serial.println("init ok");
  //  display.init();
  //  display.flipScreenVertically();
  //  display.setFont(ArialMT_Plain_10);

  delay(1500);
}

void loop(void) {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    cbk(packetSize);
  }
  delay(10);
  server.handleClient();
}
