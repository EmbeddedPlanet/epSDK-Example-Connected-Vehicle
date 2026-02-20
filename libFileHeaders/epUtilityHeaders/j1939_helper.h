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
 * @file    j1939_helper.h
 * @version 1.0.0
 * @author  Embedded Planet, Inc.
 * @author  M. Golob
 * @date    21 April 2023
 * 
 * @brief Contains functions for parsing J1939 messages read from a CAN bus
 * 
 * Built for use with the nRF SDK 17.1
 * 
 */

#ifndef J1939_HELPER_H
#define J1939_HELPER_H

#include "mcp2515.h"

/* Set max number of DM1 faults for initial release */
#define J1939_MAX_FAULT_ELEMENTS    100 /* Currently 100 SPNs that we need to listen for faults */

/* Engine RPM PGN Number */
#define ENGINE_RPM_PGN  61444

/* This is the maximum size for transferring data */
#define MAX_TP_DT 1785U

/**
 * @brief J1939 event handler definition.
 */
typedef float ( * J1939EventHandler_t )( uint8_t *rxData );

typedef struct J1939ChTable
{
    uint32_t channel;             /**< J1939 Channel Number */
    uint32_t pgn;                 /**< J1939 PGN Number */
    uint32_t spn;                 /**< J1939 SPN Number */
    J1939EventHandler_t handler;  /**< Action to perform */
    char * text;                  /**< Channel description */
    float val;                    /**< Channel value */
    uint8_t size;                 /**< Size of data in bytes */
    bool dataAval;                /**< Data available flag */
    bool cloudSupported;          /**< Channel support flag */
} J1939ChTable_t;

typedef struct J1939PGNTable
{
    uint32_t pgn;       /**< J1939 PGN Number */
    uint8_t rxData[8];  /**< Action to perform */
    bool dataAval;      /**< Data available flag */
    bool cloudSupported;/**< PGN support flag */
} J1939PGNTable_t;

/**
 * @brief J1939 PGN Table for J1939:73 Diagnostics Layer, DM1 PGN:65226.
 */
typedef struct
{
    uint8_t     SA[J1939_MAX_FAULT_ELEMENTS]; //Source address
    uint32_t    SPN[J1939_MAX_FAULT_ELEMENTS]; //Suspect Parameter Number
    uint8_t     FMI[J1939_MAX_FAULT_ELEMENTS];  //Failure Mode Indicator
    uint8_t     occurrence_count[J1939_MAX_FAULT_ELEMENTS];  //Occurence
    bool        CM[J1939_MAX_FAULT_ELEMENTS];  //Conversion method
    char        status[J1939_MAX_FAULT_ELEMENTS][sizeof("De-active")+1]; // Active or De-active status
    bool        pubStatus[J1939_MAX_FAULT_ELEMENTS]; // Publish status
    uint8_t     inactiveCnt[J1939_MAX_FAULT_ELEMENTS];  // Counter to keep track of timeout to inactivity status
    uint8_t     errors_dm1_recv;
    uint8_t     indexTracker;
}J1939DM1_t;

/**
 * @brief J1939 TP Connection Management PGN:60416
 */
typedef struct
{
    uint8_t     control_byte; /* Control byte */
    uint16_t    total_message_size; /* Total message size */
    uint8_t     number_of_packages; /* Number of packages */
    uint32_t    PGN_of_packages;  /* PGN for packages */
    uint8_t     SA;  /* Source address */
}J1939ConnMan_t;

/**
 * @brief J1939 TP Data Transfer PGN:60160
 */
typedef struct
{
    uint8_t     sequence_number; /* Sequence number */
    uint16_t    data[MAX_TP_DT]; /* Buffer for data */
    uint8_t     SA;  /* Source address */
}J1939DataTrans_t;

/**
 * @brief   Set up MCP2515 for use with J1939.
 * 
 * @param mcp215       Structure holding config data of MCP2515.
 * 
 * @return  true if successfully setup driver. false if failed.
 */
bool can_init_j1939( MCP2515 *mcp2515 );

/**
 * @brief   Parse J1939 message.
 * 
 * @param mcp215       Structure holding config data of MCP2515.
 * 
 */
void Parse_J1939_Data( MCP2515 *mcp2515 );

/**
 * @brief   Create J1939 message and send into cellular queue.
 * 
 * @param ep_serial        Serial number of device
 * @param updateVerMaj     Current major version running on unit
 * @param updateVerMin     Current minor version running on unit
 * @param updateVerBui     Current build version running on unit
 *
 */
void Create_J1939_Msg( char *ep_serial, uint8_t updateVerMaj, uint8_t updateVerMin, uint16_t updateVerBui );

/**
 * @brief   If there is need to mark DTC inactive, as timeout exceeded.
 *
 */
void J1939_DTC_Status_Check( void );

/**
 * @brief   Check if engine has started, app can initiate an immediate response to server.
 * 
 * @return  true if app should sent message immediately. false if no immediate message necessary.
 */
bool J1939_Immediate_Resp( void );

/**
 * @brief   Update list of SPNs to support
 * 
 * @param spn    SPN number to update status
 * @param status True-Support SPN, False-Do not support SPN
 */
void Update_SPN_Status( uint32_t spn, bool status );

#endif
