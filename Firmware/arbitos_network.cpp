#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "private.h"
#include "arbitos_network.h"

#ifndef DEBUG_ESP_PORT
#define DEBUG_ESP_PORT Serial
#endif

#define DEBUG(...) DEBUG_ESP_PORT.printf( "[network] " __VA_ARGS__ )

const int TIMEOUT_NETWORK = 5000;

String generateHostName(String applicationName) {
  byte mac[6];
  WiFi.macAddress(mac);
  return applicationName + "-" +
    String(mac[3], HEX)+
    String(mac[4], HEX)+
    String(mac[5], HEX);
}

void ArbitosNetwork::setupMDNS()
{
  // Call MDNS.begin(<domain>) to set up mDNS to point to
  // "<domain>.local"
  if (!MDNS.begin(this->hostName.c_str()))
  {
    DEBUG("[mdns] Error setting up MDNS responder\n");
    while(1) {
      delay(1000);
    }
  }

  MDNS.addService(applicationName, "tcp", 80);
  DEBUG("[mdns] mDNS responder started\n");
}

void ArbitosNetwork::connectWifi() {
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
    DEBUG("Connected %d.%d.%d.%d\n", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
    setupMDNS();
  }
}

void ArbitosNetwork::begin(String applicationName){
  this->applicationName = applicationName;
  this->hostName = generateHostName(applicationName);
  DEBUG("Starting %s\n", hostName.c_str());

  ArduinoOTA.onStart([]() {
    DEBUG("[ota] Start\n");
  });
  ArduinoOTA.onEnd([]() {
    DEBUG("[ota] End\n");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    DEBUG("[ota] Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    DEBUG("[ota] Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) DEBUG("[ota] Auth Failed");
    else if (error == OTA_BEGIN_ERROR) DEBUG("[ota] Begin Failed");
    else if (error == OTA_CONNECT_ERROR) DEBUG("[ota] Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) DEBUG("[ota] Receive Failed");
    else if (error == OTA_END_ERROR) DEBUG("[ota] End Failed");
  });
  ArduinoOTA.setHostname(this->hostName.c_str());
  //ArduinoOTA.setPort(9090);
  ArduinoOTA.begin();
}

void ArbitosNetwork::update(){
  connectWifi();
  MDNS.update();
  ArduinoOTA.handle();
}

