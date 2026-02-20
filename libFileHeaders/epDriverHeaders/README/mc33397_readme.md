# MC33397 Driver

This is a driver for the Motorola MC33397 I2C  Low-Side Switch. It utilizes the nRF SDK for use with the nRF52XXX series SoCs.

## Contents
**mc33397.h** - Driver header.

## Set up
1. Include **mc33397.h** in the necessary source files
2. Add the include path for **mc33397.h** to your Makefile
3. Add the path to for the associated static library to your Makefile

## Use
Detailed use can be found in the header file. However, the most simplified steps are as follows:

```C++
/* Create empty MC33397 struct */
MC33397 mc33397;

/* Initialize the struct with the proper settings. In this case, a twi interface previously created and sleep mode enabled: */
mc33397_init(&mc33397, spi, MC33397_SLEEP_MODE_ENABLED);

/* Turn on output A3 */
mc33397_write_pin(&mc33397, MC33397_A3, MC33397_PIN_ON);

/* Turn on multiple outputs at once. All not included will be autmatically set to off. */
mc33397_write_pins(&mc33397, MC33397_A0 | MC33397_A4 | MC33397_A5);

/* Read back pin faults, with bit 0 being A0 fault and bit 5 being A5 fault. */
fault = mc33397_get_faults(&mc33397, &err);
```

## Versions
V1.0.0 Initial Release
