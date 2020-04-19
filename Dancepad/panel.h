#pragma once

struct Panel
{
    int pin;
    int pressPressure;
    int releasePressure;
    char scanCode;
    unsigned long timeSincePress;
    bool pressed;
  public:
    Panel(int pin, int pressPressure, int releaseDelta, char scanCode)
    {
      this->pin = pin;
      this->pressPressure = pressPressure;
      this->releasePressure = pressPressure + releaseDelta;
      this->scanCode = scanCode;
      this->timeSincePress = 0;
      this->pressed = false;
    }
};
