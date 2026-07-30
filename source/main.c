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
 * @file    main.c
 * @version See Version in main.h
 * @author  Embedded Planet, Inc.
 * @author  golobmichael
 * @date    29 AUG 2023
 * 
 * @brief Connected Vehicle Main.  
 * 
 * Built for use with the nRF SDK 17.1
 * 
 */

/**
 * Created on: Sept 1, 2022
 * Created by: golobmichael
 * 
 * Copyright (c) Embedded Planet, Inc - All rights reserved
 *
 * This source file is private and confidential.
 * Unauthorized copying of this file is strictly prohibited.
 * 
 * Version 1.0 - 01SEPT22  Initial
 */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "queue.h"

#include "main.h"
#include "ep_bsp.h"
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "fds.h"
#include "nrfx_twi.h"
#include "nrf_drv_clock.h"
#include "uart_helper.h"
#include "cell_helper.h"
#include "mcp2515.h"
#include "mcp23008.h"
#include "qspi_helper.h"
#include "led_helper.h"
#include "j1939_helper.h"
#include "time_helper.h"
#include "obdii.h"
#include "sc16is750.h"
#if defined(THINGSBOARD_HTTPS_INTEGRATION) || defined(AZURE_MQTTS_X509) || defined(AWS_MQTTS_X509) || defined(AWS_HTTPS_X509) || defined(AZURE_MQTTS_SAS) || defined(AZURE_HTTPS_SAS)
#include "nrf_crypto.h"
#endif

SemaphoreHandle_t cellularSemaphore;
SemaphoreHandle_t dataReadySemaphore;
QueueHandle_t xCellQueue;
/* Semaphore to lock changing of sensor power enable */
SemaphoreHandle_t xSensPwrEnSemaphore;

// FreeRTOS defines
#define mainLED_TASK_STACK_SIZE             128
#define mainCell_TASK_STACK_SIZE            8196
#define DEAD_BEEF                           0xDEADBEEF                              /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */
#define CELL_QUEUE_TIMEOUT                  pdMS_TO_TICKS(3000)                     /** Time to wait on queue before moving on to the next sample */

/* SPIM instance ID. */
#define SPIM_INSTANCE  1

/* TWI instance ID. */
#define TWI_INSTANCE_ID     0

// CAN defines
#define CAN_PWR_ENABLE      PIN_NAME_TE_GPIO5
#define CAN_RESET           PIN_NAME_TE_GPIO4
#define MCP2515_INTERRUPT   PIN_NAME_TE_GPIO3

/* TWI instance. */
static const nrfx_twi_t twi = NRFX_TWI_INSTANCE(TWI_INSTANCE_ID);

/* Task handles */
TaskHandle_t CellTaskHandle;
TaskHandle_t ledTaskHandle;

/* Initial PDP config */
CellularBLEConfig_t pdnConfig = { '\0' };

system_info_struct system_info;

// Frame counter for payload
static uint8_t frameCounter = 0;

/* Flag to show car is running */
bool canFail = false;

/* Cellular Transmission Interval */
static uint32_t cellTransInt = MIN_TRANS_INTERVAL;

/* Empty SC16 struct */
SC16IS750 sc16;
// J1939 Empty MCP2515 structs (MCP and CAN) 
MCP2515 mcp2515;         //Config and SPI data
/* Empty MCP struct */
MCP23008 mcp;

/* Miscellaneous initialization including preparing the logging and cell. */
static void prvMiscInitialization( void );

/* Function for handling sensor power enable operation */
void sensorPwrEnConfig( bool status );

/*-----------------------------------------------------------*/
/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in]   line_num   Line number of the failing ASSERT call.
 * @param[in]   file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/*-----------------------------------------------------------*/

/* TWI uninitialization */
void twi_uninit (void)
{
    nrfx_twi_uninit(&twi);

    /* Uninit TWI pins */
    nrf_gpio_cfg_default(PIN_NAME_SCL);
    nrf_gpio_cfg_default(PIN_NAME_SDA);
}

/*-----------------------------------------------------------*/

void twi_init (void)
{
    nrfx_err_t err_code;

    const nrfx_twi_config_t twi_config = {
       .scl                = PIN_NAME_SCL,
       .sda                = PIN_NAME_SDA,
       .frequency          = NRF_TWI_FREQ_100K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .hold_bus_uninit     = false
    };

    err_code = nrfx_twi_init(&twi, &twi_config, NULL, NULL);
    APP_ERROR_CHECK(err_code);

    nrfx_twi_enable(&twi);
}

