// ESP32-based Headless MQTT Data Collector

#include <WiFi.h>
#include <ArduinoOTA.h>


#if ESP_ARDUINO_VERSION_MAJOR >= 3
#include <esp_eap_client.h>
#else
#include <esp_wpa2.h>
#endif

#include <PubSubClient.h>



#include "ETH.h"
#include <WiFiClientSecure.h>
#include <esp_task_wdt.h> // Watchdog timer

extern const char* ARDUINO_OTA_HOSTNAME;
extern const char* ARDUINO_OTA_PASSWORD;

// MQTT Broker settings
extern const char* mqtt_clientid;
extern const char *last_will_topic;
int mqtt_port = 8883;
extern const char* mqtt_server;
extern const char* mqtt_user; 
extern const char* mqtt_password;


// MQTT Topic that will be auto-subscribed to, in hopes of receiving
// something that will feed the watchdog timer.  Note that we'll let
// the watchdog reset the device if we don't receive anything on any topic for 60 seconds.
// Don't like?  then just feed the watchdog some other way, or turn it off.
// Note PubSubClient also pings server (within MQTT protocol) and disconnects on a
// 15 second timeout, which we would see.
char *watchdog_subscribe_topic = "unix_time/unix_time";

// SSL/TLS Certificate for MQTT Server (moved to ca_cw_cert.cpp)
extern const char* ca_cert;

WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

bool eth_connected=false;

char *device_status_to_report = "online";
bool reportable_initialization_failure=false;

void setup1() __attribute__((weak));
void loop1() __attribute__((weak));
void connectedLoop() __attribute__((weak));
void set_chipguy_rgb_pixel(uint8_t r, uint8_t g, uint8_t b) __attribute__((weak));
void finish_chipguy_setup() __attribute__((weak));

char MyEthMac[30];
char MyEthIP[30];


volatile uint32_t status_pixel_color;
void setPixelColor(uint8_t r, uint8_t g, uint8_t b) {
  uint32_t x = ((uint32_t)r << 16) + ((uint32_t)g << 8) + ((uint32_t)b);
  status_pixel_color = x;
	if (set_chipguy_rgb_pixel) set_chipguy_rgb_pixel(r,g,b);
}

bool got_disconnected_event=false;

void onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      //set eth hostname here
      uint8_t macbytes[6];
      char mactail[10];
      ETH.macAddress(macbytes);
      snprintf(mactail, 10, "%02X%02X%02X",macbytes[3],macbytes[4],macbytes[5]);
      char fullHostname[32];
      snprintf(fullHostname, 32, ARDUINO_OTA_HOSTNAME, mactail);
      ETH.setHostname(fullHostname);
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("ETH MAC: ");
      Serial.print(ETH.macAddress());
      strcpy(MyEthMac, ETH.macAddress().c_str());
      Serial.print(", IPv4: ");
      Serial.print(ETH.localIP());
      strcpy(MyEthIP, ETH.localIP().toString().c_str());
      if (ETH.fullDuplex()) Serial.print(", FULL_DUPLEX");
      Serial.print(", ");
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}

void feed_watchdog() { esp_task_wdt_reset(); }


void setup_wifi() {

  static bool ran_once;
  if (!ran_once) WiFi.onEvent(onWiFiEvent);
  ran_once=true;

#ifdef ETH_SPI_SCK
	// W5500 Ethernet over SPI
	//static SPIClass hspi(HSPI);
	SPI.begin(ETH_SPI_SCK, ETH_SPI_MISO, ETH_SPI_MOSI);
	ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST, SPI, ETH_PHY_SPI_FREQ_MHZ);	
#else
  //ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_POWER, ETH_PHY_MDC, ETH_PHY_MDIO,  ETH_CLK_MODE);
	ETH.begin();
#endif


  
  while (ETH.linkUp()==0) {
    Serial.print(".");
    delay(500);
  }

  for (int i=0; i<20; i++) {
    auto xip = ETH.localIP();
    if (xip[0]>0) break;
    delay(500);
  }


  feed_watchdog(); // feed watchdog timer

  Serial.println("");
  Serial.print("Local IP: ");
  Serial.println(ETH.localIP()); //  WiFi.localIP());


}




void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Rcvd [");
  Serial.print(topic);
  Serial.print("] ");
  if (reportable_initialization_failure==false) feed_watchdog(); // Feed watchdog
  for (int i = 0; i < length; i++) Serial.print((char)payload[i]);
  Serial.println();
}

