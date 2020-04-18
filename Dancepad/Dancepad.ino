#include <Keyboard.h>
#include "panel.h"

//Panel::Panel(int pin, int pressPressure, int releaseDelta, char scanCode)
Panel panels[4]
{
  Panel(A0, 275-25, 20, 'a'), //left
  Panel(A1, 320-25, 40, 's'), //down
  Panel(A2, 200-25, 15, 'w'), //up
  Panel(A3, 275-25, 16, 'd')  //right
};

void setup(void)
{
  Keyboard.begin();
}

void loop(void)
{
  for (int i = 0; i < 4; i++)
  {
    //panel reference
    Panel &panel = panels[i];
    //prnt(i, analogRead(panel.pin), 0);
    unsigned long mtime = millis();
    if ((mtime - panel.timeSincePress) > 20)
    {
      int pressure = analogRead(panel.pin);
      if (!panel.pressed && pressure > panel.pressPressure)
      {
        panel.timeSincePress = mtime;
        panel.pressed = true;
        Keyboard.press(panel.scanCode);
      }
      else if (panel.pressed && pressure < panel.releasePressure)
      {
        panel.timeSincePress = mtime;
        panel.pressed = false;
        Keyboard.release(panel.scanCode);
      }
    }
  }
}
