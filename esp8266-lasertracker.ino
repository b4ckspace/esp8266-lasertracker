#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <Bounce2.h>
#include "settings.h"

bool active = false;
unsigned long startTime = 0;
uint16_t durationSeconds = 0;
uint16_t lastDurationSeconds = 0;

char sprintfHelper[16] = {0};

Bounce debouncer = Bounce();
WiFiClient wifiClient;
PubSubClient mqttClient;

void setup() {
  Serial.begin(115200);

  pinMode(LASER_FAN_SENSE, INPUT);

  WiFi.hostname(WIFI_HOSTNAME);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  mqttClient.setClient(wifiClient);
  mqttClient.setServer(MQTT_HOST, 1883);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  debouncer.attach(LASER_FAN_SENSE);
  debouncer.interval(100);

  connectMqtt();

  mqttClient.publish(MQTT_TOPIC_POWERED, "on", true);
  mqttClient.publish(MQTT_TOPIC_OPERATION, "inactive", true);
}

void connectMqtt() {
  while (!mqttClient.connected()) {
    Serial.println("Connecting mqtt");
    mqttClient.connect("lasercutter", MQTT_TOPIC_POWERED, 1, true, "off");

    delay(1000);
  }
}

void sendDuration(const char* topic, uint16_t duration) {
  sprintf(sprintfHelper, "%d", durationSeconds);
  mqttClient.publish(topic, sprintfHelper);
}

void loop() {
  connectMqtt();

  if (debouncer.fell()) {
    startTime = millis();

    durationSeconds = 0;
    lastDurationSeconds = 0;

    active = true;
    sendDuration(MQTT_TOPIC_DURATION, 0);
    mqttClient.publish(MQTT_TOPIC_OPERATION, "active", true);

    Serial.println("Active");
  }

  if (active) {
    durationSeconds = (millis() - startTime) / 1000;

    if (durationSeconds != lastDurationSeconds) {
      lastDurationSeconds = durationSeconds;
      sendDuration(MQTT_TOPIC_DURATION, lastDurationSeconds);

      Serial.println(lastDurationSeconds);
    }
  }

  if (debouncer.rose()) {

    if (active) {
      sendDuration(MQTT_TOPIC_FINISHED, durationSeconds);
      mqttClient.publish(MQTT_TOPIC_OPERATION, "inactive", true);
    }

    active = false;
    Serial.println("Inactive");
  }

  mqttClient.loop();
  debouncer.update();
}
