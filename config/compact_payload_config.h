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
 * @file    compact_payload.h
 * @version 1.0.0
 * @author  Embedded Planet, Inc.
 * @author  golobmichael
 * @date    19 JAN 2024
 * 
 * @brief Header for compact payload configurations.
 * versions:
 *  1.0.0 - 19JAN2024  Initial - golobmichael
 * 
 * Built for use with the nRF SDK 17.1 and FreeRTOS.
 */

/**
 * @file compact_payload_config.h
 */

#ifndef __COMPACT_PAYLOAD_CONFIG_H__
#define __COMPACT_PAYLOAD_CONFIG_H__

/* *INDENT-OFF* */
#ifdef __cplusplus
    extern "C" {
#endif
/* *INDENT-ON* */

#define EPCP_VERSION 0x08   /** < EP Compact Payload V8 */

/**
 * @brief Sensor ID Numbers
 * 
 * Conform to the EPCP format version listed. DO NOT CHANGE at
 * risk of breaking any decoder that uses these ID numbers. 
 * 
 * Any addition or change requires a new version of the specification P399x024.
 * 
 */
typedef enum epcp_sensor_id{
    EPCP_SYSTEM    = 0x01,
    EPCP_SYSTEM_V2     = 0x02,
    //0x03 - 0x04 reserved for future use
    EPCP_CELL       = 0x05,
    EPCP_GNSS       = 0x06,
    EPCP_GTP        = 0x07,
    EPCP_SI7021     = 0x08,
    EPCP_BME680     = 0x09,
    EPCP_ICM20602   = 0x0A,
    EPCP_LSM9DS1    = 0x0B,
    EPCP_VL53L0X    = 0x0C,
    EPCP_OBDII      = 0x0D,
    EPCP_CE         = 0x0E,
    EPCP_J1939      = 0x0F
}EPCP_SENSOR_ID;

/**
 * @brief Sensor subpacket sizes
 * 
 * These are used during packet creation to set the size of a particular subpacket.
 * These are found by the addition of all data included in the packet, as specified in
 * P399x024.
 */
typedef enum epcp_sensor_packet_size{
    EPCP_SYSTEM_SIZE    = 19,
    EPCP_SYSTEM_V2_SIZE = 9,
    EPCP_CELL_SIZE      = 10,
    EPCP_GNSS_SIZE      = 10,
    EPCP_GTP_SIZE       = 13,
    EPCP_SI7021_SIZE    = 6,
    EPCP_BME680_SIZE    = 25,
    EPCP_ICM20602_SIZE  = 14,
    EPCP_LSM9DS1_SIZE   = 21,
    EPCP_VL53L0X_SIZE   = 6
    //EPCP_OBDII_SIZE       = xx OBDII size is determined dynamically
    //EPCP_CE_SIZE          = xx Connected Equipment size is determined dynamically
    //EPCP_J1939_SIZE       = xx J1939 size is determined dynamically
} EPCP_SENSOR_PACKET_SIZE;

/** 
 * @brief  Sensor Data Slots
 * 
 * Used for data position tracking within subpackets. These are generic keys, and
 * can be used multiple times per packet, but only once per subpacket.
 * 
 * For example, both the ICM20602 and the LSM9DS1 can have the EPCP_ACCEL_X sensor data slot types.
 * 
 */
typedef enum epcp_sensor_slot_type{
    EPCP_VER,           //EPCP formatting version
    EPCP_SN,            //Device Serial Number
    EPCP_FW_MAJOR,      //Firmware version major value (X.0.0)
    EPCP_FW_MINOR,      //Firmware version minor value (0.X.0)
    EPCP_FW_PATCH,      //Firmware version patch value (0.0.X)
    EPCP_MSG_CNT,       //Message counter
    EPCP_IMEI,          //Cell IMEI
    EPCP_ICCID,         //Cell ICCID
    EPCP_RSSI,          //Cell RSSI
    EPCP_RSRQ,          //Cell RSRQ
    EPCP_SAT,           //GNSS Satellites
    EPCP_LAT,           //GNSS Latitude
    EPCP_LON,           //GNSS Longitude
    EPCP_GTP_LAT,       //GPT Latitidue
    EPCP_GTP_LON,       //GPT Longitude
    EPCP_GTP_ACC,       //GTP Accuracy
    EPCP_ALARMS,        //Active sensor alarms
    EPCP_TIME,          //System time or run time
    EPCP_BATT,          //Battery voltage
    EPCP_TEMP,          //Temperature reading
    EPCP_PRES,          //Pressure reading
    EPCP_HUM,           //Humidity reading
    EPCP_GAS,           //Gas reading
    EPCP_CO2,           //CO2 reading
    EPCP_BREATH,        //Breath equivelent reading
    EPCP_IAQ_SCORE,     //Indoor air quality score
    EPCP_IAQ_ACCURACY,  //Indoor air quality score accuracy
    EPCP_ACCEL_X,       //Accelerometer x-axis reading
    EPCP_ACCEL_Y,       //Accelerometer y-axis reading
    EPCP_ACCEL_Z,       //Accelerometer z-axis reading
    EPCP_GYRO_X,        //Gyroscope x-axis reading
    EPCP_GYRO_Y,        //Gyroscope y-axis reading
    EPCP_GYRO_Z,        //Gyroscope z-axis reading
    EPCP_MAG_X,         //Magnetometer x-axis reading
    EPCP_MAG_Y,         //Magnetometer y-axis reading
    EPCP_MAG_Z,         //Magnetometer z-axis reading
    EPCP_TILT_ANG,      //Tilt angle
    EPCP_DIST,          //Distance reading
    EPCP_4_20,          //4-20mA reading
    EPCP_0_10,          //0-10V reading
    EPCP_DRY,           //Dry contact reading
    EPCP_PWR,           //Input power voltage
    EPCP_ELEMENT_CNT,   // Element count
    EPCP_VIN,           // OBDII VIN
    EPCP_PID,           // OBDII PIDs
    EPCP_ACT_DTC,       // OBDII Active DTCs
    EPCP_INACT_DTC,     // OBDII Inactive DTCs
    EPCP_DTC            // DTCs
} EPCP_SENSOR_SLOT_TYPE;

/* *INDENT-OFF* */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON* */

#endif /* __COMPACT_PAYLOAD_CONFIG_H__ */