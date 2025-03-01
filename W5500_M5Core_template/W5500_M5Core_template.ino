// M5Stack W5500-based Headless MQTT Data Collector
//
// Compiling:
// REQUIRES "ESP32 Dev Module" (esp32 core version 3.0 or later)
// W5500 support is not available in earlier core.

// Chip is standard ESP32.  Must use an ESP32 USB-to-serial 6-pin adapter or OTA.
// Tested with ESP32 core 3.1.0.
// Download trouble: try Upload speed 460800

// How to use:
// 0. Add or replace tft_setup.h as libraries/TFT_eSPI/tft_setup.h
// 1. Don't create setup() and loop().  The library defines them.  Use setup1() and loop1()
// 2. #include "W5500_M5Core_Mqtt.hpp"
// 3. Set password and other defined items.
// 4. If you want callbacks for received messages, use mqttClient.setCallback to change the handler.
//    (Otherwise, I'll Serial.print messages, and reset the watchdog timer, when messages arrive)
// 5. Add code to connectedLoop() which will only get called while the mqtt server is connected.
//    connectedLoop() should acquire sensor data and publish it.


#include "W5500_M5Core_Mqtt.hpp"


#define DEMO_ON_LCD_SCREEN 1


#if DEMO_ON_LCD_SCREEN==1
#include "NotoSansBold15.h"
#include <SPI.h>
#include "TFT_eSPI.h"
TFT_eSPI tft = TFT_eSPI();
#endif

// Items referenced by library.

// MQTT Broker settings.  %s gets replaced (via snprintf) with MAC address of device.
const char* mqtt_clientid = "%s";
const char *last_will_topic = "hello_world_%s/status";
const char* mqtt_server = "broker.hivemq.com";
const char* mqtt_user = "anyone";
const char* mqtt_password = "anypass";  

// Over-The-Air (OTA) update support.
// For security, OTA support will not start while password is still blank.
char* ARDUINO_OTA_HOSTNAME = "MY_ARDUINO_CLIENT_%s";   // Hostname as it will appear in Arduino IDE.  %s gets replaced with MAC address
char* ARDUINO_OTA_PASSWORD = "";                 

// CA certificate for the MQTT server, to support secure connections.
// This certificate was one I generated and probably won't work for you.
// Use yours, or disable certificate checking or TLS for testing.
// To circumvent security, clone your own AtomS3_Mqtt.hpp and replace
// call to setCACert() with setInsecure(), or disable TLS by changing
// WiFiClientSecure to WiFiClient.
const char* ca_cert_mine = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDszCCApugAwIBAgIUZaVFC64GKj9D1cz6d54zPBnwIDkwDQYJKoZIhvcNAQEL\n" \
"BQAwaDELMAkGA1UEBhMCVVMxDTALBgNVBAgMBFV0YWgxDjAMBgNVBAcMBVNhbmR5\n" \
"MRgwFgYDVQQKDA9DYWxkd2VsbC1XYWxsZXIxIDAeBgNVBAMMF0NhbGR3ZWxsLVdh\n" \
"bGxlciBSb290IENBMCAXDTIzMTIxOTIxNTcxOVoYDzIwNTIwMTAxMjE1NzE5WjBo\n" \
"MQswCQYDVQQGEwJVUzENMAsGA1UECAwEVXRhaDEOMAwGA1UEBwwFU2FuZHkxGDAW\n" \
"BgNVBAoMD0NhbGR3ZWxsLVdhbGxlcjEgMB4GA1UEAwwXQ2FsZHdlbGwtV2FsbGVy\n" \
"IFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDp1ING4v0V\n" \
"SceGBlCmfoFBIWdiR8xBYb76YtIrJJoAZ/XcapJtkAXkh+CUrgj8gi1iCTXdC5Ng\n" \
"JIMH/Ba+hZNaNaLtmsrBT5GFq0aAURHzaF0BkoaCWoQJWrdg3jEZDxcUkqATXQD+\n" \
"7zZyK+zkl02IvVBWVcJd09rlZ5TvXQqCNVr2psAC8LUQvX7S4sqkhN1bY+HVHpYN\n" \
"mc5awySApI4KPor150ew8Ic5i60fVJtDZnnRPp3hruLfWkY6jyvDu5dJHtN3aIYQ\n" \
"udaAJA6ykvh+sZ72kBvwRtFO23l7hDG8Tu23+qtILnvYFXp66KMMJcmTywOc5nFO\n" \
"GhY9Em5TazvfAgMBAAGjUzBRMB0GA1UdDgQWBBQoR5EAVhURqC4f0IA2UIA2Qtsv\n" \
"8zAfBgNVHSMEGDAWgBQoR5EAVhURqC4f0IA2UIA2Qtsv8zAPBgNVHRMBAf8EBTAD\n" \
"AQH/MA0GCSqGSIb3DQEBCwUAA4IBAQCdyYdPs9OlENeipGvGxKjSdmptyYxa0Khe\n" \
"7ysPedRXa9X2zNJ8ta0eriOonzjL/Jk3X6iSTO9maZ679Ue9EpxB//Q2puI70xwf\n" \
"MnXF28ZzOTzmi7wHI+6me0JKoadoJj95fj4nT2yZFE2evhLUx4pEUj+ys5glRMAP\n" \
"M3JOLmu3zgyxzf/7O5Oyzk0UCiCeH2yd+iiMYbQjbATBbmhODEXIaP5+wAcFRlA0\n" \
"rgzA9S9WDyb1sys43ietK9fGfn4an6zQILHUYYVvK0iN1XjOm3NrKe+WxvZkxmqf\n" \
"7xMQiPkvnqa3iQqvFKcGTK1wtqf0zwNRybqGJC7LYDolj2fD5O9o\n" \
"-----END CERTIFICATE-----";

