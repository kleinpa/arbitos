#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include "private.h"
#include "http_content.h"
#include "hue.h"
#include "arbitos_hardware.h"

#define DEBUG_ESP_HTTP_CLIENT
#define DEBUG_HTTPCLIENT

#ifndef DEBUG_ESP_PORT
#define DEBUG_ESP_PORT Serial
#endif

#define DEBUG(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )

/* Values set in "private.h" */
// const char* ssid = "home";
// const char* password = "P4aaw0rd55";

const char applicationId[16] = {0x32, 0x07, 0x0b, 0x4f, 0x08, 0x3c, 0x42, 0x25, 0xa6, 0x09, 0x0c, 0x35, 0x97, 0x92, 0x16, 0xa1};
const int applicationVersion = 1;
const char* applicationName = "arbitos";

const int TIMEOUT_NETWORK = 5000;
const int TIMEOUT_HUE = 5000;

ESP8266WebServer server(80);
ArbitosHardware hardware;
Hue* hue;

char hostName[32];

// Persistent State
struct State {
  char id[16];
  int version;
  char hueUsername[40];
} state;

void saveState() {
  DEBUG("[state] Writing state to EEPROM\n");
  EEPROM.begin(sizeof(state));
  EEPROM.put(0, state);
  EEPROM.commit();
  EEPROM.end();
}

void loadState() {
  DEBUG("[state] Reading state from EEPROM\n");
  EEPROM.begin(sizeof(state));
  EEPROM.get(0, state);
  if (memcmp(applicationId, state.id, 16) ||
      applicationVersion != state.version) {
    DEBUG("[state] Version check failed, reinitializing\n");
    // Fails ID check, reinitialize
    memcpy(state.id, applicationId, 16);
    state.version = applicationVersion;
    state.hueUsername[0] = '\0';
    saveState();
  }
  EEPROM.end();
}

void handleData() {
  if (server.method() == HTTP_GET) {
    String url = "{";
    url += "\"hue\":";
    url += hue->getStatus();
    url += "}";
    server.send(200, "application/javascript", url);
  } else if (server.method() == HTTP_POST) {
  }
}

void setupMDNS()
{
  // Call MDNS.begin(<domain>) to set up mDNS to point to
  // "<domain>.local"
  if (!MDNS.begin(hostName))
  {
    DEBUG("[mdns] Error setting up MDNS responder\n");
    while(1) {
      delay(1000);
    }
  }

  MDNS.addService(applicationName, "tcp", 80);
  DEBUG("[mdns] mDNS responder started\n");
}

void setHostName() {
  String s = String(applicationName) + "-";
  byte mac[6];
  WiFi.macAddress(mac);
  s += String(mac[3], HEX);
  s += String(mac[4], HEX);
  s += String(mac[5], HEX);
  s.toCharArray(hostName, 32);
}

int light = 0;
void buttonPress(int button) {
  light = button;
}

void buttonLongPress(int button) {
  hardware.blinkLED(button, 3);
}

int s[3];
int colorUpdateNeeded = 0;
void sliderMove(int slider, int value) {
  s[slider] = value;
  colorUpdateNeeded = 1;
}

void setup(void)
{
  setHostName();
  Serial.begin(115200);
  DEBUG("[main] Starting %s\n", hostName);

  loadState();
  hue = new Hue("192.168.1.149", state.hueUsername);

  server.on("/data", handleData);
  loadHttpContent(server);
  server.begin();
  hardware.begin();
  hardware.onButtonLongPress(buttonLongPress);
  hardware.onButtonPress(buttonPress);
  hardware.onSliderMove(sliderMove);
}

void connectWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long start = millis();
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(50);
      if (millis() - start > TIMEOUT_NETWORK) {
        DEBUG("[network] Timeout connecting to network.\n");
        return;
      }
    }
    DEBUG("[network] Connected %d.%d.%d.%d\n", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
    setupMDNS();
  }
}


bool lastConnected = false;
long lastColorUpdate = millis();

void loop(void)
{
  hardware.update();
  connectWifi();
  MDNS.update();

  server.handleClient();

  if(hue->isConnected() && !lastConnected) {
    hue->getUsername().toCharArray(state.hueUsername, 40);
    saveState();
  }
  lastConnected = hue->isConnected();

  if(colorUpdateNeeded && lastColorUpdate + 100 < millis()) {
    colorUpdateNeeded = 0;
    DEBUG("[io] Got %d, %d, %d\n", s[0], s[1], s[2]);
    char req[100];

    hue->request(("/lights/"+String(light)+
    "/state").c_str(), "{\"on\":true,\"hue\":"+
    String(map(s[0], 0, 1024, 0, 65535))+
    ",\"sat\":"+
    String(map(s[1], 0, 1024, 0, 254))+
    ",\"bri\":"+
    String(map(s[2], 0, 1024, 0, 254))+
    ",\"transitiontime\":0}");
    lastColorUpdate = millis();
  }

  hardware.setLED(0, (light == 0));
  hardware.setLED(1, (light == 1));
  hardware.setLED(2, (light == 2));
}
