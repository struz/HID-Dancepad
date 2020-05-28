#include <Keyboard.h>
#include "panel.h"

#define NUMPAD_4 228
#define NUMPAD_2 226
#define NUMPAD_8 232
#define NUMPAD_6 230

#define NUM_PANELS 4
#define DEBOUNCE_THRESHOLD 32

#define LED_PIN_NUM 13

#define PRESS_LOW 0x0
#define PRESS_HIGH 0x1

// === CONFIGURATION SETTINGS - CHANGE THESE ===

// Panel::Panel(int pin, int pressPressure, int releaseDelta, char scanCode, char arrowName)
// EDITME: edit the pins here to match what is plugged into your microcontroller.
// Optionally edit the 4th argument to supply a different keycode to press when pressing that
// panel down.
// Optionally edit the 2nd argument to change the default sensitivity for each panel.
Panel panels[4]
{
  Panel(A0, 750, 0, NUMPAD_4, 'L'),
  Panel(A1, 550, 0, NUMPAD_2, 'D'),
  Panel(A3, 900, 0, NUMPAD_8, 'U'),
  Panel(A4, 750, 0, NUMPAD_6, 'R')
};

// EDITME:
// - use PRESS_LOW if pressing down a panel supplies voltage to your pin.
// - use PRESS_HIGH if pressing down a panel connects your pin to ground.
byte gCircuitMode = PRESS_LOW;

// ================ END CHANGES ================

// Debug level - 0 = off, 1 = tap/release info, 2 = raw sensor voltage
byte gDebugLevel = 2;
// Keyboard input enabled/disabled - disable for testing
bool gKeyboardEnabled = true;

// Report gDebugLevel=2 results every X milliseconds
unsigned int gDebugReportThresholdMillis = 50;
unsigned long gLastReportedDebugMillis;

void handleInput() {
  // For serial console use - small buffer because we're not expecting
  // any big commands.
  #define SERIAL_BUF_LENGTH 64
  char serialBuf[SERIAL_BUF_LENGTH];
  
  if (Serial.available()) {
    size_t bytesRead = Serial.readBytesUntil('\n', serialBuf, SERIAL_BUF_LENGTH);
    if (bytesRead >= SERIAL_BUF_LENGTH) {
      // If bytesRead is equal to the length then if we null terminate below, we 
      // will clobber a meaningful character. Thus the command exceeds size at
      // SERIAL_BUF_LENGTH rather than > SERIAL_BUF_LENGTH.
      Serial.print("E: incoming command exceeded buffer size");
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
          gDebugLevel = tmpDebugLevel;
          Serial.print("M: debug level set to ");
          Serial.println(gDebugLevel, DEC);
          break;
        default:
          Serial.println("M: invalid debug level supplied");
      }
    } else if (bytesRead == 1 && serialBuf[0] == 'k') {
      // Toggle keyboard - input == "k"
      gKeyboardEnabled = !gKeyboardEnabled;
      // If we're turning off the keyboard, unpress every key that is currently potentially pressed
      if (!gKeyboardEnabled) {
        for (int i = 0; i < NUM_PANELS; i++) {
          Keyboard.release(panels[i].scanCode);
        }
      }
      Serial.print("M: keyboard input ");
      if (gKeyboardEnabled) {
        Serial.println("enabled");
      } else {
        Serial.println("disabled");
      }
    } else if (bytesRead == 2 && serialBuf[0] == 's' && serialBuf[1] == 'g') {
      // Sensor Get thresholds - input == "sg"
      sendSensorThresholds();
    } else if (bytesRead > 2 && serialBuf[0] == 's' && serialBuf[1] == 'u') {
      // Sensor Update thresholds - input ==
      // "su <Lval>,<Lval> <Dval>,<Dval>, <Uval>,<Uval>, <Rval>,<Rval>"
      updateSensorThresholds(serialBuf + 3, bytesRead - 3);
    } else {
      Serial.println("M: unknown command");
    }
  }
}

// NOTE: this function will briefly modify the sensorInput string - make sure
// it's not in read-only memory, i.e. do not pass this a constant string.
void updateSensorThresholds(char* sensorInput, size_t len) {
  // Parse the sensor data given an input string and its length
  size_t i = 0, sensorIndex = 0, thresholdIndex = 0;
  int pressures[NUM_PANELS][2];
  memset(pressures, 0, sizeof(pressures));
  
  while (i < len) {
    // Safeguards to avoid overflows
    if (sensorIndex >= NUM_PANELS || thresholdIndex > 1) {
      break;
    }

    switch (sensorInput[i]) {
      case ' ': // Sensor SEP char - moving on to next sensor
        sensorIndex++;
        thresholdIndex = 0;
        break;
      case ',': // Threshold SEP char - moving on to next threshold
        thresholdIndex++;
        break;
      default: // Assume we're dealing with a numeric character, no error checking for simplicity
        // Look ahead to find the next separator so we can convert a number
        char nextSep = (thresholdIndex == 0) ? ',' : ' ';
        char numLength = 0;
        for (int j = i; j < len; j++) {
          if (sensorInput[j] == nextSep) {
            // Found the length of our number. Can also find it
            // by running out of iteration numbers in the for loop.
            break;
          }
          numLength++;
        }
        // Do some hacky stuff to use atoi on our non null terminated string
        char originalChar = sensorInput[i + numLength];
        sensorInput[i + numLength] = '\0';
        pressures[sensorIndex][thresholdIndex] = atoi(sensorInput + i);
        sensorInput[i + numLength] = originalChar;
        
        // Move ahead to the separator to process it next
        i += numLength;
        continue;
    }
    i++;
  }
  
  // Return a message to echo the changes as we make them
  Serial.print("SU ");
  for (int i = 0; i < NUM_PANELS; i++) {
    panels[i].pressPressure = pressures[i][0];
    panels[i].releasePressure = pressures[i][1];
    Serial.print(pressures[i][0], DEC);
    Serial.print(",");
    Serial.print(pressures[i][1], DEC);
    if (i < NUM_PANELS - 1) {
      Serial.print(" ");
    }
  }
  Serial.print("\n");
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
  if (gKeyboardEnabled) {
    Keyboard.press(panel.scanCode);
  }
}

