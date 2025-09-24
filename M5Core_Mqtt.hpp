// M5Core-based Headless MQTT Data Collector
// With Provisions for UI on Separate Task
// By Michael Caldwell-Waller
//
// Compiling:
// "M5Core"(M5Stack)
// Chip is ESP32.  USB is via a USB-to-serial converter.
//
// CRITICAL: DO NOT IMPLEMENT setup() AND loop() IN YOUR .ino FILE!
// This library provides its own setup() and loop() functions via common_MqttT.hpp.
// Instead, implement these functions in your .ino file:
//   - setup1() - optional initialization for UI thread (called once)
//   - loop1() - optional UI loop running on separate core/thread
//   - connectedLoop() - mandatory function called when MQTT is connected
//
// ARCHITECTURE PATTERN:
// 1. Include this header in your .ino file: #include "M5Core_Mqtt.hpp"
// 2. Define configuration variables (WiFi credentials, MQTT settings, etc.)
// 3. Implement connectedLoop() for your main application logic
// 4. Optionally implement setup1() and loop1() for UI on second thread
// 5. DO NOT implement setup() or loop() - the library handles these!
//
// setup1() and loop1() are defined as optional weak references.
// To create a UI task, simply define these.
//
// connectedLoop() (if defined) will be called from the networking thread's loop,
// only while a good connection to MQTT is established.
//
// Supports "M5Core - esp32" (core 3.x) or "M5Core - M5Stack" (core 2.x) ESP32 Arduino core 2.x and 3.x.
// Core 3.x currently crashes if WPA2-Enterprise is used.



#include "common_MqttT.hpp"
