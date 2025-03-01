// ESP32-based Headless MQTT Data Collector

#include <WiFi.h>
#include <ArduinoOTA.h>


#if ESP_ARDUINO_VERSION_MAJOR >= 3
#include <esp_eap_client.h>
#else
#include <esp_wpa2.h>
#endif

#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <esp_task_wdt.h> // Watchdog timer

extern char* ARDUINO_OTA_HOSTNAME;
extern char* ARDUINO_OTA_PASSWORD;

// WPA2 / WPA2 Enterprise credentials
extern bool using_WPA2_Enterprise;
extern const char* ssid;
extern const char* wifi_username; // Username (applies only to WPA2 Enterprise)
extern const char* wifi_password; // Password for authentication

// Do scan?
// recommend true if WiFi might have multiple AP's on same SSID.
// recommend false if WiFi SSID is likely to be hidden.
extern bool do_wifi_scan;

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


volatile uint32_t status_pixel_color;
void setPixelColor(uint8_t r, uint8_t g, uint8_t b) {
  uint32_t x = ((uint32_t)r << 16) + ((uint32_t)g << 8) + ((uint32_t)b);
  status_pixel_color = x;
	if (set_chipguy_rgb_pixel) set_chipguy_rgb_pixel(r,g,b);
}

bool got_disconnected_event=false;





void onWiFiEvent(WiFiEvent_t event) {
  if (event==ARDUINO_EVENT_WIFI_STA_DISCONNECTED) got_disconnected_event=true;
  if (event==ARDUINO_EVENT_ETH_DISCONNECTED) got_disconnected_event=true;
}

void feed_watchdog() { esp_task_wdt_reset(); }
void setup_wifi() {
  WiFi.disconnect(true);  // Disconnect any previous WiFi connection

  WiFi.mode(WIFI_STA);    // Set WiFi to station mode

  static bool ran_once;
  if (!ran_once) WiFi.onEvent(onWiFiEvent);
  ran_once=true;

  byte best_bssid[6], second_best_bssid[6];
  byte* selected_bssid = NULL;
  int32_t best_rssi = -999, second_best_rssi = -999;
  int32_t selected_channel=0, second_best_channel=0;

  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);

  if (do_wifi_scan) {
    // Do the scan.
    // The default for the "async" parameter is false, so our usage here will block till finished.
    int n = WiFi.scanNetworks();

    // Consider the SSID for best/secondbest candidacy (assuming encryptionType matches)
    // Note that if we find nothing (e.g. it's hidden), BSSID will remain null, and the
    // WiFi library will seek it automatically (without the benefit of best/secondbest)
    for (int i = 0; i < n; ++i) {
      if (WiFi.SSID(i)==ssid) {
        bool ssid_uses_WPA2_Enterprise = (WiFi.encryptionType(i)==WIFI_AUTH_WPA2_ENTERPRISE);
        if (using_WPA2_Enterprise == ssid_uses_WPA2_Enterprise) {
          if (WiFi.RSSI(i) > best_rssi) {
            // Found a new best -- demote any old one to second-best
            second_best_rssi = best_rssi, second_best_channel = selected_channel;
            best_rssi = WiFi.RSSI(i), selected_channel = WiFi.channel(i);
            byte* bssid = WiFi.BSSID(i);
            for (int k=0; k<6; k++) second_best_bssid[k]=best_bssid[k], best_bssid[k]= bssid[k];
            selected_bssid = best_bssid;
          } else if (WiFi.RSSI(i) > second_best_rssi) {
            // Found a new second-best
            second_best_rssi = WiFi.RSSI(i), second_best_channel = WiFi.channel(i);
            byte* bssid = WiFi.BSSID(i);
            for (int k=0; k<6; k++) second_best_bssid[k]=bssid[k];
          }
        }
      }
    }
    Serial.println("WiFi.scanNetworks() completed.");
  }

  if (using_WPA2_Enterprise) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    WiFi.begin(ssid, WPA2_AUTH_PEAP, wifi_username, wifi_username, wifi_password, NULL, NULL, NULL, -1, selected_channel, selected_bssid);
#else
    esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)wifi_username, strlen(wifi_username));
    esp_wifi_sta_wpa2_ent_set_username((uint8_t *)wifi_username, strlen(wifi_username));
    esp_wifi_sta_wpa2_ent_set_password((uint8_t *)wifi_password, strlen(wifi_password));
    esp_wifi_sta_wpa2_ent_enable(); 
    WiFi.begin(ssid, NULL, selected_channel, selected_bssid);