// Task handle for the new task
TaskHandle_t myTaskHandle = NULL;
/*
void myTaskFunction(void *parameter) {
  if (setup1) setup1();
  while (true) loop1();
}
*/
void setup() {

  Serial.begin(115200);
  
#ifdef ETH_PHY_CHIPGUY_RESET
		pinMode(ETH_PHY_CHIPGUY_RESET, OUTPUT);
    digitalWrite(ETH_PHY_CHIPGUY_RESET, HIGH);
    delay(250);
    digitalWrite(ETH_PHY_CHIPGUY_RESET, LOW);
    delay(50);
    digitalWrite(ETH_PHY_CHIPGUY_RESET, HIGH);
    delay(350);
#endif
  
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  esp_task_wdt_deinit();
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = 60000,
    .idle_core_mask = 0,
    .trigger_panic = true
  };
  esp_task_wdt_init(&wdt_config);
#else
  esp_task_wdt_init(60, true); // enable 60-second watchdog timer
#endif
  esp_task_wdt_add(NULL);
  
  
  setPixelColor(0,255,255);

  if (loop1) {


    xTaskCreate(
      [](void *parameter) {
        if (setup1) setup1();
        while (true) loop1();
      }, // Task function
      //myTaskFunction,      // Task function
      "Arduino Task",      // Task name (for debugging)
      10000,               // Stack size (in words, not bytes)
      NULL,                // Task input parameter
      1,                   // Priority of the task
      &myTaskHandle        // Task handle
    );
  } else if (setup1) setup1();

  setPixelColor(255,0,0);

  setup_wifi();

  setPixelColor(255,255,0);

  espClient.setCACert(ca_cert); // Set CA certificate
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(callback);
  




  ArduinoOTA
    .onStart([]() {
      feed_watchdog(); // Feed watchdog
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      feed_watchdog(); // Feed watchdog
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });



	// call any finish functions if present
	if (finish_chipguy_setup) finish_chipguy_setup();

  Serial.println("setup() has completed.");

}


void loop() {

	static bool ota_has_started=false;
	if (ota_has_started==false && eth_connected==true) {
		ota_has_started=true;
  	// Port defaults to 3232
  	// ArduinoOTA.setPort(3232);

  	char fullHostname[64];
  	// Using snprintf so that any "%s" in the hostname will be replaced by MAC address
  	snprintf(fullHostname, 64, ARDUINO_OTA_HOSTNAME, ETH.macAddress().c_str());
    ArduinoOTA.setHostname(fullHostname);
    ArduinoOTA.setPassword(ARDUINO_OTA_PASSWORD);
		if (*ARDUINO_OTA_PASSWORD) ArduinoOTA.begin();
	
	
	}


	if (eth_connected==false) {
		setPixelColor(255,0,0);
	} else if (!mqttClient.connected()) {
    // LED YELLOW
    setPixelColor(255,255,0);

    static long last_reconnect_attempt;
    long l = millis() - last_reconnect_attempt;
    if (l > 20000 || last_reconnect_attempt == 0) {
      last_reconnect_attempt = l;
      while (!mqttClient.connected()) {
      	Serial.println("mqtt connect attempting.");
        // try to connect, which will block to return true if connection succeeded, false if failed.
        if (mqttClient.connect(mqtt_clientid, mqtt_user, mqtt_password, last_will_topic, 1, true, "offline")) {
          feed_watchdog(); // feed watchdog timer
          if (watchdog_subscribe_topic != NULL) mqttClient.subscribe(watchdog_subscribe_topic);
          mqttClient.publish(last_will_topic, device_status_to_report, true);
        } else {
          for (int i=0; i<50; i++) {
            // give time to OTA handler so long as we are struggling with MQTT.
            ArduinoOTA.handle();
            if (ETH.linkUp()==false) return;
            delay(100);
          }
        }
      }
    }
  } else {
    // LED GREEN
    setPixelColor(0,255,0);
  }
  
  ArduinoOTA.handle();
  if (!mqttClient.loop()) return;

  if (connectedLoop && eth_connected) connectedLoop();

  // void connectedLoop() {
  //   bool publish_as_retained = true;
  //   static ___ last_myvalue_published;
  //   static long myvalue_millis;
  //
  //   if ((long)millis() - myvalue_millis > 10000 || myvalue != last_myvalue_published) {
  //     bool success = mqttClient.publish("something/to/publish", "myvalue", publish_as_retained);
  //     if (success) last_myvalue_published=myvalue, myvalue_millis=millis();
  //   }
  // }  
}


