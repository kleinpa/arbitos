class ArbitosHardware
{
private:
  void (*buttonPressCallback)(int button);
  void (*buttonLongPressCallback)(int button);
  void (*sliderMoveCallback)(int slider, int value);
public:
  void begin();
  void update();
  bool button[3];
  int slider[3];
  void onButtonPress(void (*fptr)(int button));
  void onButtonLongPress(void (*fptr)(int button));
  void onSliderMove(void (*fptr)(int slider, int value));
  void setLED(int led, int value);
  void blinkLED(int led, int times);
};
