#include <ESP8266WiFi.h>
#include "arbitos_hardware.h"

#ifndef DEBUG_ESP_PORT
#define DEBUG_ESP_PORT Serial
#endif

#define DEBUG(...) DEBUG_ESP_PORT.printf( "[hardware] " __VA_ARGS__ )

// Constants

// Minimum time to recognize button press for debouncing and long-press detection
const unsigned long buttonMinTime = 1000;
const unsigned long buttonLongMinTime = 600 * 1000;

const unsigned long sliderIgnoreChange = 20;
const unsigned long sliderRangeMax[] = {890,955,1020};
const unsigned long sliderRangeMin = 20;

// Pin configuration
const byte pinSliderEnable0 = 4;
const byte pinSliderEnable1 = 5;
const byte pinSliderEnable2 = 15;

const byte pinSliderRead = A0;

const byte pinLED0 = 12;
const byte pinLED1 = 14;
const byte pinLED2 = 16;

const byte pinButton0 = 13;
const byte pinButton1 = 0;
const byte pinButton2 = 2;

const byte pinButtons[] = {pinButton0, pinButton1, pinButton2};
const byte pinLEDs[] = {pinLED0, pinLED1, pinLED2};
const byte pinSliderEnables[] = {pinSliderEnable0, pinSliderEnable1, pinSliderEnable2};

int sliderValues[3];
int sliderMoving[3];
unsigned long sliderTimes[3];
bool buttonStates[3];
bool buttonLongPressed[3];
unsigned long buttonPressTimes[3];

void ArbitosHardware::begin(){
  for(int i = 0; i < sizeof(pinButtons); i++) {
    pinMode(pinButtons[i], INPUT_PULLUP);
  }

  for(int i = 0; i < sizeof(pinSliderEnables); i++) {
      pinMode(pinSliderEnables[i], OUTPUT);
  }

  for(int i = 0; i < sizeof(pinLEDs); i++) {
      pinMode(pinLEDs[i], OUTPUT);
      this->setLED(i, LOW);
  }
}


void ArbitosHardware::update(){
  // Read sliders
  for(int i = 0; i < sizeof(pinSliderEnables); i++) {
    for(int j = 0; j < sizeof(pinSliderEnables); j++) {
      digitalWrite(pinSliderEnables[j], i == j);
    }
    delayMicroseconds(500);
    int adjustedValue, value = analogRead(pinSliderRead);
    if(abs(value - sliderValues[i]) > sliderIgnoreChange) {
      adjustedValue = min(1024,max(0,map(value, sliderRangeMin, sliderRangeMax[i], 0, 1024)));
      DEBUG("Slider %d moved to raw value %d, corrected value%d'\n", i, value, adjustedValue);
      sliderMoveCallback(i, adjustedValue);
      sliderValues[i] = value;
      sliderTimes[i] = micros();
      sliderMoving[i] = true;
    }
    if(sliderTimes[i] + 200*1000 < micros() && sliderMoving[i]) {
      adjustedValue = min(1024,max(0,map(value, sliderRangeMin, sliderRangeMax[i], 0, 1024)));
      DEBUG("Slider %d stopped moving at raw value %d, corrected value %d'\n", i, value, adjustedValue);
      sliderMoveCallback(i, adjustedValue);
      sliderValues[i] = value;
      sliderMoving[i] = false;
    }
  }

  // Read buttons
  for(int i = 0; i < sizeof(pinButtons); i++) {
    int value = !digitalRead(pinButtons[i]);
    unsigned long duration = micros() - buttonPressTimes[i];
    if(buttonStates[i] != value) {
      buttonStates[i] = value;
      if(value) {
        buttonPressTimes[i] = micros();
        buttonLongPressed[i] = false;
      } else {
        unsigned long duration = micros() - buttonPressTimes[i];
        if(duration < buttonLongMinTime && duration >= buttonMinTime) {
          DEBUG("Button %d pressed\n", i); 
          buttonPressCallback(i); 
        }
      }
    } else if (value && duration >= buttonLongMinTime && !buttonLongPressed[i]) {
      DEBUG("Button %d long pressed\n", i);
      buttonLongPressCallback(i);
      buttonLongPressed[i] = true;
    }
  }
}

void ArbitosHardware::onButtonPress(void (*fptr)(int button)){
  buttonPressCallback = fptr;
}

void ArbitosHardware::onButtonLongPress(void (*fptr)(int button)){
  buttonLongPressCallback = fptr;
}

void ArbitosHardware::onSliderMove(void (*fptr)(int slider, int value)){
  sliderMoveCallback = fptr;
}

void ArbitosHardware::setLED(int led, int value){
  digitalWrite(pinLEDs[led], !value);
}

void ArbitosHardware::blinkLED(int led, int times){
  int originalValue = digitalRead(pinLEDs[led]);
  if(!originalValue){
    digitalWrite(pinLEDs[led], !LOW);
    delay(150);
  }
  for(int i = 0; i < times; i++) {
    digitalWrite(pinLEDs[led], !HIGH);
    delay(150);
    digitalWrite(pinLEDs[led], !LOW);
    delay(150);
  }
  delay(150);
  digitalWrite(pinLEDs[led], originalValue);
}
