// ATOMS3-based Headless MQTT Data Collector
// With Provisions for Separate Sensor Task
// By Michael Caldwell-Waller

// LED Color:  Red-disconnected  Yellow=WiFiconnected  Green=MQTTconnected

// Compiling:
// "M5AtomS3"(M5Stack)
// Chip is ESP32-S3.  USB is native to this chip.
// USB CDC on boot: Enable
// Download trouble: try Upload speed 460800

// How to use from another sketch:
// 1. Create your sketch but delete setup() and loop(), I'm defining them.
// 2. Create your own AtomS3_Mqtt.cpp that includes this file (e.g. #include "../AtomS3_MQTT_Template/AtomS3_Mqtt.hpp")
// 3. Define everything that's declared here as extern (it could go inside AtomS3_Mqtt.cpp or the .ino file)
// 4. I'm assuming you don't want callbacks on received messages, but if you do, use mqttClient.setCallback
//    (Otherwise, I'll Serial.print messages, and reset the watchdog timer, when messages arrive)
// 5. Define connectedLoop() which will only get called while the mqtt server is connected.
//    connectedLoop() should acquire sensor data and publish it.

#ifndef ARDUINO_M5STACK_ATOMS3
#error AtomS3_Mqtt.hpp is designed for the M5Stack AtomS3 device, which doesn't seem to be selected as the build target.
#endif


#include <Adafruit_NeoPixel.h>

// This function gets called via weak reference to push the rgb status value
// to the physical LED, since one is present on the AtomS3.
void set_chipguy_rgb_pixel(uint8_t r, uint8_t g, uint8_t b) {
  static Adafruit_NeoPixel pixel(1, 35, NEO_GRB + NEO_KHZ800);
  static bool pixel_began;
  if (!pixel_began) pixel.begin(); else pixel_began=true;

  pixel.setPixelColor(0, pixel.Color(r,g,b));
  pixel.show();
}


void finish_chipguy_setup() {
  pinMode(41, INPUT_PULLUP);
}


#include "common_MqttT.hpp"

