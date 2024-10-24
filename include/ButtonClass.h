#ifndef BUTTON_CLASS_H
#define BUTTON_CLASS_H
#include "Arduino.h"

class Button {
  private:
    unsigned int debounceDelay;
    int pin;
  public:
    int buttonState;
    int lastButtonState;
    int pressStart;
    int pressEnd;
    unsigned long lastDebounceTime;

    Button(int pin) {
      this->pin = pin;
      this->buttonState = 0;
      this->lastButtonState = 1;
      this->lastDebounceTime = 0;
      this->pressStart = 0;
      this->pressEnd = 0;
      this->debounceDelay = 50;
    }

    int getPin() {
      return this->pin;
    }

    void read() {
      this->buttonState = digitalRead(this->pin);
      if (this->buttonState != this->lastButtonState) {
        this->lastDebounceTime = millis();
      }
      if ((millis() - this->lastDebounceTime) > this->debounceDelay) {
        if (this->buttonState == 0 && this->pressStart == 0) {
          this->pressStart = 1;
        }
        return;
      }
      if (this->pressStart == 1) {
        this->pressEnd = 1;
      }
      this->lastButtonState = this->buttonState;
    }
    int wasPressed() {
      if (this->pressStart == 1 and this->pressEnd == 1) {
        this->pressStart = 0;
        this->pressEnd = 0;
        return 1;
      }
      return 0;
    }
};

#endif
