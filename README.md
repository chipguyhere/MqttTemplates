# MqttTemplates

This library for Arduino provides the foundation for creating sensor applications that report
data via secure connection to a public MQTT server.

What's presented here is a series of example Arduino sketches that securely report "hello world"
messages, with the device MAC address and a simple counting number (1... 2... 3... etc.).
Reporting is made to a public MQTT server already on the
Internet (the HiveMQ Public MQTT Broker), so you can test the functionality without needing to set up your own server.

These examples are meant to be edited to become your sketch that reports your
sensor data, ideally over a secure connection into your private MQTT server (whether in-cloud or on-premise),
using your own self-signed certificate.  Two-way communication is permitted (either by calling subscribe() on an MQTT topic,
or you can open a listening TCP or UDP socket), in case you want to allow something else to control or operate
your device.  The "last will and testament" feature of MQTT is also demonstrated in these examples, in case you want the MQTT
server to be able to report a loss of contact with your device.

## Quick Start Guide

### Step 1: Choose Your Hardware Template
The library provides five example sketches that can be called up through the Arduino IDE's "examples" menu
(File - Examples - chipguy_MQTT_templates):

- **AtomS3** - for the M5Stack AtomS3 device that has WiFi and a color status LED as output. This low-cost device is ideal for headless WiFi sensors that don't need a screen. This will also run on ESP32S3 Dev Kit (non-M5Stack)
- **M5Core** - for the M5Stack Basic Core device using WiFi. This sketch will also run on the basic ESP32 Dev Kit (non-M5Stack).
- **M5Core W5500** - uses Ethernet and assumes you have stacked the Core onto an M5Stack Ethernet base with the W5500 Ethernet chipset.
- **Atom W5500** - for the M5Stack Atom device, uses Ethernet, assumes you have stacked it onto the AtomPOE base.
- **PoESP32** - for the M5Stack PoESP32 Ethernet device. This is sold by M5Stack as a PoE-powered Ethernet-to-UART adapter, but under the hood, it's just a regular ESP32 device that can be reprogrammed. It has no USB port, flashing this device requires opening it and using a standard 6-pin ESP32 USB-to-serial programming adapter. (You can use Over-The-Air for subsequent updates over Ethernet, so opening the device is only required for the initial programming)

### Step 2: Required Configuration Variables
Copy and customize these variables in your sketch:

```cpp
// WiFi credentials (for WiFi-enabled templates)
const char* ssid = "YOUR_WIFI_SSID";
const char* wifi_password = "YOUR_WIFI_PASSWORD";

// MQTT Broker settings (%s gets replaced with device MAC address)
const char* mqtt_clientid = "%s";
const char* mqtt_server = "your.mqtt.broker.com";
const char* mqtt_user = "your_username";
const char* mqtt_password = "your_password";
const char* last_will_topic = "sensors/%s/status";

// TLS Certificate for secure connections
const char* ca_cert = "-----BEGIN CERTIFICATE-----\n"
                      "...your certificate here...\n"
                      "-----END CERTIFICATE-----";

// Over-The-Air updates (leave password blank to disable)
const char* ARDUINO_OTA_HOSTNAME = "MY_SENSOR_%s";
const char* ARDUINO_OTA_PASSWORD = ""; // Set password to enable OTA
```

### Step 3: Implement Your Sensor Logic
The core function you'll customize is `connectedLoop()`, which runs only when MQTT is connected:

```cpp
void connectedLoop() {
  static unsigned long lastReading = 0;
  
  // Read sensor every 30 seconds
  if (millis() - lastReading > 30000) {
    float sensorValue = analogRead(A0) * 3.3 / 4095.0; // Example: read voltage
    
    // Create topic with device MAC address
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char topic[50];
    snprintf(topic, sizeof(topic), "sensors/%02X%02X%02X%02X%02X%02X/voltage", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    // Publish data
    char payload[20];
    snprintf(payload, sizeof(payload), "%.2f", sensorValue);
    bool success = mqttClient.publish(topic, payload, true); // true = retained
    
    if (success) {
      lastReading = millis();
      feed_watchdog(); // Reset watchdog timer on successful publish
    }
  }
}
```

## Key Functions You Can Implement

### connectedLoop() - Required
Called repeatedly when MQTT connection is established. Use this for:
- Reading sensor data
- Publishing to MQTT topics  
- Handling periodic tasks
- Keep execution time short to avoid blocking network stack

### setup1() - Optional
Runs on a separate thread during startup. Use for:
- Display initialization
- Sensor setup
- Hardware configuration
- Runs concurrently with main networking setup

### loop1() - Optional  
Runs repeatedly on a separate thread. Use for:
- UI updates and display management
- Non-critical background tasks
- Responsive user interface elements
- Runs independently of networking thread

## Common Sensor Integration Patterns

<details>
<summary>Click to expand sensor examples</summary>

### Simple Periodic Reading
```cpp
void connectedLoop() {
  static unsigned long lastReading = 0;
  if (millis() - lastReading > 60000) { // Every minute
    float temperature = readTemperatureSensor();
    char payload[20];
    snprintf(payload, sizeof(payload), "%.1f", temperature);
    mqttClient.publish("sensors/temperature", payload, true);
    lastReading = millis();
    feed_watchdog();
  }
}
```

### Multiple Sensors with Different Intervals  
```cpp
void connectedLoop() {
  static unsigned long lastTemp = 0, lastHumidity = 0;
  
  if (millis() - lastTemp > 60000) { // Temperature every minute
    publishTemperature();
    lastTemp = millis();
  }
  
  if (millis() - lastHumidity > 300000) { // Humidity every 5 minutes
    publishHumidity(); 
    lastHumidity = millis();
  }
}
```

