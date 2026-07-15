# Test Plan - v0.4.0-alpha4

1. Reset or power-cycle the node.
2. Verify the sequence PD5 -> PD6 -> PD7 -> all three LEDs.
3. Confirm the serial banner reports `v0.4.0-alpha4`.
4. Confirm TMAG_DEVICE_ID=0x06 and TMAG_INIT=OK.
5. Confirm PD5 resumes its existing heartbeat behaviour.
6. Confirm PD6 is continuously ON while the TMAG is operational.
7. Confirm PD7 matches DEFAULT, DIRTY and VALID calibration states.
8. Verify HELP, INFO and CAL READ still respond.
9. Verify measurement packets continue with sequential SEQ values.
