# HID-Dancepad (Struz fork)

This is some code for an Arduino Microcontroller that allows it to act as a controller for a 4 panel dance pad.

It has some cool features, and is designed to work with a [client/server package called velopad](https://github.com/struz/velopad).
- real time graphs to troubleshoot sensor inputs and activation points.
  - graphs can be paused to directly compare sensor input using the tooltip.
- update sensor thresholds without recompiling and uploading a new sketch to the microcontroller.
- support for triggering sensors on either a low or a high pin, depending on the circuit (must be configured in the code - see below).

## How to configure it

You will likely need to change some settings to make this suit your circuit / preferences.

Ctrl+F through the code for EDITME and follow the instructions.

Here's a brief overview of what needs changing:
- change gCircuitMode depending on whether your stepping on your panel supplies voltage to the pin, or connects the pin to ground.
- change the panels array parameters to suit your Arduino pin setup, pressure needs, and StepMania keybinds.

## How to use it with velopad

Upload this code to your Arduino and keep it plugged in to your PC via USB. Then follow the instructions inside [velopad](https://github.com/struz/velopad).

Note that while the velopad server process is running, you will be unable to upload code to the Arduino. Close the server process in order to upload new code.