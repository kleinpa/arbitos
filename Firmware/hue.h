#include <ESP8266HTTPClient.h>

class Hue
{
private:
  String bridgeAddress;
  String username;
  bool connected;
  HTTPClient  client;
public:
  Hue(String bridgeAddress, String username);
  void setUsername(String username);
  String getUsername();
  boolean isConnected();
  void begin();
  boolean connect();
  boolean request(const char* url, String content);
  String getStatus();
  boolean setOff(String url);
  boolean setRGB(String url, double r, double g, double b);
};
