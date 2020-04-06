#pragma once

struct Panel
{
    int pin;
    int pressPressure;
    int releasePressure;
    char scanCode;
  public:
    Panel(int pin, int pressPressure, int releaseDelta, char scanCode)
    {
      this->pin = pin;
      this->pressPressure = pressPressure;
      this->releasePressure = pressPressure - releaseDelta;
      this->scanCode = scanCode;
    }
};
