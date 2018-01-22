/*
 Pashkadez ESP8266 Sonoff MQTT example
*/
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "XATA2";
const char* password = "Nevadastate01";
const char* mqtt_server = "192.168.1.110";
const char* mqtt_client_id = "Sonoff";
const char* sub_topic = "/in/sonoff";
const char* pub_topic = "/out/sonoff";

                                  //ESP-07 pins
#define D0              16        //WAKE  =>  16
#define D1              5         //IOS   =>  5
#define D2              4         //      =>  4
#define D3              0         //      =>  0    sonoff Button
#define D4              2         //LED   =>  2
#define D5              14        //CLK   =>  14   sonnof additional pin
#define D6              12        //MISO  =>  12   sonoff Relay
#define D7              13        //MOSI  =>  13   sonoff LED
#define D8              15        //CS    =>  15
#define D9              3         //RX    =>  3

#define RELAY_PIN       D6
#define TOGGLE_PIN      D3
#define LED_PIN         D7


boolean                 RELAY_state               = false;
boolean                 LED_state                 = false;
const char*             LIGHT_ON                  = "ON";
const char*             LIGHT_OFF                 = "OFF";

WiFiClient espClient;

PubSubClient client(espClient);
unsigned long keepalivetime=0;
unsigned long MQTT_reconnect=0;
unsigned long lastMqttSwitch = 0;
unsigned long lastMqttSensor = 0;

void setup_wifi() {

  delay(10);
  
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  Serial.print(topic);
  Serial.print("  ");
  String strTopic = String(topic);
  String strPayload = String((char*)payload);
  Serial.println(strPayload);

  if (strTopic == sub_topic) {
    if (strPayload == LIGHT_ON) {
      digitalWrite(RELAY_PIN, HIGH);
      digitalWrite(LED_PIN, HIGH);
      client.publish(pub_topic, "ON");
    }    

    else if (strPayload == LIGHT_OFF) {
      digitalWrite(RELAY_PIN, LOW);
      digitalWrite(LED_PIN, LOW);
      client.publish(pub_topic, "OFF");
      }
  } 
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
     
  pinMode(RELAY_PIN, OUTPUT); 
  digitalWrite(RELAY_PIN, LOW);
  pinMode(TOGGLE_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT); 
  digitalWrite(LED_PIN, LOW);
  delay (250);
  digitalWrite(LED_PIN, HIGH);
  
 if (client.connect(mqtt_client_id)) {
    client.publish(pub_topic, "OFF");
    client.subscribe("/in/#");
  }
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  if (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt_client_id)) {
      Serial.println("connected");
      // ... and resubscribe
      client.subscribe("/in/#");
    } 
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again ");
      // Wait 5 seconds before retrying
      delay(50);
    }
  }
}

void loop() {

  ArduinoOTA.handle();

  
  client.loop();

  if (lastMqttSwitch > millis()) lastMqttSwitch = 0;

  client.loop();

// реалізація роботи кнопки, це жопа але воно працює))))) 
  if (digitalRead(TOGGLE_PIN) == LOW) {
    RELAY_state = digitalRead(RELAY_PIN);
    digitalWrite(RELAY_PIN, !RELAY_state);
    LED_state = digitalRead(LED_PIN);
    digitalWrite(LED_PIN, !LED_state);
    if (digitalRead(RELAY_PIN)) client.publish(pub_topic, "ON");
    else client.publish(pub_topic, "OFF");
    delay (250);
  }

  client.loop();

 if (millis() > (lastMqttSwitch + 60000)) {
     if (!client.connected()) {
      if (client.connect(mqtt_client_id)) client.subscribe("/in/#");
    }

    client.loop();

    if (client.connected()) {
      if (digitalRead(RELAY_PIN)) client.publish(pub_topic, "ON");
      else client.publish(pub_topic, "OFF");
    } 
    lastMqttSwitch = millis();
  }

  client.loop();
if (!client.connected()) {
    reconnect();
  }

}