/*-----------------------------------------------------------*/
#if J1939_ACTIVE == 1
/**
 * @brief The J1939 task function parses and sends the data into the cell queue.
 *
 * @param[in] pvParameter   Pointer that will be used as the parameter for the task.
 */
void j1939_transfer_task(void *pvParameters)
{
    /* String for http app attributes */
    char sharedAppAttr[MAX_ATTR_VALUE_LENGTH] = {'\0'};
    uint16_t dlyCnt = 0;
    
    while(1)
    {
        init_uart(TASK_3);

        /* If time expried */
        if( dlyCnt > cellTransInt )
        {
            /* Create J1939 message */
            Create_J1939_Msg(system_info.ep_serial, updateVerMaj, updateVerMin, updateVerBui);

            /* Get HTTP Attributes here for now, Display attributes */
            for( uint8_t i = 0; i < num_HTTP_ATTRIBUTES; i++ )
            {
                getAttributes(i,sharedAppAttr);
                /* Check if attribute value is valid */
                if(sharedAppAttr[0] != '\0')
                {
                    /* For this app, only 1 attribute used, and its for cell transmission interval, overwrite this value */
                    cellTransInt = ( uint32_t ) strtoul( sharedAppAttr, NULL, 10 );
                    DBGI("New Cell Transmission Interval: %ds",cellTransInt);
                }
            }

            /* Reset timer */
            dlyCnt = 0;
        }

        /* Check if its been 30s since receiving active dtc, then de-activate dtc */
        J1939_DTC_Status_Check();

        /* Increment delay counter */
        dlyCnt++;

        /* Delay */
        vTaskDelay(pdMS_TO_TICKS(2000));

        uninit_uart(TASK_3);
    }

    vTaskDelete( NULL );
}

/**
 * @brief The J1939 task function which controls reading in of J1939 messages.
 *
 * @param[in] pvParameter   Pointer that will be used as the parameter for the task.
 */
static void j1939_task_function (void * pvParameters)
{
    init_uart(TASK_4);

    if (can_init_j1939( &mcp2515 ))
    {
       DBGI("Initialized CAN successfully");
    }
    else
    {
       DBGE("CAN controller init error");
    }

    // J1939 Parse SPN and send to cell, create after J1939 initialized
    xTaskCreate(j1939_transfer_task,
                "J1939transfertask",
                2000,
                NULL,
                tskIDLE_PRIORITY+2,
                NULL);

    while (true)
    {
        /* Check to see if receive interrupt received */
        while(nrf_gpio_pin_read(MCP2515_INTERRUPT) == 0)
        {
            /* Parse message */
            Parse_J1939_Data( &mcp2515 );        
            
            // Monitor FreeRTOS stack usage
            DBGI("FreeRTOS Heap Remaining: %d Bytes", xPortGetFreeHeapSize());
        }
        /* Give delay between passess */
        vTaskDelay(pdMS_TO_TICKS(3));
    }
}

#endif

/*-----------------------------------------------------------*/

#if OBDII_ACTIVE == 1
/**
 * @brief The OBDII task function which controls reading in of OBDII messages.
 *
 * @param[in] pvParameter   Pointer that will be used as the parameter for the task.
 */
