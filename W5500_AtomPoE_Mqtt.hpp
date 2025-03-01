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
//#define ETH_CLK_MODE        ETH_CLOCK_GPIO0_IN


// SPI pins. atom 19,22,23,33
#define ETH_PHY_CS   19
#define ETH_PHY_IRQ  -1
#define ETH_PHY_RST  -1

#define ETH_SPI_SCK  22
#define ETH_SPI_MISO 23
#define ETH_SPI_MOSI 33

#include <Adafruit_NeoPixel.h>

// This function gets called via weak reference to push the rgb status value
// to the physical LED, since one is present on the AtomS3.
void set_chipguy_rgb_pixel(uint8_t r, uint8_t g, uint8_t b) {
  static Adafruit_NeoPixel pixel(1, 27, NEO_GRB + NEO_KHZ800);
  static bool pixel_began;
  if (!pixel_began) pixel.begin(); else pixel_began=true;

  pixel.setPixelColor(0, pixel.Color(r,g,b));
  pixel.show();
}

void finish_chipguy_setup() {
  pinMode(39, INPUT_PULLUP);
}



#include "comETH_MqttT.hpp"