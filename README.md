# MqttTemplates

This library for Arduino provides the foundation for creating simple sensor applications that report
data via secure connection to a public MQTT server.  It's meant for you to edit the examples to use
the transport and/or private MQTT server of your choice.

What's presented here is a series of example Arduino sketches that securely report "hello world"
messages, with the device MAC address and a simple counting number (1... 2... 3... etc.), to a public MQTT server already on the
Internet (the HiveMQ Public MQTT Broker).  It is meant to be edited to become your sketch that reports your
sensor data, ideally over a secure connection into your private MQTT server using your own
self-signed certificate.

The library provides five example sketches.  Simply compile and flash, first editing the sketch to provide WiFi credentials, or
else you provide a wired Ethernet connection.

* M5Core - for the M5Stack Basic Core device using WiFi.  This sketch will also run on the basic ESP32 Dev Kit (non-M5Stack).
* M5Core W5500 - uses Ethernet and assumes you have stacked the Core onto an M5Stack Ethernet base with the W5500 Ethernet chipset.
* AtomS3 - for the M5Stack AtomS3 device that has WiFi and a color status LED as output.  This low-cost device is ideal for headless WiFi sensors that don't need a screen.  This will also run on ESP32S3 Dev Kit (non-M5Stack)
* Atom W5500 - for the M5Stack Atom device, uses Ethernet, assumes you have stacked it onto the AtomPOE base.
* PoESP32 - for the M5Stack PoESP32 Ethernet device.  This is sold by M5Stack as a PoE-powered Ethernet-to-UART adapter,
  but under the hood, it's just a regular ESP32 device that can be reprogrammed.  It has no USB port, flashing this device requires opening it and
  using a standard 6-pin ESP32 USB-to-serial programming adapter.  (You can use Over-The-Air for subsequent updates
  over Ethernet, so opening the device is only required for the initial programming)

These examples are designed for M5Stack's ESP32 products, but using M5Stack products 
is not actually necessary.  For example,
the M5Core example will work on the basic ESP32 dev kit simply by removing the references
to the M5Core's built-in LCD screen: set DEMO_ON_LCD_SCREEN to 0, or delete the relevant code,
or even leave it in there (the example will still run and produce serial port output,
and doesn't care that no physical LCD screen is present).

# Basic example functionality

Each example will attempt to connect to the public HiveMQ MQTT broker using a secure connection
(TLS), and send simple "Hello World" messages.  Upon success, the example will attempt to turn
on a green LED (assumed to be part of the device or connected) or change the LCD text to green
(for M5Core which has an LCD display instead of an LED).

If the example finds itself unable to update the MQTT server, the ESP32 hardware watchdog timer is
engaged, and will hard reset the device.

Each example will start setup1() and loop1(), if they exist, and run the Arduino sketch of your
choice on a separate task thread, all while maintaining the persistent connection to the MQTT
server, and updating the MQTT topics of your choice.

# Library dependencies

To compile this, you'll want to have one or more of the following libraries already installed.
Use Arduino IDE's Library Manager to download and install these.

* *PubSubClient* (for communicating with MQTT servers over TCP)
* *Adafruit NeoPixel* (to change the LED color on AtomS3, or attached to port of PoESP32)
* *TFT_eSPI* (for the M5Stack Basic Core, to drive its LCD screen)

To use the LCD screen on M5Core, you'll have to install the TFT_eSPI library (externally),
and you will also have to copy one file (tft_setup.h, provided with example) into the TFT_eSPI library
folder.  This file contains the hardware configuration for the M5's LCD hardware.
The requirement to have you copy a setup file is a design decision by the authors of the TFT_eSPI library.
If you aren't using an LCD screen, the dependency on TFT_eSPI disappears if you change
DEMO_ON_LCD_SCREEN in the example sketch to 0.

# Arduino Over-The-Air functionality

Each example also includes support for Arduino Over-The-Air update functionality, so you
can update your sketch code over the network, but it is disabled until you set a password.
Simply setting a password in your sketch before uploading it over Serial/USB will enable the function.
Enabling it the first time requires flashing the sketch over serial/USB, but from then
on, the Arduino IDE can see and update the device over the network if selected as the "port".

Despite the word "air" in the naming, Over-the-Air updates are supported for Ethernet-based devices as well as WiFi.
