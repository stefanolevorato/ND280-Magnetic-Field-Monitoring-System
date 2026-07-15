# ND280 NodeOS v0.4.0-alpha4 - LED Power-On Self-Test

Use the `MicrochipStudio_Flat` directory for the working Microchip Studio project.

## New boot sequence

At every reset or power-up:

1. PD5 ON for 120 ms.
2. PD6 ON for 120 ms.
3. PD7 ON for 120 ms.
4. PD5 + PD6 + PD7 ON together for 250 ms.
5. All LEDs OFF, then normal NodeOS initialization begins.

The sequence tests the three LED circuits only. It does not encode TMAG channel count,
TCA9548A state or future OLED information.

## Runtime meanings

- PD5: unchanged heartbeat, toggled after each published measurement.
- PD6: sensor status.
- PD7: calibration/configuration status.

## Import into Microchip Studio

Add all `.c` and `.h` files from `MicrochipStudio_Flat` to an ATmega328PB GCC C project.
Clean, rebuild and flash.
