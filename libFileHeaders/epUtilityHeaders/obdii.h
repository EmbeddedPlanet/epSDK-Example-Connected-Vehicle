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
 * @file    obdii.h
 * @version 2.0.0
 * @author  Embedded Planet, Inc.
 * @author  M. Timieski
 * @date    10 AUG 2023
 * 
 * @brief Contains functions for parsing OBDII messages read from a CAN bus
 * 
 * Built for use with the nRF SDK 17.1
 * 
 */

#ifndef OBDII_H
#define OBDII_H

//#include "nrfx_spi.h"
#include <nrfx.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "mcp2515.h"

/* Max DTCs supported */
#define MAX_DTC_SUPPORTED       30

/* OBDII Service Modes */
#define OBDII_SVC_CURRENT_DATA      0x01
#define OBDII_SVC_FREEZE_DATA       0x02
#define OBDII_SVC_STORED_DTCS       0x03
#define OBDII_SVC_CLEAR_DTCS        0x04
#define OBDII_SVC_TEST_NON_CAN      0x05
#define OBDII_SVC_TEST_CAN          0x06
#define OBDII_SVC_PENDING_DTCS      0x07
#define OBDII_SVC_CNTL_OP           0x08
#define OBDII_SVC_VEHICLE_INFO      0x09
#define OBDII_SVC_PERM_DTCS         0x0A

/** @brief Used to hold connection data to the vehicle ECU.  The data can be accessed globally.
 */
typedef struct OBDII
{
    uint8_t vin[17];              /** < Holds the vehicle VIN number in ASCII */
    uint32_t supported_pid_01_20; /** < bit endoded value if ECU supports pids from  1(MSB) to 20(LSB), value of 1 = supported */
    uint32_t supported_pid_21_40; /** < bit endoded value if ECU supports pids from 21(MSB) to 40(LSB), value of 1 = supported */
    uint32_t supported_pid_41_60; /** < bit endoded value if ECU supports pids from 41(MSB) to 60(MSB), value of 1 = supported */
    uint32_t supported_pid_61_80; /** < bit endoded value if ECU supports pids from 61(MSB) to 80(MSB), value of 1 = supported */
    uint32_t supported_pid_81_A0; /** < bit endoded value if ECU supports pids from 81(MSB) to A0(MSB), value of 1 = supported */
    bool    connected;            /** < true when ECU supplies OBDII data during initialization. */
} OBDII_t;
extern OBDII_t obdii_gen_info;

/** @brief Used to hold engineering data for a PID from the vehicle ECU.  Some PIDs return more than a single result:
 *         All PIDs return 'results_a/units_a', some PIDs 'results_b/units_b'.  The number of results available is set in 'number_of_results'.
 *         The data can be accessed globally.
 */
typedef struct OBDII_DATA
{
    uint8_t pid;                /** < Holds the PID number that has been converted */
    char pid_name[45];          /** < Holds a text description of the converted PID */
    char units_a[17];           /** < Holds a text description of the engineering units for the first parameter in the results */
    char units_b[17];           /** < Holds a text description of the engineering units for the optional second parameter in the results */
    uint8_t number_of_results;  /** < Holds the number of parameters available in the result */
    double result_a;            /** < Holds the PID data in engineering units for the first parameter in the result */
    double result_b;            /** < Holds the PID data in engineering units for the optional second parameter in the result */
    bool Data_Rcvd;             /** < Holds the bool to signify data successfully received and stored from the bus */
    bool cloudSupported;        /** < PIDs to send to cloud */
} OBDII_DATA_t;

/**
 * @brief OBDII PID Table. All PIDs from A0-FF are reserved so they are not included in table.
 *        The OBDII_DATA structure in populated for each element with: pid, pid_name, units_a, units_b, number_of_results, cloudSupported
 *        each element accesses each pid, example: obdii_data[0].pid_name is the name of pid 0, obdii_data[2].pid_name is the name of pid 2.
 *        The table results are updated through the Read_OBDII_Data function.
 *        The data can be accessed globally.
 */
extern OBDII_DATA_t obdii_data[];

/** @brief Used to hold engineering data for a DTC from the vehicle ECU.
 *         The data can be accessed globally.
 * 
 * Code Represents as follows ABCDD
 *      A: 'P'-Powertrain Code (Engine & Transmission)
 *         'B'-Body Code (Inlcudes A/C & Air Bag)
 *         'C'-Chassis code (Include ABS)
 *         'U'-Network Code (Wiring Bus)
 *      B: '0'-Generic OBD Code
 *         '1'-Manufacturing Special Code
 *      C: '1'-Fuel & Air Metering
 *         '2'-Fuel & Air Metering (Injector Circuit)
 *         '3'-Ignition System or Misfire
 *         '4'-Auxiliary Emission Controls
 *         '5'-Vehicle Speed Control & Idle Control System
 *         '6'-Computer Output Circuit
 *         '7'-Transmission
 *         '8'-Transmission
 *      DD: Special Fault Index
 */
typedef struct
{
    uint8_t cnt_act_dtc;
    uint8_t dtc_byte_1[MAX_DTC_SUPPORTED];
    uint8_t dtc_byte_2[MAX_DTC_SUPPORTED];
    uint8_t status[MAX_DTC_SUPPORTED];
    uint8_t published[MAX_DTC_SUPPORTED];
} OBDII_DTC_DATA_t;

extern OBDII_DTC_DATA_t OBDII_DTC_data;

/**
 * @brief   Set up MCP2515 for use with OBDII.
 * 
 * @param mcp2515       Structure holding config data of MCP2515.
 * 
 * @return  true if successfully setup driver. false if failed.
 */
bool can_init_obdii( MCP2515 *mcp2515 );

/**
 * @brief   Setup OBDII data to go to cell queue 
 * 
 * @param ep_serial        Serial number of device
 * @param updateVerMaj     Current major version running on unit
 * @param updateVerMin     Current minor version running on unit
 * @param updateVerBui     Current build version running on unit
 * 
 */
void Create_OBDII_Msg( char *ep_serial, uint8_t updateVerMaj, uint8_t updateVerMin, uint16_t updateVerBui );

/**
 * @brief   Gather general obdii information
 * 
 * @param mcp2515       Structure holding config data of MCP2515.
 * 
 * @return  a bool, true equals pass
 */
bool Gather_General_OBDII_Info( MCP2515 *mcp2515 );

/**
 * @brief   Read all available data for selected service
 * 
 * @param mcp215       Structure holding config data of MCP2515.
 * @param service      Service from the list above titled OBDII Service Modes
 * 
 */
void Read_OBDII_Data( MCP2515 *mcp2515, uint8_t service );

/**
 * @brief   Update list of PIDs to support
 * 
 * @param pid    PID number to update status
 * @param status True-Support PID, False-Do not support PID
 */
void Update_PID_Status( uint8_t pid, bool status );

#endif

