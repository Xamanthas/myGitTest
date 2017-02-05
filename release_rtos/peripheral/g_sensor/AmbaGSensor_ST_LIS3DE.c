/**
 * @file rtos/peripheral/g_sensor/AmbaGSensor_ST_LIS3DE.c
 *
 * This driver is for ST LIS3DE G-Sensor.
 *
 * History:
 *    2015/05/20 [Yuchi Wei] - created file
 *
 * Copyright (C) 2009-2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <AmbaDataType.h>
#include <AmbaKAL.h>
#include <AmbaPrint.h>
#include <AmbaPrintk.h>
#include <AmbaUART.h>

#include <string.h>
#include <stdlib.h>

#include "AmbaI2C.h"
#include "AmbaGSensor.h"
#include "AmbaGSensor_ST_LIS3DE.h"

//#define AMBA_GSENSOR_DBG
#ifdef AMBA_GSENSOR_DBG
#define GSENSOR_DBG   AmbaPrint
#else
#define GSENSOR_DBG(...)
#endif

static AMBA_G_SENSOR_INFO_s LIS3ED_Info = {
	.Interface = G_SENSOR_DIGITAL_INTERFACE_I2C,
	.Axis = G_SENSOR_3_AXIS,
	.Polarity = G_SENSOR_NEGATIVE_POLARITY,
	.MaxSense = 16383,
	.MaxBias = 65535,
	.AdcResolution = 16,
	.SampleCycle = 100,

	.I2CCHN = AMBA_I2C_CHANNEL0,
	.I2CAddr = GSENSOR_ST_LIS3DE_I2C_ADDR,
	.MaxReg = MAX_REG,
};

int GSENSOR_ST_LIS3DE_GetInfo(AMBA_G_SENSOR_INFO_s *Info)
{
	int res = NG;

	if(Info != NULL){
		memcpy((void*)Info, &LIS3ED_Info, sizeof(LIS3ED_Info));
		res = OK;
	}

	return res;
}

int GSENSOR_ST_LIS3DE_SetInfo(AMBA_G_SENSOR_INFO_s *Info)
{
	int res = NG;

	if(Info != NULL){
		memcpy((void*)(&LIS3ED_Info), Info, sizeof(LIS3ED_Info));
		res = OK;
	}

	return res;
}

int GSENSOR_ST_LIS3DE_Read(UINT16 Addr, UINT16 *Data)
{
	UINT8 RxDataBuf = 0;
	UINT16 WorkUINT16[3];

	WorkUINT16[0] = AMBA_I2C_RESTART_FLAG | LIS3ED_Info.I2CAddr; /* Slave Address + r/w (0) */
	WorkUINT16[1] = Addr;											         /* Sub Address */
	WorkUINT16[2] = AMBA_I2C_RESTART_FLAG | LIS3ED_Info.I2CAddr | 0x01;

	if (AmbaI2C_ReadAfterWrite(LIS3ED_Info.I2CCHN, AMBA_I2C_SPEED_FAST,
							   3, WorkUINT16, 1, &RxDataBuf, 5000) != OK) {
		GSENSOR_DBG("[%s] fail", __func__);
		return NG;
	}else{
		*Data = (UINT16)RxDataBuf;
		GSENSOR_DBG("[%s] read 0x%x data:0x%x", __func__, Addr, RxDataBuf);
	}

	return OK;
}

int GSENSOR_ST_LIS3DE_Write(UINT16 Addr, UINT16 Data)
{
	UINT8 TxDataBuf[2];
	int rval = OK;

	TxDataBuf[0] = (UINT8)Addr;
	TxDataBuf[1] = (UINT8)(Data);

	GSENSOR_DBG("[%s]: Addr: 0x%02x, Data: 0x%02x", __func__, Addr, Data);

	if(AmbaI2C_Write(LIS3ED_Info.I2CCHN,
	                                    AMBA_I2C_SPEED_FAST,
						 LIS3ED_Info.I2CAddr,
						 2,
						 TxDataBuf,
						 5000) != OK){
		GSENSOR_DBG("[%s]: Addr: 0x%02x, Data: 0x%02x fail", __func__, Addr, Data);
		rval = NG;
	}

	return rval;

}

