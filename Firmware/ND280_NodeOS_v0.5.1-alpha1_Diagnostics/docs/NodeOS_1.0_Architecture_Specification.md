# ND280 Magnetic Field Monitoring System

## NodeOS 1.0 Architecture Specification

**Status:** Draft for architecture freeze  
**Project:** ND280 magnetic-field monitoring at J-PARC  
**Reference node:** ATmega328PB + TMAG5273A2  
**Current validated magnetic range:** ±266 mT  
**Current development board:** single-sensor prototype  
**Planned module concept:** ATmega328PB + TCA9548A + up to 8 TMAG5273A2 sensors

---

## 1. Purpose

NodeOS is the firmware platform for intelligent magnetic-field acquisition modules used in the ND280 monitoring system.

Each module shall:

1. identify itself through hardware DIP switches;
2. initialize one or more TMAG5273A2 sensors;
3. acquire temperature and three-axis magnetic-field data;
4. average and calibrate the measurements;
5. load and store persistent calibration data in EEPROM;
6. expose measurements and diagnostics through a transport-independent interface;
7. recover automatically after reset or power loss.

The architecture must support the present single-sensor development board and the future eight-sensor module without rewriting the complete firmware.

---

## 2. System Architecture

```text
PC Monitor / DAQ
       |
Master Controller
       |
External communication bus
       |
+-------------------------+
| Intelligent Node        |
|                         |
|  ATmega328PB             |
|   |                      |
|   +-- TWI0: external bus |
|   |                      |
|   +-- TWI1               |
|          |               |
|       TCA9548A            |
|     +--+--+--+--+        |
|     |  |  |  |  |        |
|   TMAG5273A2 x 1..8       |
+-------------------------+
```

The current prototype uses one TMAG5273A2 directly on TWI1. The future module will insert a TCA9548A between TWI1 and the sensors.

---

## 3. Design Principles

### 3.1 Hardware identity is not stored in EEPROM

The node ID is always read from the five DIP-switch inputs.

```text
PC0 -> ID bit 0
PC1 -> ID bit 1
PC2 -> ID bit 2
PC3 -> ID bit 3
PD2 -> ID bit 4
```

The external pull-ups are 100 kΩ to 3.3 V.

```text
Switch open        -> logic 1
Switch closed/GND  -> logic 0
Node ID            -> inverted five-bit value
```

This provides 32 hardware identifiers.

### 3.2 Sensor acquisition is independent of communication

The measurement code must not directly print to UART or write to an external bus.

It shall produce a measurement object that can be published through any transport:

- UART during development;
- external I²C target interface;
- RS-485 or CAN in future revisions;
- another transport without changing acquisition code.

### 3.3 Calibration is independent of the sensor driver

The TMAG driver returns uncalibrated engineering values.

A calibration service applies:

```text
Bcorrected = gain * [Bmeasured - offset - kT * (T - Tcal)]
```

Initial values:

```text
offset = 0
gain = 1
temperature coefficient = 0
```

### 3.4 Stable versions remain reproducible

Every working release must remain buildable and recoverable.

Suggested tags:

```text
node-v0.2.1
node-v0.3.0-eeprom
node-v0.3.1-nodeos-uart
monitor-v0.2.1
monitor-v0.3.0-calibration
```

---

## 4. Firmware Layers

## 4.1 HAL

The HAL contains only ATmega328PB register-level code.

```text
hal/
    gpio.c
    gpio.h
    uart0.c
    uart0.h
    twi0.c
    twi0.h
    twi1.c
    twi1.h
    eeprom_hal.c
    eeprom_hal.h
    timer.c
    timer.h
```

Rules:

- no TMAG register definitions;
- no calibration logic;
- no application protocol;
- no GUI-specific concepts.

---

## 4.2 Drivers

Drivers know the hardware devices but not the final application.

```text
drivers/
    tmag5273.c
    tmag5273.h
    tca9548a.c
    tca9548a.h
    board_id.c
    board_id.h
    led.c
    led.h
    buzzer.c
    buzzer.h
```

### TMAG5273 driver responsibilities

- verify device and manufacturer IDs;
- configure the TMAG5273A2;
- select ±133 mT or ±266 mT range;
- read temperature and X/Y/Z;
- report communication and conversion errors;
- return raw and engineering-unit data.

### TCA9548A driver responsibilities

