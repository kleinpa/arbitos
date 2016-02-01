#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include "private.h"
#include "http_content.h"
#include "hue.h"

/* Values set in "private.h" */
// const char* ssid = "home";
// const char* password = "P4aaw0rd55";

const int TIMEOUT_NETWORK = 5000;
const int TIMEOUT_HUE = 5000;

const int PIN_BUTTON = 2;
const int PIN_LED = 1;

// Persistent State
int STATE_VERSION = 0;
struct State {
  char id[16];
  int version;
  char hue_username[40];
} state;

ESP8266WebServer server(80);
Hue* hue;

void setup(void)
{
  EEPROM.begin(sizeof(state));
  loadState();
  hue = new Hue("192.168.1.146", state.hue_username);


  pinMode(PIN_BUTTON, INPUT_PULLUP);
  server.on("/data", handleData);
  load_http_content(server);
  server.begin();
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

void connect_wifi() {
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long start = millis();
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(50);
      if (millis() - start > TIMEOUT_NETWORK) {
        //Serial.println("Timeout connecting to network.");
        return;
      }
    }
    //Serial.println(WiFi.localIP());
  }
}

long next_check_hue = millis();
bool toggle = false;
bool last_button = false;
void loop(void)
{
  connect_wifi();
  
  server.handleClient();

  long now = millis();
  
  bool button = !digitalRead(PIN_BUTTON);
  if (button && !last_button) {
    next_check_hue = now + TIMEOUT_HUE;
    if(!hue->is_connected()) {
      hue->get_username().toCharArray(state.hue_username, 40);
      saveState();
    }
    toggle = !toggle;
    if(toggle){
      hue->request("/groups/1/action", "{\"on\":true,\"bri\":254,\"transitiontime\":0}");
    } else {
      hue->request("/groups/1/action", "{\"on\":false,\"transitiontime\":0}");
    }
  }
  last_button = button;
}

char STATE_ID[16] = {0x32, 0x07, 0x0b, 0x4f, 0x08, 0x3c, 0x42, 0x25, 0xa6, 0x09, 0x0c, 0x35, 0x97, 0x92, 0x16, 0x9c};

void loadState() {
  EEPROM.get(0, state);
  if (!(//memcmp(STATE_ID, state.id, 16) &&
      STATE_VERSION == state.version)) {
    // Fails ID check, reinitialize
    memcpy(state.id, STATE_ID, 16);
    state.version = STATE_VERSION;
    state.hue_username[0] = '\0';
    saveState();
  }
}

void saveState() {
  EEPROM.put(0, state);
  EEPROM.commit();
}