int GSENSOR_ST_LIS3DE_Init(void)
{
	UINT16 Data;

	if((GSENSOR_ST_LIS3DE_Read( REG_WHO_AM_I, &Data) != OK) || (Data != WHO_AM_I_ID)){
		GSENSOR_DBG("[%s]: Addr: 0x%02x, Data: 0x%02x", __func__, REG_WHO_AM_I, (UINT8)Data);
		return NG;
	};

	GSENSOR_ST_LIS3DE_Read( REG_CTRL1, &Data);
	Data = ODR_100HZ|Z_EN|Y_EN|X_EN;
	GSENSOR_ST_LIS3DE_Write( REG_CTRL1, Data);

	GSENSOR_ST_LIS3DE_Read( REG_CTRL4, &Data);
	Data = BDU|FS_2G|HR_MODE;
	GSENSOR_ST_LIS3DE_Write( REG_CTRL4, Data);

	return OK;
}

static int GSENSOR_ST_LIS3DE_enable_interrupt(int sensitivity){

	UINT16 Data;

	GSENSOR_ST_LIS3DE_Read( REG_CTRL5, &Data);
	Data = Data|0x08;
	GSENSOR_ST_LIS3DE_Write( REG_CTRL5, Data);

	Data = I2_INT2 | HL_ACTIVE;
	GSENSOR_ST_LIS3DE_Write( REG_CTRL6, Data);
	Data = YHIE_YUPE | YHIE_XUPE;
	GSENSOR_ST_LIS3DE_Write( REG_INT1_CFG, Data);

	switch(sensitivity){
	case 0:
		Data = 0;
		GSENSOR_ST_LIS3DE_Write( REG_CTRL6, Data);
		Data = 0;
		GSENSOR_ST_LIS3DE_Write( REG_INT1_CFG, Data);
		break;
	case 1:
		Data = 0x46;
		GSENSOR_ST_LIS3DE_Write( REG_INT1_THS, Data);
		Data = 0;
		GSENSOR_ST_LIS3DE_Write( REG_INT1_DURA, Data);
		break;
	case 2:
		Data = 0x56;
		GSENSOR_ST_LIS3DE_Write( REG_INT1_THS, Data);
		Data = 0;
		GSENSOR_ST_LIS3DE_Write( REG_INT1_DURA, Data);
		break;
	case 3:
		Data = 0x66;
		GSENSOR_ST_LIS3DE_Write( REG_INT1_THS, Data);
		Data = 0;
		GSENSOR_ST_LIS3DE_Write( REG_INT1_DURA, Data);
		break;
	default:
		break;
	}
	return OK;
}

static int GSENSOR_ST_LIS3DE_disable_interrupt(void){

	UINT16 Data;
	Data = 0;
	GSENSOR_ST_LIS3DE_Write( REG_CTRL6, Data);
	Data = 0;
	GSENSOR_ST_LIS3DE_Write( REG_INT1_CFG, Data);
	return OK;
}


static int GSENSOR_ST_LIS3DE_poll_interrupt_status(void)
{
	int rval = -1;
	UINT16 Data;
	GSENSOR_ST_LIS3DE_Read( REG_INT1_SRC, &Data);
	if(Data&IA)
	rval = OK;
	return rval;
}


AMBA_G_SENSOR_OBJ_s AmbaGSensor_LIS3DEObj = {
	.Init       = GSENSOR_ST_LIS3DE_Init,
	.GetInfo = GSENSOR_ST_LIS3DE_GetInfo,
	.SetInfo = GSENSOR_ST_LIS3DE_SetInfo,
	.Write    = GSENSOR_ST_LIS3DE_Write,
	.Read    = GSENSOR_ST_LIS3DE_Read,
	.enable_interrupt = GSENSOR_ST_LIS3DE_enable_interrupt,
	.disable_interrupt    = GSENSOR_ST_LIS3DE_disable_interrupt,
	.poll_interrupt_status    = GSENSOR_ST_LIS3DE_poll_interrupt_status,
};


