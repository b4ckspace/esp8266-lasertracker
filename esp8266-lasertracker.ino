#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <Bounce2.h>
#include <SimpleTimer.h>
#include <ESP8266HTTPClient.h>
#include "settings.h"

bool active = false;
bool powered = true;

unsigned long startTime = 0;
uint16_t durationSeconds = 0;

char sprintfHelper[16] = {0};

Bounce laserActiveDebouncer = Bounce();
Bounce laserPoweredDebouncer = Bounce();

SimpleTimer timer;
WiFiClient wifiClient;
PubSubClient mqttClient;

void setup() {
  Serial.begin(115200);

  pinMode(LASER_ACTIVE_SENSE, INPUT);
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

  laserActiveDebouncer.attach(LASER_FAN_SENSE);
  laserActiveDebouncer.interval(150);
  
  laserPoweredDebouncer.attach(LASER_ACTIVE_SENSE);
  laserPoweredDebouncer.interval(50);

  connectMqtt();

  timer.setInterval(1000, []() {
    if (active) {
      durationSeconds++;
      publishDuration(MQTT_TOPIC_DURATION, durationSeconds);

      Serial.println(durationSeconds);
    }
  });

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

bool storeInLasertracker(uint16_t durationSeconds) {
  HTTPClient http;
  http.begin("http://" + LASERTRACKER_HOST + "/api/laseroperations?duration=" + String(durationSeconds)); //HTTP

  uint16_t statusCode = http.GET();
  http.end();
  
  if (statusCode == 200) {
    return true;
  }

  return false;
}

void publishDuration(const char* topic, uint16_t duration) {
  Serial.print("Pub: ");
  Serial.println(duration);
  
  sprintf(sprintfHelper, "%d", durationSeconds);
  mqttClient.publish(topic, sprintfHelper);
}

void loop() {
  connectMqtt();

  if (laserPoweredDebouncer.fell()) {
    durationSeconds = 0;
    powered = false;
    active = false;

    Serial.println("Laser is off");
  } else if (laserPoweredDebouncer.rose()) {
    durationSeconds = 0;
    powered = true;
    active = false;
    
    Serial.println("Laser is on");
  }

  if (powered) {

    // Laser is now powered
    if (laserActiveDebouncer.fell()) {
      startTime = millis();
  
      durationSeconds = 0;
  
      publishDuration(MQTT_TOPIC_DURATION, 0);
      mqttClient.publish(MQTT_TOPIC_OPERATION, "active", true);
      
      active = true;
      Serial.println("Active");
    }
 
    // Laser is now inactive
    if (laserActiveDebouncer.rose()) {
  
      if (active) {
        if (durationSeconds > 0) {
          publishDuration(MQTT_TOPIC_FINISHED, durationSeconds);
          storeInLasertracker(durationSeconds);
        }
        
        mqttClient.publish(MQTT_TOPIC_OPERATION, "inactive", true);
      }
  
      active = false;
      Serial.println("Inactive");
    }
    
  }

  mqttClient.loop();
  timer.run();
  
  laserActiveDebouncer.update();
  laserPoweredDebouncer.update();
}
