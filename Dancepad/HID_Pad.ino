#include <Keyboard.h>
#include "panel.h"

//Panel::Panel(int pin, int pressPressure, int releaseDelta, char scanCode)
Panel panels[4]
{
  Panel(A0, 300, 20, 'a'),
  Panel(A1, 370, 20, 's'),
  Panel(A2, 310, 20, 'w'),
  Panel(A3, 330, 20, 'd')
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
    int pressure = analogRead(panel.pin);
    
    if (pressure > panel.pressPressure)
    {
      Keyboard.press(panel.scanCode);
    }
    else if(pressure < panel.releasePressure)
    {
      Keyboard.release(panel.scanCode);
    }
  }
}
