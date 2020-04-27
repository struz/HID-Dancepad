#include <Keyboard.h>
#include "panel.h"

#define NUMPAD_4 228
#define NUMPAD_2 226
#define NUMPAD_8 232
#define NUMPAD_6 230

#define NUM_PANELS 4
#define DEBOUNCE_THRESHOLD 32

#define LED_PIN_NUM 13

//Panel::Panel(int pin, int pressPressure, int releaseDelta, char scanCode)
// Lower pressure values = lower sensitivity, but faster hold release
//Panel panels[4]
//{
//  Panel(A0, 95, 20, NUMPAD_4, 'L'),
//  Panel(A1, 75, 20, NUMPAD_2, 'D'),
//  Panel(A2, 120, 0, NUMPAD_8, 'U'),
//  Panel(A3, 102, 0, NUMPAD_6, 'R')
//};

Panel panels[4]
{
  Panel(A0, 125, 0, NUMPAD_4, 'L'),
  Panel(A1, 75, 20, NUMPAD_2, 'D'),
  Panel(A2, 120, 0, NUMPAD_8, 'U'),
  Panel(A3, 100, 0, NUMPAD_6, 'R')
};

// Debug level - 0 = off, 1 = tap/release info, 2 = raw sensor voltage
byte debugLevel = 2;
// Keyboard input enabled/disabled - disable for testing
bool keyboardEnabled = true;

// Used for debugging
bool held = false;
unsigned long millisActivated = 0;
unsigned long releasedMillis = 0;
unsigned int pressureThreshold = 260;
// Report debugLevel=2 results every X milliseconds
unsigned int debugReportThresholdMillis = 50;
unsigned long lastReportedDebugMillis;

void handleInput() {
  // For serial console use - small buffer because we're not expecting
  // any big commands. Can only support (SERIAL_BUF_LENGTH - 1) length comamnds
  // so that we have print-safe strings.
  #define SERIAL_BUF_LENGTH 32
  char serialBuf[SERIAL_BUF_LENGTH];
  
  if (Serial.available()){
    size_t bytesRead = Serial.readBytesUntil('\n', serialBuf, SERIAL_BUF_LENGTH);
    if (bytesRead >= SERIAL_BUF_LENGTH) {
      Serial.print("E:incoming command exceeded buffer size");
      return;
    }

    // null terminate for print safety
    serialBuf[bytesRead] = '\0';
    
//    Serial.print("Command received: " );
//    Serial.println(serialBuf);

    // Command handling
    if (bytesRead == 2 && serialBuf[0] == 'd') {
      // Change debug level - input == "d<level>"
      int tmpDebugLevel = serialBuf[1] - 48;
      switch (tmpDebugLevel) {
        case 0:
        case 1:
        case 2:
          debugLevel = tmpDebugLevel;
          Serial.print("M:debug level set to ");
          Serial.println(debugLevel, DEC);
          break;
        default:
          Serial.println("M:invalid debug level supplied");
      }
    } else if (bytesRead == 1 && serialBuf[0] == 'k') {
      // Toggle keyboard - input == "k"
      keyboardEnabled = !keyboardEnabled;
      // If we're turning off the keyboard, unpress every key that is currently potentially pressed
      if (!keyboardEnabled) {
        for (int i = 0; i < NUM_PANELS; i++) {
          Keyboard.release(panels[i].scanCode);
        }
      }
      Serial.print("M:keyboard input ");
      if (keyboardEnabled) {
        Serial.println("enabled");
      } else {
        Serial.println("disabled");
      }
    } else if (bytesRead == 2 && serialBuf[0] == 's' && serialBuf[1] == 'g') {
      // Sensor Get thresholds - input == "sg"
      sendSensorThresholds();
    } else if (bytesRead > 2 && serialBuf[0] == 's' && serialBuf[1] == 'u') {
      // Sensor Update thresholds - input == "su"
      updateSensorThresholds();
    } else {
      Serial.println("M:unknown command");
    }
  }
}

void updateSensorThresholds() {
  // TODO
}

void sendSensorThresholds() {
  // Return message is ST for Sensor Thresholds, followed by
  // the sensor press and release thresholds for each sensor
  // i.e. "ST 50,60 50,40 50,50 60,70"
  Serial.print("ST ");
  for (int i = 0; i < NUM_PANELS; i++) {
    Serial.print(panels[i].pressPressure, DEC);
    Serial.print(",");
    Serial.print(panels[i].releasePressure, DEC);
    if (i < NUM_PANELS - 1) {
      Serial.print(" ");
    }
  }
  Serial.print("\n");
}

