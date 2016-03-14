#include <ESP8266HTTPClient.h>

class Hue
{
private:
  String bridge_address;
  String username;
  bool connected;
  HTTPClient  client;
public: 
  Hue(String bridge_address, String username);
  void set_username(String username);
  String get_username();
  boolean is_connected();
  void begin();
  boolean connect();
  boolean request(char* url, String content);
  String get_status();
};

