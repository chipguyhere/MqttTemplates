// PoESP32-based Headless MQTT Data Collector
// With Provisions for UI on Separate Task
// By Michael Caldwell-Waller
//
// Compiling:
// "M5Core"(M5Stack)
// Chip is ESP32.  USB is via a USB-to-serial converter.
//
//
// setup1() and loop1() are defined as optional weak references.
// To create a UI task, simply define these.
//
// connectedLoop() (if defined) will be called from the networking thread's loop,
// only while a good connection to MQTT is established.
//
// Supports "M5Core - esp32" (core 3.x) or "M5Core - M5Stack" (core 2.x) ESP32 Arduino core 2.x and 3.x.
// Core 3.x currently crashes if WPA2-Enterprise is used.

#define ETH_PHY_ADDR        1
#define ETH_PHY_TYPE        ETH_PHY_W5500

#define ETH_PHY_CS   26
#define ETH_PHY_IRQ  34
#define ETH_PHY_RST  13
//#define ETH_PHY_CHIPGUY_RESET 13
#define ETH_PHY_SPI_FREQ_MHZ 1

// SPI pins
#define ETH_SPI_SCK  18
#define ETH_SPI_MISO 19
#define ETH_SPI_MOSI 23

#include "comETH_MqttT.hpp"