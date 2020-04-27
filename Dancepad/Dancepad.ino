#include <Keyboard.h>
#include "panel.h"

#define NUMPAD_4 228
#define NUMPAD_2 226
#define NUMPAD_8 232
#define NUMPAD_6 230

#define NUM_PANELS 4
#define DEBOUNCE_THRESHOLD 32

#define LED_PIN_NUM 13

// ===== Serial Protocol =====
// The serial protocol used to communicate with a host here is fairly basic.
// I've moved away from ASCII just so we can transmit more data without
// worrying about baud rate, even though that's unlikely to become an issue.

// There is a command separation byte 0xFF (nbsp if rendered in ascii) which
// separates all commands to and from the device over serial.

// Command message formats are command specific and are documented in comments.
// They all begin with the byte 0xFE followed by an ascii code unique to the 
// command.
// ===========================

// TODO: the above won't work, we need control characters to say that the next input is an int and how long it is
// etc. Otherwise a specifically crafted int would look like a command in itself.

#define SERIAL_PROTOCOL_CMD_START_BYTE = 0xFE
#define SERIAL_PROTOCOL_CMD_END_BYTE = 0xFF

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

// Sends the bytes 
void sendSerialCommand(char cmdCode, cmdContentBytes byte[]) {
  // 1 char for command header, 1 char for footer
  unsigned int cmdLength = 2 + sizeof(cmdContentBytes);
  char cmdBytes = malloc(cmdLength);
  cmdBytes[0] = 
  Serial.write(cmdBytes);
  free(cmdBytes);
}

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
    } else if (input.length() == 8 && input.equals("keyboard")) {
      // Toggle keyboard
      keyboardEnabled = !keyboardEnabled;
      // If we're turning off the keyboard, unpress every key that is currently potentially pressed
      if (!keyboardEnabled) {
        for (int i = 0; i < NUM_PANELS; i++) {
          Keyboard.release(panels[i].scanCode);
        }
      }
      Serial.print("Keyboard input ");
      if (keyboardEnabled) {
        Serial.print("enabled\n");
      } else {
        Serial.print("disabled\n");
      }
    } else {
      Serial.println("Unknown command");
    }
  }
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
  if (debugLevel >= 1) {
    Serial.print("===\n[");
    Serial.print(pressure, DEC);
    Serial.print("] ");
    Serial.print(panel.arrowName);
    Serial.println(" pressed\n===");
  }
}

void printDebugPanelRelease(Panel panel, int pressure, unsigned long mtime) {
  if (debugLevel >= 1) {
    Serial.print("===\n[");
    Serial.print(pressure, DEC);
    Serial.print("] ");
    Serial.print(panel.arrowName);
    Serial.print(" released, held: ");
    Serial.print(mtime - panel.timeSincePress, DEC);
    Serial.print("\n===\n");
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
  Serial.println("Setup complete");
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

  if (debugLevel >= 2) {
    if ((millis() - lastReportedDebugMillis) > debugReportThresholdMillis) {
      Serial.print("SD:"); // SD for Sensor Data
      for (int i = 0; i < NUM_PANELS; i++) {
        Serial.print(LDURrawPanelVoltages[i]);
        Serial.print(" ");
      }
      Serial.print("\n");
      lastReportedDebugMillis = millis();
    }
  }
  // Process commands received via serial console
  handleInput();
}
