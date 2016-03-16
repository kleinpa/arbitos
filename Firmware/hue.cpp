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

boolean Hue::setOff(String url) {
  return this->request(url.c_str(), "{\"on\":false,\"transitiontime\":1}");
}

void rgb2hsv(double r, double g, double b, double &h, double &s, double &v)
{
  // http://stackoverflow.com/a/6930407
  double      min, max, delta;

  min = r < g ? r : g;
  min = min < b ? min : b;

  max = r > g ? r : g;
  max = max > b ? max : b;

  v = max;                                // v
  delta = max - min;
  if (delta < 0.00001)
  {
    s = 0;
    h = 0; // undefined, maybe nan?
    return;
  }
  if( max > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
    s = (delta / max);                  // s
  } else {
    // if max is 0, then r = g = b = 0
    // s = 0, v is undefined
    s = 0.0; 
    h = NAN;                            // its now undefined
    return;
  }
  if( r >= max )                           // > is bogus, just keeps compilor happy
    h = ( g - b ) / delta;        // between yellow & magenta
  else
    if( g >= max )
      h = 2.0 + ( b - r ) / delta;  // between cyan & yellow
    else
      h = 4.0 + ( r - g ) / delta;  // between magenta & cyan

  h *= 60.0;                              // degrees

  if( h < 0.0 )
    h += 360.0;

  return;
}

boolean Hue::setRGB(String url, double r, double g, double b) {
  double h, s, v;
  rgb2hsv(r, g, b, h, s, v);
    
  return this->request(url.c_str(), "{\"on\":true,\"hue\":"+
    String(int(h/360.0*65535))+
    ",\"sat\":"+
    String(int(s/1.0*254))+
    ",\"bri\":"+
    String(int(v/1.0*254))+
    ",\"transitiontime\":1}");
}