void doKeyPress(Panel panel) {
  if (keyboardEnabled) {
    Keyboard.press(panel.scanCode);
  }
}

void doKeyRelease(Panel panel) {
  if (keyboardEnabled) {
    Keyboard.release(panel.scanCode);
  }
}

void printDebugPanelHold(Panel panel, int pressure) {
  if (debugLevel == 1) {
    Serial.print("M:===\nM:[");
    Serial.print(pressure, DEC);
    Serial.print("] ");
    Serial.print(panel.arrowName);
    Serial.println(" pressed\nM:===");
  }
}

void printDebugPanelRelease(Panel panel, int pressure, unsigned long mtime) {
  if (debugLevel == 1) {
    Serial.print("M:===\nM:[");
    Serial.print(pressure, DEC);
    Serial.print("] ");
    Serial.print(panel.arrowName);
    Serial.print(" released, held: ");
    Serial.print(mtime - panel.timeSincePress, DEC);
    Serial.print("\nM:===\n");
  }
}

// turnOnLED turns on the LED
void turnOnLED() {
  digitalWrite(LED_PIN_NUM, HIGH);
}

// checkTurnOffLED turns off the LED if no panels are pressed right now
void checkTurnOffLED() {
  bool turnOffLED = true;
  for (int i = 0; i < NUM_PANELS; i++) {
    if (panels[i].pressed) {
      turnOffLED = false;
      break;
    }
  }
  if (turnOffLED) {
    digitalWrite(LED_PIN_NUM, LOW);
  }
}

void setup(void)
{
  // Setup code, runs once
  // Required for keyboard emulation
  Keyboard.begin();
  // Initialize the LED digital pin as output
  pinMode(LED_PIN_NUM, OUTPUT);
  // Use a pull up resistor for our analog pins
  pinMode(A0, INPUT_PULLUP);
  pinMode(A1, INPUT_PULLUP);
  pinMode(A0, INPUT_PULLUP);
  pinMode(A0, INPUT_PULLUP);
  // Setup serial for debugging purposes
  Serial.begin(9600);
  while (! Serial); // Wait untilSerial is ready - Leonardo
  Serial.println("M:setup complete");
}

void loop(void)
{
  // In order of LDUR, like the game
  int LDURrawPanelVoltages[4];
  for (int i = 0; i < NUM_PANELS; i++) {
    Panel &panel = panels[i];
    unsigned long mtime = millis();
    int pressure = analogRead(panel.pin);
    LDURrawPanelVoltages[i] = pressure;

    // timeSincePress is simple debouncing
//    if ((mtime - panel.timeSincePress) > DEBOUNCE_THRESHOLD) {
      if (!panel.pressed && pressure < panel.pressPressure) {
        printDebugPanelHold(panel, pressure);
        panel.timeSincePress = mtime;
        panel.pressed = true;
        doKeyPress(panel);
        turnOnLED();
      } else if (panel.pressed && pressure > panel.releasePressure) {
        printDebugPanelRelease(panel, pressure, mtime);
        panel.timeSincePress = mtime;
        panel.pressed = false;
        doKeyRelease(panel);

        // If no more panels are pressed, turn off the LED
        checkTurnOffLED();
      }
//    }
  }

  if (debugLevel == 2) {
    unsigned long reportMillis = millis();
    if ((reportMillis - lastReportedDebugMillis) > debugReportThresholdMillis) {
      // Debug message format: "SD <report millis> <Lv> <Dv> <Uv> <Rv>
      // where Lv is the Left sensor voltage, etc
      Serial.print("SD "); // SD for Sensor Data message, as opposed to "M:" for info message
      Serial.print(reportMillis);
      Serial.print(" ");
      for (int i = 0; i < NUM_PANELS; i++) {
        Serial.print(LDURrawPanelVoltages[i]);
        if (i < NUM_PANELS - 1) {
          Serial.print(" ");
        }
      }
      Serial.print("\n");
      lastReportedDebugMillis = reportMillis;
    }
  }
  // Process commands received via serial console
  handleInput();
}