#endif 
  } else {
    WiFi.begin(ssid, wifi_password, selected_channel, selected_bssid);
  }

  int timeoutcounter=30;
  auto wfs = WiFi.status();
  static char* wfs_statuses[] = {"idle","no ssid","scanned","connected","conn fail","conn lost","disconn"};
	// Serial.print("WiFi status is: ");
	// ln(wfs_statuses[wfs]);

  while (wfs != WL_CONNECTED) {
    if (--timeoutcounter < 0) return;
    if (got_disconnected_event) {
			// Serial.println("WiFi got disconnected event.");
    	break;
    }
    delay(500);
    auto new_wfs = WiFi.status();
    if (new_wfs != wfs) {
    	// Serial.print("WiFi status is now: ");
    	// Serial.println(wfs_statuses[new_wfs]);
    }
    wfs = new_wfs;
  }
  got_disconnected_event=false;
  if (WiFi.status() != WL_CONNECTED && second_best_rssi != -999) {
    // Trying alternate (if one identified), or just trying via esp selection (if none)
    selected_bssid = NULL, selected_channel=0;
    if (second_best_rssi != -999) selected_bssid = second_best_bssid, selected_channel=second_best_channel;
    if (using_WPA2_Enterprise) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
      WiFi.begin(ssid, WPA2_AUTH_PEAP, wifi_username, wifi_username, wifi_password, NULL, NULL, NULL, -1, selected_channel, selected_bssid);
#else
      WiFi.begin(ssid, NULL, selected_channel, selected_bssid);
#endif
    } else WiFi.begin(ssid, wifi_password, selected_channel, selected_bssid);
    timeoutcounter = 30;
    wfs = WiFi.status();
    while (wfs != WL_CONNECTED) {
      if (--timeoutcounter < 0) return;
      if (got_disconnected_event) return;
      delay(500);
      wfs = WiFi.status();
    }
  }

  feed_watchdog(); // feed watchdog timer

  // Serial.print("WiFi connected, local IP ");
  // Serial.println(WiFi.localIP());

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
  

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  char fullHostname[64];
  // Using snprintf so that any "%s" in the hostname will be replaced by MAC address
  snprintf(fullHostname, 64, ARDUINO_OTA_HOSTNAME, WiFi.macAddress().c_str());
  ArduinoOTA.setHostname(fullHostname);

  ArduinoOTA.setPassword(ARDUINO_OTA_PASSWORD);


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

	if (*ARDUINO_OTA_PASSWORD) ArduinoOTA.begin();


	// call any finish functions if present
	if (finish_chipguy_setup) finish_chipguy_setup();

  Serial.println("setup() has completed.");

}

// Adds the device MAC address to a single string replacing %s, sharing a
// single common buffer.
static const char* withmac(const char *str) {
  // Return any string not containing a percent sign unchanged.
  const char *p = str;
  for (; *p; p++) if (*p=='%') break;
  if (*p==0) return str;

  static char withmac_buffer[256];  
  uint8_t mac[6];
  WiFi.macAddress(mac);  // Get MAC address
  char macstr[20];
  snprintf(macstr, sizeof(macstr), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  snprintf(withmac_buffer, sizeof(withmac_buffer), str, macstr, macstr, macstr);
  return withmac_buffer;
}

void loop() {

  while (WiFi.status() != WL_CONNECTED) {
    // LED RED
    setPixelColor(255,0,0);

    setup_wifi(); // Reconnect to WiFi if the connection is lost
    // LED YELLOW
    setPixelColor(255,255,0);

  }

  if (!mqttClient.connected()) {
    // LED YELLOW
    setPixelColor(255,255,0);

    static long last_reconnect_attempt;
    long l = millis() - last_reconnect_attempt;
    if (l > 20000 || last_reconnect_attempt==0) {
      last_reconnect_attempt = l;
      while (!mqttClient.connected()) {
        // try to connect, which will block to return true if connection succeeded, false if failed.
        if (mqttClient.connect(withmac(mqtt_clientid), mqtt_user, mqtt_password, last_will_topic, 1, true, "offline")) {
          feed_watchdog(); // feed watchdog timer
          if (watchdog_subscribe_topic != NULL) mqttClient.subscribe(withmac(watchdog_subscribe_topic));
          mqttClient.publish(withmac(last_will_topic), device_status_to_report, true);
        } else {
          for (int i=0; i<50; i++) {
            // give time to OTA handler so long as we are struggling with MQTT.
            ArduinoOTA.handle();
            if (WiFi.status() != WL_CONNECTED) return;
            delay(100);
          }
        }
      }
    }
  } else {
    // LED GREEN
    setPixelColor(0,255,0);
  }
  
  if (WiFi.status() != WL_CONNECTED) return;
  ArduinoOTA.handle();
  if (!mqttClient.loop()) return;

  if (connectedLoop) connectedLoop();

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


