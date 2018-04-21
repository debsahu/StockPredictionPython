#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>                          // https://github.com/marvinroger/async-mqtt-client
                                                      // https://github.com/me-no-dev/ESPAsyncTCP
#include <SSD1306.h>                                  // https://github.com/ThingPulse/esp8266-oled-ssd1306
//#include "secret.h"

#ifndef SECRET
  // Update these with values suitable for your network.
  const char* ssid = "wifi_ssid";
  const char* password = "wifi_pass";
  IPAddress MQTT_HOST(192, 168, 0, 000);
  uint16_t MQTT_PORT 1883;
#endif

const char* stktopic1 = "home/stock/yclose";
const char* stktopic2 = "home/stock/predhigh";
const char* stktopic3 = "home/stock/predlow";
const char* stktopic4 = "home/stock/predopen";
const char* stktopic5 = "home/stock/predclose";

int8_t nStatus = -1;
String prediction;
double yclose, predhigh, predlow, predopen, predclose;

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;
SSD1306  display(0x3c, D2, D1);
Ticker displayNxt;
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

void incrnStatus() {
  nStatus++;
}

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  uint16_t packetIdSub1 = mqttClient.subscribe(stktopic1,1);
  Serial.print("Subscribing at QoS 1, packetId: "); Serial.println(packetIdSub1);
  uint16_t packetIdSub2 = mqttClient.subscribe(stktopic2,1);
  Serial.print("Subscribing at QoS 1, packetId: "); Serial.println(packetIdSub2);
  uint16_t packetIdSub3 = mqttClient.subscribe(stktopic3,1);
  Serial.print("Subscribing at QoS 1, packetId: "); Serial.println(packetIdSub3);
  uint16_t packetIdSub4 = mqttClient.subscribe(stktopic4,1);
  Serial.print("Subscribing at QoS 1, packetId: "); Serial.println(packetIdSub4);
  uint16_t packetIdSub5 = mqttClient.subscribe(stktopic5,1);
  Serial.print("Subscribing at QoS 1, packetId: "); Serial.println(packetIdSub5);
}


void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.print("Disconnected from MQTT, reason: ");
  if (reason == AsyncMqttClientDisconnectReason::TLS_BAD_FINGERPRINT) {
    Serial.println("Bad server fingerprint.");
  } else if (reason == AsyncMqttClientDisconnectReason::TCP_DISCONNECTED) {
    Serial.println("TCP Disconnected.");
  } else if (reason == AsyncMqttClientDisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION) {
    Serial.println("Bad server fingerprint.");
  } else if (reason == AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED) {
    Serial.println("MQTT Identifier rejected.");
  } else if (reason == AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE) {
    Serial.println("MQTT server unavailable.");
  } else if (reason == AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS) {
    Serial.println("MQTT malformed credentials.");
  } else if (reason == AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED) {
    Serial.println("MQTT not authorized.");
  } else if (reason == AsyncMqttClientDisconnectReason::ESP8266_NOT_ENOUGH_SPACE) {
    Serial.println("Not enough space on esp8266.");
  }
  
  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t length, size_t index, size_t total) {
  Serial.print("MQTT: Recieved [");
  Serial.print(topic);
  Serial.printf("]: %s\n", payload);

  if (strcmp(topic, stktopic1) == 0) {
    yclose = atof(payload);
  } else if (strcmp(topic, stktopic2) == 0) {
    predhigh = atof(payload);
  } else if (strcmp(topic, stktopic3) == 0) {
    predlow = atof(payload);
  } else if (strcmp(topic, stktopic4) == 0) {
    predopen = atof(payload);
  } else if (strcmp(topic, stktopic5) == 0) {
    predclose = atof(payload);
  }
  stock_predictor();
}

void stock_predictor() {
  if(predhigh > yclose and predlow > yclose ) {
    prediction = "BUY \nat Open";
  } else if(predhigh < yclose and predlow < yclose ) {
    prediction = "SELL \nat Open";
  } else {
    prediction = "HOLD";
  }
}

void displayInit() {
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_24);
}

void displayInt(double dispInt, int x, int y) {
  //display.setColor(WHITE);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(x, y, String(dispInt));
  display.setFont(ArialMT_Plain_24);
  display.display();
}

void displayString(String dispString, int x, int y) {
  //display.setColor(WHITE);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(x, y, dispString);
  display.setFont(ArialMT_Plain_24);
  display.display();
}

void setup(void)
{
  Serial.begin(115200);

  displayInit();
  displayString("Welcome", 64, 15);
  
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setClientId("StockPredictor1");

  connectToWifi();

  displayNxt.attach(2, incrnStatus);
}

void loop(void){
  if (nStatus >5) nStatus = 0;
  switch (nStatus) {
    case 0:
      display.clear();
      displayString("TSLA", 64, 15);
      break;
    case 1:
      display.clear();
      displayString(prediction, 64, 0);
      break;
    case 2:
      display.clear();
      displayString("Pred High", 64, 0);
      displayInt(predhigh, 64, 30);
      break;
    case 3:
      display.clear();
      displayString("Pred Low", 64, 0);
      displayInt(predlow, 64, 30);
      break;
    case 4:
      display.clear();
      displayString("Pred Open", 64, 0);
      displayInt(predopen, 64, 30);
      break;
    case 5:
      display.clear();
      displayString("Pred Close", 64, 0);
      displayInt(predclose, 64, 30);
      break;
  }
}
