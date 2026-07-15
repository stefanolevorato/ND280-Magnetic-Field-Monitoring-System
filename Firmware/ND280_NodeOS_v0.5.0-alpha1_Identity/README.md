# ND280 NodeOS v0.5.0-alpha1

Identity milestone based on the validated `v0.4.0-alpha4` single-sensor firmware.

## Microchip Studio

Use the `MicrochipStudio_Flat` directory. Add all `.c` and `.h` files to an
ATmega328PB GCC C project.

## New commands

```text
INFO
IDENTITY
```

Both return the complete node identity, including build date and time.

## LED POST

```text
POST_SINGLE_LED_MS = 240
POST_ALL_LEDS_MS   = 500
POST_GAP_MS        = 120
```