static void obdii_task_function (void * pvParameters)
{
    bool firstPass = true;
    /* String for http app attributes */
    char sharedAppAttr[MAX_ATTR_VALUE_LENGTH] = {'\0'};

    /* Turn on debug uart */
    init_uart(TASK_4);

    if (can_init_obdii(&mcp2515))
    {
        DBGI("Initialized CAN successfully");
    }
    else
    {
        DBGE("Failed to Initialize CAN");
    }
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    while(1)
    {
        // Periodic gathering data, check for engine running (comms with ECU)
        while(!Gather_General_OBDII_Info( &mcp2515 ))
        {
            /* Set flag */
            canFail = true;
            //Shutdown power to the OBDII system
            DBGI("OBDII Failure, System is powered down, no CAN comms...\r\n\r\n");
            /* The following are functions we only want to call once during sleep */
            if( firstPass )
            {
                // Suspend LED task
                vTaskSuspend( ledTaskHandle );
                // Enable debug out for LED
                led_pause();
                firstPass = false;
            }
            /* Shutoff CAN */
            nrf_gpio_pin_clear(CAN_PWR_ENABLE);
            //Set the MCP1515 in reset to prevent the wiring from being a current leakage path
            nrf_gpio_pin_clear(CAN_RESET);
            /* Uninit UART */
            uninit_uart(TASK_4);
            /* Delay, put unit to sleep */
            vTaskDelay(pdMS_TO_TICKS(60000));
            /* Init debug uart */
            init_uart(TASK_4);
            //Enable power to the MCP2515
            nrf_gpio_pin_set(CAN_PWR_ENABLE);
            nrf_gpio_pin_set(CAN_RESET);
            vTaskDelay(pdMS_TO_TICKS(1000));
            //Put the MCP registers back in place after power was cut
            if (can_init_obdii(&mcp2515))
            {
                DBGI("Initialized CAN successfully");
                DBGI("System is powering up...\r\n\r\n");
            }
            else
            {
                DBGE("Failed to reinit CAN, retrying");
            }
        }
        if( canFail )
        {
            /* Reset flag */
            canFail = false;
            // Reenable LED task
            led_resume();
            vTaskResume( ledTaskHandle );
            /* Clear flag as next pass would be first again */
            firstPass = true;
            /* Renter code, OBDII on car is running again */
        }

        /* Get current obdii data with service 01
        Service 2 is data captured when the last trouble code
        is captured) */
        Read_OBDII_Data( &mcp2515, OBDII_SVC_CURRENT_DATA );

        /* Read Service 03 stored DTCs, Mode 7 and 10 are not useful for our application to get DTC */
        /* Mode 10 is used by emissions testers to see if someone got the MIL off but did not fix the repair */
        Read_OBDII_Data( &mcp2515, OBDII_SVC_STORED_DTCS );

        /* Create OBDII Json message */
        Create_OBDII_Msg(system_info.ep_serial, system_info.updateVerMaj, system_info.updateVerMin, system_info.updateVerBui);

        /* Get HTTP Attributes here for now, Display attributes */
        for( uint8_t i = 0; i < num_HTTP_ATTRIBUTES; i++ )
        {
            getAttributes(i,sharedAppAttr);
            /* Check if attribute value is valid */
            if(sharedAppAttr[0] != '\0')
            {
                /* For this app, only 1 attribute used, and its for cell transmission interval, overwrite this value */
                cellTransInt = ( uint32_t ) strtoul( sharedAppAttr, NULL, 10 );
                DBGI("New Cell Transmission Interval: %ds",cellTransInt);
            }
        }

        // Monitor FreeRTOS stack usage
        DBGI("FreeRTOS Heap Remaining: %d Bytes", xPortGetFreeHeapSize());

        /* Delay */
        vTaskDelay(pdMS_TO_TICKS(cellTransInt*1000));
    }

    vTaskDelete( NULL );
}
#endif
/*-----------------------------------------------------------*/

