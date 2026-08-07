# System Views Catalogue

## SV-01 - Overall System Architecture
Purpose: show the complete ND280 magnetic-field monitoring architecture.
Key message: autonomous intelligent nodes are coordinated through a segmented network.

## SV-02 - Intelligent Acquisition Node
Purpose: define the functional content of one acquisition node.
Key message: local sensor acquisition (TWI1) is separated from inter-board communication (TWI0).

## SV-03 - NodeOS Layered Architecture
Purpose: show the embedded software abstraction levels.
Key message: the application never directly accesses hardware resources.

## SV-04 - Measurement Processing Workflow
Purpose: show how a physical field measurement becomes a scientific data product.
Key message: measurement, calibration, diagnostics, transport and aggregation are explicit stages.

## SV-05 - Engineering Evolution
Purpose: show how validation drives each subsequent development stage.
Key message: every validated stage becomes the engineering baseline for the next revision.

## RV-01 - Reference Single-Sensor Platform
Purpose: show the validated reference acquisition chain.
Key message: one TMAG5273A2, NodeOS and ND280 Monitor were sufficient to validate the complete end-to-end architecture.