void doKeyRelease(Panel panel) {
  if (gKeyboardEnabled) {
    Keyboard.release(panel.scanCode);
  }
}

void printDebugPanelHold(Panel panel, int pressure) {
  if (gDebugLevel == 1) {
    Serial.print("M: ===\nM: [");
    Serial.print(pressure, DEC);
    Serial.print("] ");
    Serial.print(panel.arrowName);
    Serial.println(" pressed\nM: ===");
  }
}

void printDebugPanelRelease(Panel panel, int pressure, unsigned long mtime) {
  if (gDebugLevel == 1) {
    Serial.print("M: ===\nM: [");
    Serial.print(pressure, DEC);
    Serial.print("] ");
    Serial.print(panel.arrowName);
    Serial.print(" released, held: ");
    Serial.print(mtime - panel.timeSincePress, DEC);
    Serial.print("\nM: ===\n");
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

// sensorIsPressed returns true if a sensor is currently pressed, taking into account
// gCircuitMode to work out whether it is pressed when voltage is supplied or removed.
bool sensorIsPressed(int sensorValue, int sensorThreshold) {
  return gCircuitMode == PRESS_LOW ? sensorValue <= sensorThreshold : sensorValue >= sensorThreshold;
}

void setup(void)
{
  // Setup code, runs once
  // Required for keyboard emulation
  Keyboard.begin();
  // Initialize the LED digital pin as output
  pinMode(LED_PIN_NUM, OUTPUT);
  // Initialise our input pins based on the circuit mode.
  for (int i = 0; i < NUM_PANELS; i++) {
    if (gCircuitMode == PRESS_LOW) {
      // The pin outputs current, which is dragged down to ground by stepping on the sensor. The more pressure, the
      // lower the voltage.
      pinMode(panels[i].pin, OUTPUT);
      digitalWrite(panels[i].pin, HIGH);
    } else {
      // Voltage is supplied to the pin by stepping on the sensor.
      pinMode(panels[i].pin, INPUT);
    }
  }
  // Setup serial for debugging purposes
  Serial.begin(9600);
  while (! Serial); // Wait untilSerial is ready - Leonardo
  Serial.println("M: setup complete");
}

void loop(void)
{
  // In order of LDUR, like the game
  int LDURrawPanelVoltages[4];
  // Used to emit a debug message on state change when debug level is appropriate
  bool anyPanelPressedStateChanged = false;
  for (int i = 0; i < NUM_PANELS; i++) {
    Panel &panel = panels[i];
    unsigned long mtime = millis();
    int pressure = analogRead(panel.pin);
    LDURrawPanelVoltages[i] = pressure;

    // timeSincePress is simple debouncing
//    if ((mtime - panel.timeSincePress) > DEBOUNCE_THRESHOLD) {
      if (!panel.pressed && sensorIsPressed(pressure, panel.pressPressure)) {
        printDebugPanelHold(panel, pressure);
        panel.timeSincePress = mtime;
        panel.pressed = true;
        doKeyPress(panel);
        turnOnLED();
        anyPanelPressedStateChanged = true;
      } else if (panel.pressed && !sensorIsPressed(pressure, panel.releasePressure)) {
        printDebugPanelRelease(panel, pressure, mtime);
        panel.timeSincePress = mtime;
        panel.pressed = false;
        doKeyRelease(panel);

        // If no more panels are pressed, turn off the LED
        checkTurnOffLED();
        anyPanelPressedStateChanged = true;
      }
//    }
  }

  if (gDebugLevel == 2) {
    unsigned long reportMillis = millis();
    // If we reached the threshold for another update, or if we had a sensor state change
    if ((reportMillis - gLastReportedDebugMillis) > gDebugReportThresholdMillis || anyPanelPressedStateChanged) {
      // Debug message format: "SD <report millis> <Lv>,<Ps> <Dv>,<Ps> <Uv>,<Ps> <Rv>,<Ps>
      // where Lv is the Left sensor voltage (etc) and <Ps> is the pressed state of the panel as T=true, F=false
      Serial.print("SD "); // SD for Sensor Data message, as opposed to "M: " for info message
      Serial.print(reportMillis);
      Serial.print(" ");
      for (int i = 0; i < NUM_PANELS; i++) {
        Serial.print(LDURrawPanelVoltages[i]);
        Serial.print(",");
        
        if (panels[i].pressed) {
          Serial.print("T");
        } else {
          Serial.print("F");
        }
        
        if (i < NUM_PANELS - 1) {
          Serial.print(" ");
        }
      }
      Serial.print("\n");
      gLastReportedDebugMillis = reportMillis;
    }
  }
  // Process commands received via serial console
  handleInput();
}
