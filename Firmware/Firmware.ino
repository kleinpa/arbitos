#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include "private.h"
#include "http_content.h"
#include "hue.h"

#define DEBUG_ESP_HTTP_CLIENT
#define DEBUG_HTTPCLIENT

#ifndef DEBUG_ESP_PORT
#define DEBUG_ESP_PORT Serial
#endif

#define DEBUG(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )

/* Values set in "private.h" */
// const char* ssid = "home";
// const char* password = "P4aaw0rd55";

const int TIMEOUT_NETWORK = 5000;
const int TIMEOUT_HUE = 5000;

const int pin_s0_en = 4;
const int pin_s1_en = 5;
const int pin_s2_en = 15;

const int pin_led_0 = 12;
const int pin_led_1 = 14;
const int pin_led_2 = 16;

const int pin_button_0 = 13;
const int pin_button_1 = 0;
const int pin_button_2 = 2;


ESP8266WebServer server(80);
Hue* hue;

char name[32];

// Persistent State
char STATE_ID[16] = {0x32, 0x07, 0x0b, 0x4f, 0x08, 0x3c, 0x42, 0x25, 0xa6, 0x09, 0x0c, 0x35, 0x97, 0x92, 0x16, 0xa1};
int STATE_VERSION = 1;
struct State {
  char id[16];
  int version;
  char hue_username[40];
} state;

void saveState() {
  DEBUG("[state] Writing state to EEPROM\n");
  EEPROM.put(0, state);
  EEPROM.commit();
}

void loadState() {
  DEBUG("[state] Reading state from EEPROM\n");
  EEPROM.get(0, state);
  if (memcmp(STATE_ID, state.id, 16) ||
      STATE_VERSION != state.version) {
    DEBUG("[state] Version check failed, reinitializing\n");
    // Fails ID check, reinitialize
    memcpy(state.id, STATE_ID, 16);
    state.version = STATE_VERSION;
    state.hue_username[0] = '\0';
    saveState();
  }
}

void handleData() {
  if (server.method() == HTTP_GET) {
    String url = "{";
    url += "\"hue\":";
    url += hue->get_status();
    url += "}";
    server.send(200, "application/javascript", url);
  } else if (server.method() == HTTP_POST) {
  }
}

void setupMDNS()
{
  // Call MDNS.begin(<domain>) to set up mDNS to point to
  // "<domain>.local"
  if (!MDNS.begin(name))
  {
    DEBUG("[mdns] Error setting up MDNS responder\n");
    while(1) {
      delay(1000);
    }
  }

  MDNS.addService("arbitos", "tcp", 80);
  DEBUG("[mdns] mDNS responder started\n");
}

void set_name() {
  String s = "arbitos-";
  byte mac[6];
  WiFi.macAddress(mac);
  s += String(mac[3],HEX);
  s += String(mac[4],HEX);
  s += String(mac[5],HEX);
  s.toCharArray(name, 32);
}

void setup(void)
{
  set_name();
  Serial.begin(115200);
  DEBUG("[main] Starting %s\n", name);
  EEPROM.begin(sizeof(state));
  loadState();
  hue = new Hue("192.168.1.149", state.hue_username);

  pinMode(pin_s0_en, OUTPUT);
  pinMode(pin_s1_en, OUTPUT); 
  pinMode(pin_s2_en, OUTPUT);

  pinMode(pin_led_0, OUTPUT);
  pinMode(pin_led_1, OUTPUT); 
  pinMode(pin_led_2, OUTPUT);

  pinMode(pin_button_0, INPUT_PULLUP); 
  pinMode(pin_button_1, INPUT_PULLUP);
  pinMode(pin_button_2, INPUT_PULLUP);

  digitalWrite(pin_s0_en, 0);
  digitalWrite(pin_s1_en, 0);
  digitalWrite(pin_s2_en, 0);

  server.on("/data", handleData);
  load_http_content(server);
  server.begin();
}

void connect_wifi() {
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

bool last_connected = false;
long last_color_update = millis();
int light = 0;
void loop(void)
{
  connect_wifi();
  MDNS.update();

  server.handleClient();

  long now = millis();

  if(hue->is_connected() && !last_connected) {
    hue->get_username().toCharArray(state.hue_username, 40);
    saveState();
  }
  last_connected = hue->is_connected();

  if(!digitalRead(pin_button_0)) light = 0;
  if(!digitalRead(pin_button_1)) light = 1;
  if(!digitalRead(pin_button_2)) light = 2;
  digitalWrite(pin_led_0, !(light == 0));
  digitalWrite(pin_led_1, !(light == 1));
  digitalWrite(pin_led_2, !(light == 2));

  if(last_color_update + 100 < millis()) {
    digitalWrite(pin_s0_en, 1);
    delay(1);
    int s0 = analogRead(A0);
    Serial.print(", ");
    digitalWrite(pin_s0_en, 0);
    digitalWrite(pin_s1_en, 1);
    delay(1);
    int s1 = analogRead(A0);
    Serial.print(", ");
    digitalWrite(pin_s1_en, 0);
    digitalWrite(pin_s2_en, 1);
    delay(1);
    int s2 = analogRead(A0);
    digitalWrite(pin_s2_en, 0);

    DEBUG("[io] Got %d, %d, %d\n", s0, s1, s2);
    char req[100];

    hue->request(("/lights/"+String(light)+
    "/state").c_str(), "{\"on\":true,\"hue\":"+
    String(map(s0, 0, 1024, 0, 65535))+
    ",\"sat\":"+
    String(map(s1, 0, 1024, 0, 254))+
    ",\"bri\":"+
    String(map(s2, 0, 1024, 0, 254))+
    ",\"transitiontime\":0}");
    last_color_update = millis();
  }
}
