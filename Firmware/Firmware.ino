#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "private.h"
#include "http_content.h"

/* Values set in "private.h" */
// const char* ssid = "home";
// const char* password = "P4aaw0rd55";

const int TIMEOUT_NETWORK = 5000;
const int TIMEOUT_HUE = 5000;

const int PIN_PIR = 2;
const int PIN_LED = 1;

// Runtime State
bool motion = 0;
String hue_username = "";
String request = "";
String response = "";

// Persistent State
int STATE_VERSION = 0;
struct State {
  char id[16];
  int version;
  float setpoint;
  float epsilon;
} state;

ESP8266WebServer server(80);

void setup(void)
{
  //EEPROM.begin(sizeof(state));
  //loadState();

  //Serial.begin(115200);
  //Serial.println();

  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_LED, OUTPUT);

  server.on("/data", handleData);
  load_http_content(server);
  server.begin();
}

void handleData() {
  if (server.method() == HTTP_GET) {
    String url = "{";
    url += "\"motion\":";
    url += motion;
    url += ",\"hue_username\":\"";
    url += hue_username;
    url += "\",\"request\":\"";
    url += request;
    url += "\",\"response\":\"";
    url += response;
    url += "\"}";
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
        Serial.println("Timeout connecting to network.");
        return;
      }
    }
    Serial.println(WiFi.localIP());

  }
}

void connect_hue() {
  if(!hue_username.length()) {
    WiFiClient client;
    String host = "192.168.1.146";
    const int httpPort = 80;  
    if (!client.connect("192.168.1.146", httpPort)) {
      Serial.println("connection failed");
      return;
    }
    
    delay(10);
    client.print(
      "POST /api HTTP/1.1\r\nHost: " + host + "\r\n" +
      "Content-Length: 25\r\n\r\n{\"devicetype\": \"esp_hue\"}");
    while (client.available()) {
      if(client.find("success")){
        client.find("username");
        client.find(":");
        client.find("\"");
        hue_username = client.readStringUntil('\"');  
      }
    }
  }
}

void hue_request(char* m, char* url, String content) {
  response = "1";
    WiFiClient client;
    String host = "192.168.1.146";
    const int httpPort = 80;  
    if (!client.connect("192.168.1.146", httpPort)) {
      Serial.println("connection failed");
      return;
    }
    
    delay(10);
    request = String(m) + " /api/"+hue_username+url+" HTTP/1.1\r\nHost: " + host + "\r\n" +
      "Content-Length: "+content.length()+"\r\n\r\n"+content;
    client.print(request);
      response = "2";
    while (client.available()) {
        response = client.readString();  
    }
}

long next_check_hue = millis();
void loop(void)
{
  connect_wifi();
  server.handleClient();
  bool last_motion = motion;
  motion = digitalRead(PIN_PIR);
  digitalWrite(PIN_LED, !motion);


  
  if(last_motion != motion || millis() > next_check_hue) {
    next_check_hue = millis() + TIMEOUT_HUE;
    connect_hue();
    if(motion){
      hue_request("PUT", "/lights/1/state","{\"on\":true,\"bri\":255}");
    } else {
      hue_request("PUT", "/lights/1/state","{\"on\":false}");
    }
  }
}

/*char STATE_ID[16] = {0x32, 0x07, 0x0b, 0x4f, 0x08, 0x3c, 0x42, 0x25, 0xa6, 0x09, 0x0c, 0x35, 0x97, 0x92, 0x16, 0x9c};

void loadState() {

  EEPROM.get(0, state);
  if (!(//memcmp(STATE_ID, state.id, 16) && 
      STATE_VERSION == state.version)) {
    // Fails ID check, reinitialize
    memcpy(state.id, STATE_ID, 16);
    state.version = STATE_VERSION;
    state.setpoint = 8;
    state.epsilon = 0.2;
    saveState();
  }
}

void saveState() {
  EEPROM.put(0, state);
  EEPROM.commit();
}*/