- initialize the multiplexer;
- select one sensor channel;
- disable all channels;
- verify the selected channel;
- recover from bus errors where possible.

---

## 4.3 Services

Services implement reusable system behaviour.

```text
services/
    measurement_service.c
    measurement_service.h
    calibration_service.c
    calibration_service.h
    configuration_service.c
    configuration_service.h
    diagnostics_service.c
    diagnostics_service.h
    transport_service.c
    transport_service.h
    command_service.c
    command_service.h
```

### Measurement service

Responsibilities:

- manage one or more sensor channels;
- acquire N samples;
- average raw or converted values;
- assign sequence numbers;
- create measurement records;
- detect invalid or saturated readings.

### Calibration service

Responsibilities:

- apply offsets, gains and temperature coefficients;
- expose calibration validity;
- provide factory defaults;
- support future multi-channel calibration.

### Configuration service

Responsibilities:

- load EEPROM data;
- validate magic number, version and CRC;
- provide defaults when EEPROM is empty or corrupt;
- save verified configuration;
- never overwrite the DIP-derived hardware ID.

### Diagnostics service

Responsibilities:

- sensor availability;
- EEPROM validity;
- bus faults;
- sample failures;
- reset cause;
- saturation state;
- calibration state.

### Transport service

Responsibilities:

- publish measurements;
- accept commands;
- hide the physical communication interface from the application;
- initially support UART0;
- later support TWI0 target mode.

---

## 4.4 Application

```text
application/
    node_app.c
    node_app.h
    main.c
```

`main.c` should remain minimal:

```c
int main(void)
{
    node_app_init();

    while (1)
    {
        node_app_process();
    }
}
```

Application flow:

```text
Acquire
  -> Average
  -> Calibrate
  -> Update diagnostics
  -> Publish
  -> Process commands
```

---

## 5. Core Data Structures

## 5.1 Measurement

```c
typedef struct
{
    uint8_t node_id;
    uint8_t channel;
    uint32_t sequence;
    uint32_t uptime_ms;

    int16_t temperature_centi_c;
    int32_t bx_micro_t;
    int32_t by_micro_t;
    int32_t bz_micro_t;

    uint8_t sensor_status;
    uint16_t diagnostic_flags;
} measurement_t;
```

Recommended internal magnetic-field unit: **microtesla**.

This avoids floating-point dependence and preserves adequate resolution:

```text
1 mT = 1000 µT
200 mT = 200000 µT
```

---

## 5.2 Node configuration

```c
typedef struct
{
    uint32_t magic;
    uint16_t structure_version;
    uint16_t structure_size;

    uint8_t averaging_samples;
    uint16_t sample_interval_ms;
    uint16_t magnetic_range_mt;

    int32_t offset_micro_t[8][3];
    int32_t gain_ppm[8][3];
    int32_t temp_coeff_micro_t_per_c[8][3];
    uint32_t noise_sigma_nano_t[8][3];

    int16_t calibration_temperature_centi_c;
    uint32_t calibration_counter;

    uint16_t crc16;
} node_configuration_t;
```

The first implementation may support one channel while reserving the structure for eight channels.

---

## 6. EEPROM Policy

EEPROM shall store:

- structure magic;
- structure version;
- calibration parameters;
- acquisition configuration;
- CRC;
- calibration counter.

EEPROM shall not store:

- hardware node ID;
- transient sequence number;
- current measurement;
- temporary command state.

On startup:

```text
Read EEPROM
  -> validate magic
  -> validate version
  -> validate structure size
  -> validate CRC
      -> valid: use stored configuration
      -> invalid: load defaults and report CAL=DEFAULT
```

Calibration states:

```text
DEFAULT  no valid stored calibration
DIRTY    modified in RAM, not saved
VALID    loaded from verified EEPROM
ERROR    EEPROM or CRC failure
```

---

## 7. Communication

## 7.1 Current development transport

UART0 at 4800 baud, 8N1.

Current packet example:

```text
$ND280,VER=0.3.1,ID=4,SEQ=123,AVG=10,CAL=VALID,T=29.87,BX=-0.20,BY=-0.24,BZ=-0.21,STATUS=0x13
```

UART is a development and diagnostics transport, not the final sensor-array topology.

## 7.2 Planned external node interface

TWI0 will operate as an I²C target interface.

The node address shall be derived from the DIP ID:

```text
External address = configurable base + node ID
Example base 0x20:
ID 0  -> 0x20
ID 31 -> 0x3F
```

