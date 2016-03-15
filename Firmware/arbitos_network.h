class ArbitosNetwork {
private:
  String applicationName;
  String hostName;
  void setupMDNS();
  void connectWifi();
public:
  void begin(String applicationName);
  void update();
};
