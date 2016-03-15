#include <WString.h>
#include <ESP8266WiFi.h>
#include "hue.h"

#ifndef DEBUG_ESP_PORT
#define DEBUG_ESP_PORT Serial
#endif

#define DEBUG(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )

Hue::Hue(String bridgeAddress, String username) {
  this->bridgeAddress = bridgeAddress;
  this->setUsername(username);
  if(this->connected) DEBUG("[hue] Using saved username '%s'\n", this->username.c_str());
}

boolean Hue::connect() {
  if (!connected) {
    DEBUG("[hue] Connecting...\n");
    client.begin(bridgeAddress, 80, "/api");
    client.POST("{\"devicetype\": \"arbitos\"}");
    WiFiClient* resp = client.getStreamPtr();
    while (resp->available()) {
      if (resp->find("success")) {
        resp->find("username");
        resp->find(":");
        resp->find("\"");
        username = resp->readStringUntil('\"');
        DEBUG("[hue] Connected with username '%s'\n", this->username.c_str());
        this->connected = true;
      }
    }
  }
  client.end();
  if(!this->connected) DEBUG("[hue] Failed to connect\n");
  return this->connected;
}

boolean Hue::request(const char* url, String content) {
  if (!this->connected) { if(!connect()) return false; }

  DEBUG("[hue] Sending command '%s' to '%s'\n", content.c_str(), url);
  client.begin(bridgeAddress, 80, "/api/" + username + url);
  int code = client.sendRequest("PUT", (uint8_t *) content.c_str(), content.length());
  if(code != HTTP_CODE_OK && code > 0) {
    DEBUG("[hue] Received code %d\n", code);
    this->connected = false;
  }
  if(code <= 0) {
    DEBUG("[hue] Unknown error during request\n");
  }
  client.end();
}

String Hue::getUsername() {
  return username;
}

void Hue::setUsername(String username) {
  this->username = username;
  if(username[0]) this->connected = true;
}

boolean Hue::isConnected() {
  return connected;
}

String Hue::getStatus() {
  String s;
  s += "{\"connected\":";
  s += connected?"true":"false";
  s += ",\"username\":\"";
  s += username;
  s += "\"}";
  return s;
}
