# Abstract

The magnetic field inside the ND280 detector represents one of the fundamental environmental parameters affecting detector performance, charged-particle reconstruction and long-term detector stability.

A continuous monitoring system capable of measuring the magnetic field with adequate spatial granularity, long-term stability and high reliability is therefore required.

Commercial magnetic field monitoring solutions are generally not suitable for the mechanical, geometrical and integration constraints imposed by the detector. They also provide limited flexibility in terms of scalability, customization and integration with the experiment control infrastructure.

The objective of this project is the development of a modular and distributed magnetic field monitoring system specifically designed for the ND280 detector.

The proposed architecture is based on intelligent acquisition nodes built around the ATmega328PB microcontroller and the Texas Instruments TMAG5273A2 three-axis magnetic sensor.

Each node performs local acquisition, averaging, calibration, diagnostics and health monitoring while exposing a standardized communication interface towards higher-level controllers.

The architecture has intentionally been designed to separate hardware-dependent functions, acquisition services, calibration, diagnostics and communication.

This modular approach allows the same firmware architecture to support both the current single-sensor prototype and future multi-sensor nodes based on an I²C multiplexer without requiring major architectural changes.

The long-term objective is the realization of a scalable, maintainable and robust magnetic field monitoring infrastructure suitable for deployment within the ND280 detector.


# 1. Introduction

## 1.1 Purpose of the Project

The objective of this project is the design, implementation and qualification of a distributed magnetic field monitoring system for the ND280 experiment.

The system has been conceived to provide continuous monitoring of the magnetic field through a network of intelligent acquisition nodes specifically designed for the experiment.

The project is developed within INFN and follows an engineering approach in which hardware, firmware, software and documentation evolve together.

---

## 1.2 Motivation

The magnetic field is one of the fundamental parameters affecting detector performance.

Long-term monitoring of the magnetic field allows:

- verification of detector operating conditions;
- detection of abnormal variations;
- correlation with detector performance;
- long-term stability studies;
- support during detector installation and maintenance.

Commercial magnetic-field monitoring solutions generally do not satisfy the geometrical, mechanical and integration requirements of the experiment.

For this reason a dedicated monitoring system has been designed.

---

## 1.3 Design Philosophy

The project has been developed following a modular architecture.

The system is divided into four independent layers:

- Hardware
- NodeOS Firmware
- ND280 Monitor
- Documentation

Each layer can evolve independently while maintaining compatibility with the others.

The firmware has been designed so that hardware-dependent functions are isolated from acquisition, calibration and communication services.

This approach minimizes future modifications when new hardware revisions become available.

---

## 1.4 Intelligent Acquisition Node

Each acquisition node is based on:

- ATmega328PB microcontroller
- TMAG5273A2 three-axis magnetic sensor
- EEPROM configuration storage
- Hardware node identification through DIP switches
- Local diagnostics
- Watchdog supervision

Each node performs:

- magnetic-field acquisition;
- temperature acquisition;
- averaging;
- calibration;
- diagnostics;
- communication with higher-level controllers.

---

## 1.5 Current Development Status

The first hardware prototype has been completely validated.

The following functions are operational:

- ISP programming
- UART communication
- TWI communication
- TMAG5273A2 driver
- EEPROM configuration
- Calibration framework
- Diagnostics
- Watchdog
- Command parser based on a finite-state machine
- Python monitoring application
- Long-term stability tests

The current prototype represents the reference implementation of the NodeOS architecture.

---

## 1.6 Future Developments

The next hardware revision will extend the current architecture by introducing an I²C multiplexer (TCA9548A) allowing one acquisition node to manage multiple TMAG5273A2 sensors.

The firmware architecture has been intentionally designed to support this evolution without major structural modifications.

Future work will include:

- multi-sensor acquisition;
- master controller;
- distributed node network;
- enhanced diagnostics;
- qualification on the final detector hardware.

---

## 1.7 Scope of this Document

This Technical Design Report describes:

- system requirements;
- hardware architecture;
- firmware architecture;
- communication protocol;
- monitoring software;
- validation campaign;
- future developments.

The purpose of this document is to provide a complete technical description of the ND280 Magnetic Field Monitoring System and to record the engineering decisions taken during its development.