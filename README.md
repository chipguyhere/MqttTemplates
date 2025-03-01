# MqttTemplates

This library for Arduino provides the foundation for creating simple sensor applications that report
data to secure MQTT.  You can edit the examples to use the transport of your choice.

It's designed in the form of example sketches that securely report "hello world"
messages to a public MQTT server, meant to be edited into your sketch that reports your valuable
sensor data into your private MQTT server, securely if desired.

The library provides five example sketches.  Simply compile and flash, providing WiFi credentials
or (for the W5500 examples) a wired Ethernet connection.

* M5Core - for the M5Stack BASIC device that has ESP32 and WiFi.
* M5Core W5500 - uses Ethernet and assumes you have stacked the M5Core onto an Ethernet module.
* AtomS3 - for the M5Stack AtomS3 device that has WiFi and an LED as output.
* Atom W5500 - for the M5Stack Atom device, uses Ethernet, assumes you have stacked it onto the AtomPOE base.
* PoESP32 - for the M5Stack PoESP32 Ethernet device.  Flashing this device requires opening it and using a standard ESP32 USB-to-serial programming adapter.

These examples are designed for M5Stack products but using M5Stack is not necessary.  For example,
the M5Core example will work on the basic ESP32 dev kit simply by removing the references
to the M5Core's built-in LCD screen: set DEMO_ON_LCD_SCREEN to 0, or delete the relevant code.

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

# Arduino Over-The-Air functionality

Each example also includes support for Arduino Over-The-Air update functionality, so you
can update your sketch code over the network, but it is disabled until you set a password.
Simply setting a password in your sketch before uploading it over Serial/USB will enable the function.
Enabling it the first time requires flashing the sketch over serial/USB, but from then
on, the Arduino IDE can see and update the device over the network if selected as the "port".
