# MCP2515Driver

This is a driver for the Microchip MCP2515 SPI to CAN controller. It utilizes the nRF SDK for use with the nRF52XXX series SoCs.

**Note:** There is a known issue with the example code on Connected Vehicle DES0277_12/13 where the power-down test will report an error. This issue needs to be investigated further, but the power enable pins were all determined to be behaving correctly. The issue likely has to do with the change from SPI to SPIM or other recent changes to the MCP2515 driver.  

## Contents
**mcp2515.h** - Driver header

## Set up
1. Include **mcp2515.h** in the necessary source files
2. Add the include path for **mcp2515.h** to your Makefile
3. Add the path for the associated static library to your Makefile

## Use
Detailed use can be found in the header file. 
```C++
/* Create empty MCP struct */
MCP2515 mcp2515;

/* Initialize mcp struct with the proper settings */
bool ret = mcp2515_init(mcp2515,spim);
if (ret == false)
{
	DBGE("mcp2515_init failure");
}

//Set the addressing to use extended (29bit) encoding/decoding
ret = mcp2515_set_address_mode(mcp2515,CAN_ADDRESS_MODE);
if (ret == false)
{
	DBGE("mcp2515_set_address_mode failure");
}

//Set CAN speed
ret = mcp2515_can_speed_write (mcp2515, 20000000, 500000);
if (ret == false)
{
	DBGE("mcp2515_can_speed_write failure");
}

//Once the device has been configured, set the device is set to "normal mode"
mcp2515_write_reg(mcp2515, MCP2515_CANCTRL, MCP2515_CANCTRL_REQOP_NORMAL);
```

## Versions
V1.0.0 Initial Release