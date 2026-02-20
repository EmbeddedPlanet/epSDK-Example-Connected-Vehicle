/****************************************************************************                                                                     *
 * Copyright (c) 2026 Embedded Planet, Inc.                                 *
 * SPDX-License-Identifier: Apache-2.0                                      *
 *                                                                          *
 * Licensed under the Apache License, Version 2.0 (the "License");          *
 * you may not use this file except in compliance with the License.         *
 * You may obtain a copy of the License at                                  *
 *                                                                          *
 *     http://www.apache.org/licenses/LICENSE-2.0                           *
 *                                                                          *
 * Unless required by applicable law or agreed to in writing, software      *
 * distributed under the License is distributed on an "AS IS" BASIS,        *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 * See the License for the specific language governing permissions and      *
 * limitations under the License.                                           *
 ****************************************************************************/

/**
 * @file    mc33397.h
 * @version 1.0.0
 * @author  Embedded Planet, Inc.
 * @author  Dan Maher
 * @date    9 June 2022
 * 
 * @brief Contains functions for the configuration and control of the SPI MC33397 low-side switch.
 * 
 * Built for use with the nRF SDK 17.1
 * 
 * @see Data sheet:     https://rocelec.widen.net/view/pdf/u56vahidjm/MOTOS08149-1.pdf?t.download=true&u=5oefqw
 * @see Error codes:    https://infocenter.nordicsemi.com/index.jsp?topic=%2Fsdk_nrf5_v17.1.0%2Fgroup__nrfx__error__codes.html
 */

#ifndef MC33397_H
#define MC33397_H

#include "nrfx_spim.h"

/** @brief Used to hold configuration data of a single MC33397. Required prior to call to mc33397_init.
 */
typedef struct{
    nrfx_spim_t spi;     /** < SPI interface that the MC33397 is using */ 
    uint8_t tx_message; /** < Holds the current pin output data, and is used to send data to the MC33397 */
    uint8_t rx_message; /** < Holds the return data from the MC33397. */
    bool sleep_mode;    /** < Holds the sleep bit, which is ORed to tx_message during transmission */
} MC33397;

/** @brief Can be used as a parameter for mc33397_init for code readability.
 */
typedef enum{
    MC33397_SLEEP_MODE_DISABLED,    /** < If passed to mc33397_init, sleep mode will be disabled */
    MC33397_SLEEP_MODE_ENABLED      /** < If passed to mc33397_init, sleep mode will be enabled */
} mc33397_sleep_mode;

/** @brief Can be used as a parameter to mc33397_write_pin for code readability.
 */
typedef enum{
    MC33397_PIN_OFF,    /** < If passed to mc33397_write_pin, the pin will be turned off*/
    MC33397_PIN_ON      /** < If passed to mc33397_write_pin, the pin will be turned on*/
} mc33397_pin_level;

/** @brief MC33397 pin names for passing into the mc33397_write functions for code readability. 
 * mc33397_write_pins allows for the ORing of these values to set multiple pins.
 */
typedef enum{
    MC33397_A0 = 0x01,      /** < Output pin A0 */
    MC33397_A1 = 0x02,      /** < Output pin A1 */
    MC33397_A2 = 0x04,      /** < Output pin A2 */
    MC33397_A3 = 0x08,      /** < Output pin A3 */ 
    MC33397_A4 = 0x10,      /** < Output pin A4 */
    MC33397_A5 = 0x20,      /** < Output pin A5 */
    MC33397_NONE = 0x00,    /** < All pins off */
    MC33397_ALL = 0xFF,     /** < All pins on */
} mc33397_pins;

/**
 * @brief   Inits a MC33397 struct with the SPI interface used and the sleep mode.
 * 
 * See page 15 of the 33397 data sheet under "EN" for a description of sleep mode.
 * 
 * @param mc33397       Pointer to the MC33397 struct
 * @param spi           SPIM interface being used
 * @param sleep_mode    MC33397_SLEEP_MODE_DISABLED to disable sleep mode
 *                      MC33397_SLEEP_MODE_ENABLED to enable sleep mode
 */
void mc33397_init(MC33397 *mc33397, nrfx_spim_t spi, mc33397_sleep_mode sleep_mode);

/**
 * @brief   Sets the output of all output pins at once
 * 
 * See page 14 of the 33397 data sheet under "A0-A5" for pin description
 * 
 * @param mc33397   Pointer to the MC33397 struct
 * @param pins      An ORing of the pins that should be turned on (switched low)
 * 
 * For ease of use, an ORing of elements in the mc33397_pins enum can be used, or just a raw byte where bits 5:0 
 * correspond to output pins 5:0. All pins not included (or set to 0) will be automatically configured to be off.
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t mc33397_write_pins(MC33397 *mc33397, mc33397_pins pins);

/**
 * @brief   Sets a single output pin
 * 
 * @param mc33397   Pointer to the MC33397 struct
 * @param pin       Pin to be written. Can use an element from the mc33397_pins enum, or just a number 0-5.
 * @param level     MC33397_PIN_OFF to turn the output off. 0 can also be used.
 *                  MC33397_PIN_ON to turn the output on. 1 can also be used.
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t mc33397_write_pin(MC33397 *mc33397, mc33397_pins pin, mc33397_pin_level level);

/**
 * @brief   Retrieves the current pin faults from the MC33397.
 * 
 * See page 1 of the data sheet for a description of possible pin faults
 * 
 * @param mc33397   Pointer to the MC33397 struct
 * @param ret_code  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * 
 * @return  bit 0 - Fault on A0 (active fault if high)
 *          bit 1 - Fault on A1
 *          bit 2 - Fault on A2
 *          bit 3 - Fault on A3
 *          bit 4 - Fault on A4    
 *          bit 5 - Fault on A5
 *          bit 6 - Don't care
 *          bit 7 - Don't care
 */ 
uint8_t mc33397_get_faults(MC33397 *mc33397, nrfx_err_t *ret_code);

/**
 * @brief   Can be used after a sensor power cycle to send down the last known chip configuration. This includes
 * the sleep mode and the last known output levels.
 * 
 * @param mc33397   Pointer to the MC33397 struct
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file. 
 */
nrfx_err_t mc33397_send_config(MC33397 *mc33397);

#endif