// CA root certificate for the HiveMQ Free Public MQTT server, provided
// for testing purposes.
const char* ca_cert = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIEkjCCA3qgAwIBAgITBn+USionzfP6wq4rAfkI7rnExjANBgkqhkiG9w0BAQsF\n" \
"ADCBmDELMAkGA1UEBhMCVVMxEDAOBgNVBAgTB0FyaXpvbmExEzARBgNVBAcTClNj\n" \
"b3R0c2RhbGUxJTAjBgNVBAoTHFN0YXJmaWVsZCBUZWNobm9sb2dpZXMsIEluYy4x\n" \
"OzA5BgNVBAMTMlN0YXJmaWVsZCBTZXJ2aWNlcyBSb290IENlcnRpZmljYXRlIEF1\n" \
"dGhvcml0eSAtIEcyMB4XDTE1MDUyNTEyMDAwMFoXDTM3MTIzMTAxMDAwMFowOTEL\n" \
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n" \
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n" \
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n" \
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n" \
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n" \
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n" \
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n" \
"jgSubJrIqg0CAwEAAaOCATEwggEtMA8GA1UdEwEB/wQFMAMBAf8wDgYDVR0PAQH/\n" \
"BAQDAgGGMB0GA1UdDgQWBBSEGMyFNOy8DJSULghZnMeyEE4KCDAfBgNVHSMEGDAW\n" \
"gBScXwDfqgHXMCs4iKK4bUqc8hGRgzB4BggrBgEFBQcBAQRsMGowLgYIKwYBBQUH\n" \
"MAGGImh0dHA6Ly9vY3NwLnJvb3RnMi5hbWF6b250cnVzdC5jb20wOAYIKwYBBQUH\n" \
"MAKGLGh0dHA6Ly9jcnQucm9vdGcyLmFtYXpvbnRydXN0LmNvbS9yb290ZzIuY2Vy\n" \
"MD0GA1UdHwQ2MDQwMqAwoC6GLGh0dHA6Ly9jcmwucm9vdGcyLmFtYXpvbnRydXN0\n" \
"LmNvbS9yb290ZzIuY3JsMBEGA1UdIAQKMAgwBgYEVR0gADANBgkqhkiG9w0BAQsF\n" \
"AAOCAQEAYjdCXLwQtT6LLOkMm2xF4gcAevnFWAu5CIw+7bMlPLVvUOTNNWqnkzSW\n" \
"MiGpSESrnO09tKpzbeR/FoCJbM8oAxiDR3mjEH4wW6w7sGDgd9QIpuEdfF7Au/ma\n" \
"eyKdpwAJfqxGF4PcnCZXmTA5YpaP7dreqsXMGz7KQ2hsVxa81Q4gLv7/wmpdLqBK\n" \
"bRRYh5TmOTFffHPLkIhqhBGWJ6bt2YFGpn6jcgAKUj6DiAdjd4lpFw85hdKrCEVN\n" \
"0FE6/V1dN2RMfjCyVSRCnTawXZwXgWHxyvkQAiSr6w10kY17RSlQOYiypok1JR4U\n" \
"akcjMS9cmvqtmg5iUaQqqcT5NJ0hGA==\n" \
"-----END CERTIFICATE-----";




char *hello_topic_ptr=NULL;
char hello_topic_buffer[50];


// If setup1 exists, then it will be called using the 2nd core/thread prior to
// any calls to loop1(), and will run concurrently with 1st thread's setup().
void setup1() {

#if DEMO_ON_LCD_SCREEN==1
  tft.begin();
  const int baseRotation = 1;
  tft.setRotation(baseRotation);
  tft.fillScreen(TFT_BLACK);
  tft.loadFont(NotoSansBold15);    // Must load the font first
  tft.setCursor(0,0);
#endif  
  
}




// If loop1 exists, then it will be called repeatedly on the 2nd core/thread
// after setup1() completes (if defined)
void loop1() {
  

#if DEMO_ON_LCD_SCREEN==1
  tft.setCursor(0,0);
  tft.setTextColor(tft.color24to16(status_pixel_color), 0, true);
  tft.println("network");
  if (hello_topic_ptr) tft.println(hello_topic_ptr);
  delay(200);
#endif


}




// connectedLoop() is a mandatory definition which gets called repeatedly, using
// the first thread, from the MQTT client thread's main loop, but only while the MQTT server is
/// connected.  It should publish sensor data to the MQTT server.
void connectedLoop() {

  // EXAMPLE PUBLICATION... publishes a new count every 10 seconds or so.
  // Note that mqttClient.publish is generally a blocking call, not returning until success or
  // after timeout for failure.  This is planned for.  Use loop1() thread for any UI or other
  // logic that needs to maintain realtime responsiveness.
  bool publish_as_retained = true;
  static int last_myvalue_published=0;
  static long myvalue_millis;
  if ((long)millis() - myvalue_millis > 10000) {
    uint8_t mac[6];
    WiFi.macAddress(mac);  // Get MAC address
    snprintf(hello_topic_buffer, sizeof(hello_topic_buffer), "hello_world_%02X%02X%02X%02X%02X%02X/hello", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    hello_topic_ptr = hello_topic_buffer;
    char myvalue_str[30];
    sprintf(myvalue_str, "myvalue-%d",last_myvalue_published+1);
    bool success = mqttClient.publish(hello_topic_buffer, myvalue_str, publish_as_retained);
    if (success) {
      last_myvalue_published++, myvalue_millis=millis();
      feed_watchdog();
      Serial.print("Successfully published ");
      Serial.print(myvalue_str);
      Serial.print(" to ");
      Serial.println(hello_topic_buffer);
    }
  }
}