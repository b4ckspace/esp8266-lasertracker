# ESP8266-Lasertracker

This tool takes a digital input from our laser fan control and tracks the duration. The duration and operational state is publishes to mqtt topics

* project/laser/operation (laser active/inactive)
* project/laser/duration (current duration updated every second)
* project/laser/finished (whole duration in seconds)
* project/laser/powered (laser powered or off)

## config.h

You have to copy the config.h.example to config.h and replace the strings with your credentials

## dependencies

* PubSubClient
* Arduino/ESP8266