The external controller shall read a stable register map or message buffer. It shall never directly access the internal TMAG bus.

## 7.3 Future robustness study

For long external distances, compare:

- standard low-frequency I²C;
- PCA9515B bus segmentation;
- differential I²C such as PCA9615;
- P82B96-class bus extension;
- RS-485;
- CAN.

This decision must be based on measured bus capacitance, cable topology, EMC environment and recovery requirements.

---

## 8. Planned Eight-Sensor Node

Each node will contain:

```text
1 x ATmega328PB
1 x TCA9548A
1..8 x TMAG5273A2
5-bit DIP node ID
EEPROM internal to ATmega328PB
external communication interface
local diagnostics LEDs
```

Startup sequence:

```text
Read DIP node ID
Load EEPROM configuration
Initialize TWI1
Initialize TCA9548A
For channel 0..7:
    select channel
    probe TMAG at 0x35
    verify DEVICE_ID and manufacturer ID
    configure ±266 mT range
Mark channel present or absent
Start acquisition
Expose data to external controller
```

Because each TCA9548A channel contains only one TMAG, all sensors may retain native address `0x35`.

---

## 9. PC Software Architecture

```text
software/ND280Monitor/
    main.py
    parser.py
    serial_worker.py
    logger.py
    calibration.py
    node_state.py
    replay.py
```

Current capabilities:

- serial connection;
- packet parsing;
- live plots;
- multi-node state;
- CSV logging;
- packet-loss detection;
- stationary noise acquisition;
- six-position offset workflow;
- JSON calibration export framework.

Planned capabilities:

- read/write node configuration;
- EEPROM verification;
- live statistics;
- replay;
- multi-channel node display;
- calibration reports;
- offline analysis.

---

## 10. Calibration Strategy

The development board validates the procedure only. Final performance characterization must be repeated on the final hardware.

### Without an external calibrated magnetometer

The system can determine:

- repeatability;
- stationary noise;
- short- and long-term drift;
- temperature correlation;
- offset using opposite orientations;
- axis consistency.

It cannot certify absolute gain without a known magnetic-field reference.

### Calibration levels

**Quick offset procedure**

- acquire a stationary reference;
- useful for relative monitoring;
- does not represent an absolute zero-field calibration.

**Six-position procedure**

- acquire X+, X-, Y+, Y-, Z+, Z-;
- estimate hard-axis offsets;
- validate orientation response.

**Absolute calibration**

Requires one of:

- calibrated gaussmeter;
- calibrated Hall probe;
- characterized Helmholtz coil;
- mapped reference magnet.

---

## 11. Architecture Freeze Criteria

NodeOS 1.0 architecture may be frozen when:

- HAL interfaces are stable;
- sensor and multiplexer drivers are separated;
- measurement and calibration services are independent;
- EEPROM format is versioned;
- transport abstraction supports UART and planned TWI0;
- application code has no direct register access;
- single-sensor regression tests pass;
- eight-channel design is representable without architectural changes.

---

## 12. Immediate Next Actions

1. Archive the currently working firmware and monitor releases.
2. Create the repository structure.
3. Move current drivers into HAL, drivers and services folders without changing behaviour.
4. Write regression tests for:
   - UART;
   - TWI1;
   - TMAG identity;
   - measurement;
   - Board ID;
   - EEPROM CRC and defaults.
5. Define the TWI0 external register map.
6. Add calibration write/read support to the monitor.
7. Start the TCA9548A prototype design only after the single-node calibration path is complete.

---

## 13. Open Decisions

- final master-to-node physical bus;
- number and spacing of sensors per 2 m module;
- external connector and cable;
- local power distribution;
- time synchronization method;
- acceptable sampling rate;
- absolute calibration method;
- environmental and radiation constraints;
- watchdog and fault recovery policy;
- firmware update strategy.

---

## 14. Current Project Baseline

Validated functions:

```text
ATmega328PB ISP programming
Internal 1 MHz clock
UART0 at 4800 baud
TWI1 on PE0/PE1
TMAG5273A2 identity
±266 mT acquisition
Temperature acquisition
Ten-sample averaging
Five-bit DIP node ID
Versioned ASCII packet
Windows live monitor
Multi-node GUI
CSV logging
Noise acquisition
EEPROM defaults, CRC and persistence
Bidirectional UART command framework
```

This document is the starting point for the NodeOS 1.0 architecture review.
