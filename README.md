# MqttTemplates

This library for Arduino provides the foundation for creating simple sensor applications that report
data to a secure MQTT connection.  You can edit the examples to use the transport of your choice.  Most of the
heavy lifting is done in included ".hpp" files, to keep the actual example sketches short.

It's designed in the form of example sketches that securely report "hello world"
messages to a public MQTT server already on the Internet (the HiveMQ Public MQTT Broker), meant
to be edited to become your sketch that reports your valuable
sensor data into your private MQTT server (securely if desired), or to another TCP/UDP/IP network service of your choice.

The library provides five example sketches.  Simply compile and flash, providing WiFi credentials
or a wired Ethernet connection.

* M5Core - for the M5Stack BASIC device that has ESP32 and WiFi.
* M5Core W5500 - uses Ethernet and assumes you have stacked the M5Core onto an Ethernet module.
* AtomS3 - for the M5Stack AtomS3 device that has WiFi and an LED as output.
* Atom W5500 - for the M5Stack Atom device, uses Ethernet, assumes you have stacked it onto the AtomPOE base.
* PoESP32 - for the M5Stack PoESP32 Ethernet device.  Flashing this device requires opening it and
  using a standard ESP32 USB-to-serial programming adapter.  (You can use Over-The-Air for subsequent updates
  over Ethernet, so opening the device is only required for the initial programming)

These examples are designed for M5Stack products containing ESP32, but using M5Stack is not necessary.  For example,
the M5Core example will work on the basic ESP32 dev kit simply by removing the references
to the M5Core's built-in LCD screen: set DEMO_ON_LCD_SCREEN to 0, or delete the relevant code,
or even leave it in there (the example will still run even if no physical LCD screen is present).

Note that to use the LCD screen on M5Core, you'll have to install the TFT_eSPI library (externally),
and you will also have to copy one file (tft_setup.h, provided with example) into the TFT_eSPI library
folder.  This file contains the hardware configuration for the M5's LCD hardware.
The requirement to have you copy a setup file is a design decision by the authors of the TFT_eSPI library.

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

# Arduino Over-The-Air functionality

Each example also includes support for Arduino Over-The-Air update functionality, so you
can update your sketch code over the network, but it is disabled until you set a password.
Simply setting a password in your sketch before uploading it over Serial/USB will enable the function.
Enabling it the first time requires flashing the sketch over serial/USB, but from then
on, the Arduino IDE can see and update the device over the network if selected as the "port".

Despite the word "air" in the naming, Over-the-Air updates are supported for Ethernet-based devices as well as WiFi.