static void LEDTask( void * pvParameters )
{
    led_init();

    for(;;)
    {
        if( cellStatus == CellSuccess )
        {
            led_mode(LED_CELL_OK, true, 1);
        }
        else if( cellStatus == CellFailToSend || cellStatus == CellFailToRec )
        {
            led_mode(LED_FAIL_TO_SEND, true, 1);
        }
        else
        {
            led_mode(LED_FAIL_TO_REG, true, 1);
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    vTaskDelete( NULL );
}

/*-----------------------------------------------------------*/

void config_sc16_rs232 (void)
{
    //Set SC16 to default
    if(sc16_reset(&sc16))
    {
        DBGE("Error: sc16_reset");
    }

    //Set baud rate based on 1.8432 MHz crystal 
    if(sc16_configure_baud(&sc16, 115200, 1.8432))
    {
        DBGE("Error: sc16_configure_baud");
    }

    // Enable enhanced features bit (EFR)
    sc16_enhanced_reg_enable(&sc16, true);
    if(sc16_set_enhanced(&sc16, true))
    {
        DBGE("Error: sc16_set_enhanced");
    }
    sc16_enhanced_reg_enable(&sc16, false);

    //Set word length
    if(sc16_set_comm_format(&sc16, SC16_WORD_LEN_8_BITS, 0, 0, 0))
    {
        DBGE("Error: sc16_set_comm_format");
    }
    
    //Set up FIFO
    if(sc16_enable_fifo(&sc16, true))
    {
        DBGE("Error: sc16_enable_fifo");
    }
    
    if(sc16_rx_reset_fifo(&sc16))
    {
        DBGE("Error: sc16_rx_reset_fifo");
    }

    if(sc16_tx_reset_fifo(&sc16))
    {
        DBGE("Error: sc16_tx_reset_fifo");
    }
    
    if(sc16_interrupts_en(&sc16, 0x11))
    {
        DBGE("Error: sc16_interrupts_en");
    }
}

/*-----------------------------------------------------------*/
void sensorPwrEnConfig( bool status )
{
    static uint8_t counter = 0;
    static bool init = true;

    //If first pass then configure to output and create semaphore
    if(init)
    {
        xSensPwrEnSemaphore = xSemaphoreCreateBinary();
        nrf_gpio_cfg_output(PIN_NAME_SENSOR_POWER_ENABLE);
        //Clear flag
        init = false;
    }
    else
    {
        //Take semaphore
        xSemaphoreTake(xSensPwrEnSemaphore, pdMS_TO_TICKS(500));
    }

    //If status is false, and all tasks want this pin disabled, then clear pin
    if(status == false)
    {
        //Decrement counter if above zero
        if(counter > 0)
        {
            counter--;
        }
        //If counter is now 0, then disable
        if(counter == 0)
        {
            nrf_gpio_pin_clear(PIN_NAME_SENSOR_POWER_ENABLE);
        }
    }

    //If status is true then set pin and increase counter
    if(status == true)
    {
        nrf_gpio_pin_set(PIN_NAME_SENSOR_POWER_ENABLE);
        if(counter < 0xFF)
        {
            counter++;
        }
    }

    //Give semaphore
    xSemaphoreGive(xSensPwrEnSemaphore);
}

/*-----------------------------------------------------------*/

/**@brief Function for application main entry.
 */
int main(void)
{
    /* Miscellaneous initialization including preparing the logging and cell */
    prvMiscInitialization();

    // Start FreeRTOS scheduler.
    vTaskStartScheduler();

    for (;;)
    {
        APP_ERROR_HANDLER(NRF_ERROR_FORBIDDEN);
    }
}

/*-----------------------------------------------------------*/
// Initialization function
static void prvMiscInitialization( void )
{
    nrfx_err_t error;

    // Initialize bsp
    ep_bsp_init(WATCHDOG_RELOAD, WATCHDOG_RELOAD_RATE);

    // Initialize modules.
    init_uart(MAIN_LOOP);
    init_swo();
    uart_helper.dbgi = true;

    // Initialize time helper
    set_time(0);

    /* Check for version number in qpsi, otherwise use default */
    ImageDescriptor_t xDescriptor;
    ProductionTable_t xProdTable;

    //Enable sensor power
    sensorPwrEnConfig(true);

    /* Initialize qspi driver.*/
    nrfx_err_t err_code = qspi_init();

    if( err_code != NRFX_SUCCESS )
    {
        DBGE( "Unable to initialize QSPI driver" );
    }

    nrf_delay_ms(pdMS_TO_TICKS(2500));

    err_code = qspi_read( (uint8_t * ) &xDescriptor, sizeof(xDescriptor), 0 );
    if( err_code != NRFX_SUCCESS )
    {
        DBGE( "Read failed with with error code %d", err_code );
    }

    if( xDescriptor.usImageFlags == otapalIMAGE_FLAG_VALID )
    {
        DBGI("Image Flag:%d",xDescriptor.usImageFlags);
        DBGI("Image Size:%d bytes",xDescriptor.ulImageSize);
        DBGI("Image Checksum:%x", xDescriptor.checksum);

        system_info.updateVerMaj = ( ( xDescriptor.updateVersion >> 24 ) & 0xFF );
        system_info.updateVerMin = ( ( xDescriptor.updateVersion >> 16 ) & 0xFF );
        system_info.updateVerBui = ( xDescriptor.updateVersion & 0xFFFF );
        sprintf( system_info.updateVerStr,"%02x.%02x.%02x", system_info.updateVerMaj, system_info.updateVerMin, system_info.updateVerBui );
    }
    else
    {
        /* Set integer version */
        system_info.updateVerMaj = VERSION_MAJOR;
        system_info.updateVerMin = VERSION_MINOR;
        system_info.updateVerBui = VERSION_BUILD;
        /* Set string version */
        sprintf( system_info.updateVerStr,"%02x.%02x.%02x", system_info.updateVerMaj, system_info.updateVerMin, system_info.updateVerBui );
    }

    /* Read production table in external flash */
    err_code = qspi_read( (uint8_t * ) &xProdTable, sizeof(xProdTable), otapal_PROD_TBL_START );
    if( err_code != NRFX_SUCCESS )
    {
        DBGE( "Prod Table read failed with with error code %d", err_code );
    }
    else
    {
        /* Use production table values in external flash */
        if( strncmp( xProdTable.serialNumber, "\xFF\xFF\xFF\xFF\xFF\xFF", sizeof( xProdTable.serialNumber ) ) != 0 )
        {
            memcpy( system_info.ep_serial, xProdTable.serialNumber, sizeof( xProdTable.serialNumber ) );
            DBGI("Device SN: %s", system_info.ep_serial);
        }
        /* Use default production table values, if needed */
        else
        {
            DBGE("Invalid Production Table, Programming Defaults");
        }
    }

    /* Uninit the qspi driver */
    qspi_uninit();

    DBGI("*************************************************");
    DBGI("* Embedded Planet CV FreeRTOS Example v%s *", system_info.updateVerStr);
    DBGI("*************************************************");

    //Enable the CAN interface
    nrf_gpio_cfg_output(CAN_PWR_ENABLE);
    nrf_gpio_pin_set(CAN_PWR_ENABLE);
    
    //Release the MCP1515 reset
    nrf_gpio_cfg_output(CAN_RESET);
    nrf_gpio_pin_set(CAN_RESET);

    //Init nRF TWI interface
    twi_init();
    nrf_delay_ms(200);

    //For the connected vehicle the MC33397 must be disabled: ~GPO_EN and the ~GPO_CS set high
    // to keep the part from holding the SPI bus hostage
    //The MCP23008 driver is used to set GP6 and GP7 high

    //Initialize mcp struct with the proper settings. In this case, the twi0 interface that we 
    //configured in twi_init, at device address 39. 
    mcp23008_init(&mcp, 39, twi);

    //Set input pins (all others are outputs)
    mcp23008_direction(&mcp, MCP23008_GPIO0 | MCP23008_GPIO1 | MCP23008_GPIO2 | MCP23008_GPIO3 | MCP23008_GPIO5 );

    //To turn on output GPIO 4:
    mcp23008_write_pin(&mcp, MCP23008_GPIO4, 1);

    //To turn on output GPIO 6:
    mcp23008_write_pin(&mcp, MCP23008_GPIO6, 1);

    //To turn on output GPIO 7:
    mcp23008_write_pin(&mcp, MCP23008_GPIO7, 1);

    //Config interrupt pin to be open drain
    mcp23008_interrupt_open_drain(&mcp, true);

    //Config interrupt to be active low
    mcp23008_interrupt_polarity(&mcp,0);

    //Initialize sc16 struct with the proper settings. In this case, the twi0 interface that we 
    //configured in twi_init, at device address 9A. 
    sc16_init(&sc16, 0x9A, twi);

    //Set up the sc16 for RS232 communication
    config_sc16_rs232();

#if OBDII_ACTIVE == 1
    // OBDII Tasks and timer
    xTaskCreate(obdii_task_function,
                "OBDIIreadtask",
                2500,
                NULL,
                tskIDLE_PRIORITY+2,
                NULL);
#endif

#if J1939_ACTIVE == 1
    // J1939 Tasks and timer
    xTaskCreate(j1939_task_function,
                "J1939readtask",
                2000,
                NULL,
                tskIDLE_PRIORITY+4,
                NULL);

    /* Set interrupt pin for J1939 */
    nrf_gpio_cfg_input(MCP2515_INTERRUPT, NRF_GPIO_PIN_PULLUP);
#endif

    /* Cell needs a couple of seconds before starting */
    nrf_delay_ms(2000);

    DBGI( ( "Cell Task Initializing...\r\n" ) );

    cellularSemaphore = xSemaphoreCreateBinary();
    if(!cellularSemaphore){
        DBGE("cellularSemaphore creation failed!");
    }

    //Setup cellular data queue
    xCellQueue = xQueueCreate(CELL_QUEUE_SIZE, sizeof(cell_queue_msg));
    if(xCellQueue == NULL){
        DBGE("xCellQueue creation failed!");
    }

    #if defined(THINGSBOARD_HTTPS_INTEGRATION) || defined(AZURE_MQTTS_X509) || defined(AWS_MQTTS_X509) || defined(AWS_HTTPS_X509) || defined(AZURE_MQTTS_SAS) || defined(AZURE_HTTPS_SAS)
    //If protocol is MQTT, then initialize crypto here
    if( nrf_crypto_init() != NRF_SUCCESS)
    {
        DBGE(("Failed to initialize nrf crypto"));
    }
    #endif

    /* Create the task to run tests. */
    xTaskCreate( CellularTask,
                "CellularTask",
                mainCell_TASK_STACK_SIZE,
                NULL,
                tskIDLE_PRIORITY+2,
                &CellTaskHandle );

    /* Create the task to run tests. */
    xTaskCreate( LEDTask,
                "LEDTask",
                mainLED_TASK_STACK_SIZE,
                NULL,
                tskIDLE_PRIORITY+1,
                &ledTaskHandle );

    /* Disable two wire interface */
    twi_uninit();
    
    // Put uart and sensor_en to sleep
    uninit_uart(MAIN_LOOP);
    sensorPwrEnConfig(false);
}