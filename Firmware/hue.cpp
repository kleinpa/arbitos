#include <WString.h>
#include <ESP8266WiFi.h>
#include "hue.h"


Hue::Hue(String bridge_address, String username) {
  this->bridge_address = bridge_address;
  this->username = username;
  connected = true;
}

boolean Hue::connect() {
  if (!connected) {
    client.begin(bridge_address, 80, "/api");
    client.POST("{\"devicetype\": \"esp_hue\"}");
    WiFiClient* resp = client.getStreamPtr();
    while (resp->available()) {
      if (resp->find("success")) {
        resp->find("username");
        resp->find(":");
        resp->find("\"");
        username = resp->readStringUntil('\"');
        connected = true;
      }
    }
  }
  client.end();
  return connected;
}

boolean Hue::request(char* url, String content) {
  if (!connected) { if(!connect()) return false; }
  
  client.begin(bridge_address, 80, "/api/" + username + url);
  httpresponse = "";
  httprequest = "/api/" + username + url + "|" + content;
  client.sendRequest("PUT", (uint8_t *) content.c_str(), content.length());
  httpresponse = client.getString();
  client.end();
}

String Hue::get_username() {
  return username;
}

void Hue::set_username(String username) {
  this->username = username;
}

boolean Hue::is_connected() {
  return connected;  
}
String Hue::get_status() {
  String s;
  s += "{\"connected\":";
  s += connected?"true":"false";
  s += ",\"username\":\"";
  s += username;
  s += "\",\"request\":\"";
  s += httprequest;
  s += "\",\" httpresponse\":\"";
  s += httpresponse;
  s += "\"}";
  return s;  
}