### JSON Payload for Multiple Values
```cpp
void connectedLoop() {
  static unsigned long lastReading = 0;
  if (millis() - lastReading > 30000) {
    char jsonPayload[200];
    snprintf(jsonPayload, sizeof(jsonPayload), 
      "{\"temperature\":%.1f,\"humidity\":%.1f,\"pressure\":%.2f}",
      readTemp(), readHumidity(), readPressure());
    mqttClient.publish("sensors/all", jsonPayload, true);
    lastReading = millis();
    feed_watchdog();
  }
}
```

### Event-Driven Publishing
```cpp
void connectedLoop() {
  static bool lastMotionState = false;
  bool currentMotion = digitalRead(MOTION_PIN);
  
  if (currentMotion != lastMotionState) {
    mqttClient.publish("sensors/motion", currentMotion ? "ON" : "OFF", true);
    lastMotionState = currentMotion;
    feed_watchdog();
  }
}
```

</details>

## Hardware Compatibility

These examples are designed for M5Stack's ESP32 products, but using M5Stack products 
is not actually necessary. For example,
the M5Core example will work on the basic ESP32 dev kit simply by removing the references
to the M5Core's built-in LCD screen: set DEMO_ON_LCD_SCREEN to 0, or delete the relevant code,
or even leave it in there (the example will still run and produce serial port output,
and doesn't care that no physical LCD screen is present).

## Basic Example Functionality

Each example will attempt to connect to the public HiveMQ MQTT broker using a secure connection
(TLS), and send simple "Hello World" messages. Upon success, the example will attempt to turn
on a green LED (assumed to be part of the device or connected) or change the LCD text to green
(for M5Core which has an LCD display instead of an LED).

**Status LED Colors:**
- **Red** - Network disconnected
- **Yellow** - Network connected, MQTT connecting
- **Green** - MQTT connected and operational

If the example finds itself unable to update the MQTT server, the ESP32 hardware watchdog timer is
engaged, and will hard reset the device.

Each example will start setup1() and loop1(), if they exist, and run the Arduino sketch of your
choice on a separate task thread, all while maintaining the persistent connection to the MQTT
server, and updating the MQTT topics of your choice.

# Software and Library Dependencies

Arduino IDE is available as a free download for Win/Mac/Linux from https://arduino.cc

Compiling for ESP32 microcontrollers requires that you have the ESP32 Arduino Core installed.
Using the "Boards Manager" method to do this is the simplest and easiest.
Directions to install the Core are here: https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html

To compile these examples, you'll want to have one or more of the following libraries already installed.
Use Arduino IDE's Library Manager to download and install these.

* *PubSubClient* (required by all examples for communicating with MQTT servers over TCP)
* *Adafruit NeoPixel* (to change the LED color on AtomS3, or attached to accessory port of PoESP32)
* *TFT_eSPI* (for the M5Stack Basic Core, to drive its LCD screen)

To use the LCD screen on M5Core, in addition to installing TFT_eSPI,
you will also have to copy one file (tft_setup.h, provided by me in *this* library in the M5Core example folder) into TFT_eSPI's library
folder, overwriting any existing file by the same name. This file contains the hardware configuration for the Core's LCD display.
The requirement to have you copy a setup file is a design decision by the authors of the TFT_eSPI library.
If you aren't using any LCD screen, the dependency on the TFT_eSPI library disappears if you change
DEMO_ON_LCD_SCREEN in the example sketch to 0.

# Installing the Library

To install this repository as a library into Arduino IDE, download it from GitHub as a ZIP file. The ZIP file will contain
a single folder (named MqttTemplates_main or similar)... place that single folder into your *libraries* folder which will
automatically be created and exist below your Arduino documents folder after you install any library from within Arduino IDE.

After the library is installed, these MQTT template examples will be recognized as new Examples that you can select from
the menus in Arduino IDE. Look under "Examples from Custom Libraries".

# Arduino Over-The-Air Functionality

Each example also includes support for Arduino Over-The-Air update functionality, so you
can flash new revisions of your sketch code over the network, but it is disabled until you set a password.
Simply setting a password in your sketch before uploading it over Serial/USB will enable the function.
Your ESP32 device will then advertise itself over the network as being able to accept Over-The-Air updates.
Enabling it the first time requires flashing the sketch over serial/USB, but from then
on, the Arduino IDE can see and update the device over the network if selected as the "port".

Despite the word "air" in the naming, Over-the-Air updates are supported for both Ethernet and WiFi devices.

## Troubleshooting

### Common Issues
- **Compilation errors**: Ensure ESP32 Arduino Core is installed and the correct board is selected
- **WiFi connection fails**: Check SSID and password, ensure 2.4GHz network
- **MQTT connection fails**: Verify server address, credentials, and certificate
- **Device resets frequently**: Check `feed_watchdog()` calls in your `connectedLoop()`

### Security Notes
- The included `ca_cert` is for the HiveMQ public broker - replace with your own for production
- For testing without proper certificates, you have two options:
  - **Keep TLS but skip certificate verification**: Replace `espClient.setCACert(ca_cert)` with `espClient.setInsecure()` in your library copy
  - **Disable TLS entirely**: Modify the library to use `WiFiClient` instead of `WiFiClientSecure`
- Always use strong passwords for OTA updates in production environments

## Documentation
- **[Getting Started](#quick-start-guide)** - Basic setup and examples above
- **[Sensor Patterns](#common-sensor-integration-patterns)** - Common implementation examples
- **[Hardware Compatibility](#hardware-compatibility)** - Device-specific information
