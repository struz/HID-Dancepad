#pragma once

struct Panel
{
    int pin;
    int pressPressure;
    int releasePressure;
    char scanCode;
    char arrowName;
    unsigned long timeSincePress;
    bool pressed;
  public:
    Panel(int pin, int pressPressure, int releaseDelta, char scanCode, char arrowName)
    {
      this->pin = pin;
      this->pressPressure = pressPressure;
      this->releasePressure = pressPressure + releaseDelta;
      this->scanCode = scanCode;
      this->arrowName = arrowName;
      this->timeSincePress = 0;
      this->pressed = false;
    }
};
