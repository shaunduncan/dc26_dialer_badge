# DC26 Rotary Dialer Badge
A badge for DC26 that uses an old Western Electric rotary pulse dial to drive lighting and sound effects.
The working model uses a raspberry pi zero w to drive a neopixel RGBW strip, 16x2 character display, and
two small 8Ohm/0.5W speakers.

## Build Requirements

Some external libraries are required

- wiringPi (http://wiringpi.com)
- rpi_ws281x (https://github.com/jgarff/rpi_ws281x)

## Building

Run `make`, which will produce a binary named `badge` in the `build/` directory. Note that building has only
been tested on a raspberry pi zero w.
