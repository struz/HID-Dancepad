#include <Keyboard.h>
#include "panel.h"

//Panel::Panel(int pin, int pressPressure, int releaseDelta, char scanCode)
Panel panels[4]
{
  Panel(A2, 300, 20, '4'),
  Panel(A1, 370, 20, '2'),
  Panel(A0, 180, 20, '8'),
  Panel(A3, 330, 20, '6')
};

// Debug level - 0 = off, 1 = tap/release info, 2 = raw sensor voltage
byte debugLevel = 1;
// Keyboard input enabled/disabled - disable for testing
bool keyboardEnabled = false;

// Used for debugging
bool held = false;
unsigned long millisActivated = 0;
unsigned long releasedMillis = 0;
unsigned int pressureThreshold = 260;
// Report debugLevel=2 results every X milliseconds
unsigned int debugReportThresholdMillis = 50;
unsigned long lastReportedDebugMillis;

void handleInput() {
  if (Serial.available()){
    String input = Serial.readString();
    input.trim();
    Serial.print("Command received: " );
    Serial.println(input);

    // Command handling
    // debug <level>
    if (input.length() == 7 && input.startsWith("debug")) {
      int tmpDebugLevel = input.charAt(6) - 48;
      switch (tmpDebugLevel) {
        case 0:
        case 1:
        case 2:
          debugLevel = tmpDebugLevel;
          Serial.print("debug level set to ");
          Serial.println(debugLevel, DEC);
          break;
        default:
          Serial.println("Invalid debug level supplied");
      }
      // TODO: keyboard on/off, remember to release all keys that are curerntly held when turning off!!
    } else {
      Serial.println("Unknown command");
    }
  }
}

void doKeyPress(char key) {
  if (keyboardEnabled) {
    Keyboard.press(key);
  }
}

void doKeyRelease(char key) {
  if (keyboardEnabled) {
    Keyboard.release(key);
  }
}

void setup(void)
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (! Serial); // Wait untilSerial is ready - Leonardo
  Keyboard.begin();
  Serial.println("Hello world");
}

void loop(void)
{
  // In order of LDUR, like the game
  int rawPanelVoltages[4];
  for (int i = 0; i < 4; i++) {
    Panel &panel = panels[i];
    unsigned long mtime = millis();
    int pressure = analogRead(panel.pin);
    rawPanelVoltages[i] = pressure;

    // timeSincePress is simple debouncing
    if ((mtime - panel.timeSincePress) > 20) {
      if (!panel.pressed && pressure < panel.pressPressure) {
        if (debugLevel >= 1) {
          Serial.print("[");
          Serial.print(pressure, DEC);
          Serial.print("] ");
          Serial.print(panel.scanCode);
          Serial.println(" pressed");
        }
        panel.timeSincePress = mtime;
        panel.pressed = true;
        doKeyPress(panel.scanCode);
      } else if (panel.pressed && pressure > panel.releasePressure) {
        if (debugLevel >= 1) {
          Serial.print("[");
          Serial.print(pressure, DEC);
          Serial.print("] ");
          Serial.print(panel.scanCode);
          Serial.print(" released, held: ");
          Serial.print(mtime - panel.timeSincePress, DEC);
          Serial.print("\n");
        }
        panel.timeSincePress = mtime;
        panel.pressed = false;
        doKeyRelease(panel.scanCode);
      }
    }
  }

  if (debugLevel >= 2) {
    if ((millis() - lastReportedDebugMillis) > debugReportThresholdMillis) {
      for (int i = 0; i < 4; i++) {
        Serial.print(rawPanelVoltages[i]);
        Serial.print(" ");
      }
      Serial.print("\n");
      lastReportedDebugMillis = millis();
    }
  }
  // Process serial commands
  handleInput();
}
