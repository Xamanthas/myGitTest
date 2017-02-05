/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaImageMainEis.h
 *
 *  @Copyright      :: Copyright (C) 2013 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Constants and Definitions for Amba Main Eis
 *
 *  @History        ::
 *      Date        Name        Comments
 *      01/27/2015  Sean Tsao   Created
 *
\*-------------------------------------------------------------------------------------------------------------------*/

#ifndef _AMBA_IMAGE_MAIN_EIS_H_
#define _AMBA_IMAGE_MAIN_EIS_H_

//#define AMBA_EIS_IMU_API
#ifdef AMBA_EIS_IMU_API
#include "AmbaIMU.h"
#endif

typedef enum _AMBA_IMAGE_MAIN_EIS_IMU_READ_MODE_e_ {
    EIS_IMU_READ_REG = 0,
    EIS_IMU_READ_FIFO,
    EIS_IMU_READ_TOTAL
} AMBA_IMAGE_MAIN_EIS_IMU_READ_MODE_e;

#ifdef AMBA_EIS_IMU_API
typedef AMBA_IMAGE_MAIN_EIS_IMU_DATA_s AMBA_IMU_DATA_s;
#else
/*\
 *  pseudo code for testing only, will be removed after AmbaIMU ready.
\*/
typedef struct _AMBA_IMAGE_MAIN_EIS_THREE_AXIS_DATA_s_ {
    UINT32 X;
    UINT32 Y;
    UINT32 Z;
} AMBA_IMAGE_MAIN_EIS_THREE_AXIS_DATA_s;

typedef AMBA_IMAGE_MAIN_EIS_THREE_AXIS_DATA_s    AMBA_IMAGE_MAIN_EIS_GYRO_DATA_s;
typedef AMBA_IMAGE_MAIN_EIS_THREE_AXIS_DATA_s    AMBA_IMAGE_MAIN_EIS_ACCEL_DATA_s;
typedef AMBA_IMAGE_MAIN_EIS_THREE_AXIS_DATA_s    AMBA_IMAGE_MAIN_EIS_MAG_DATA_s;

typedef struct _AMBA_IMAGE_MAIN_EIS_TEMP_DATA_s_ {
    UINT32 Value;
} AMBA_IMAGE_MAIN_EIS_TEMP_DATA_s;

typedef struct _AMBA_IMAGE_MAIN_EIS_IMU_DATA_s_ {
    UINT32                              FrameSync;
    AMBA_IMAGE_MAIN_EIS_GYRO_DATA_s     Gyro;
    AMBA_IMAGE_MAIN_EIS_ACCEL_DATA_s    Acceleration;
    AMBA_IMAGE_MAIN_EIS_MAG_DATA_s      Magnet;
    AMBA_IMAGE_MAIN_EIS_TEMP_DATA_s     Temperature;
} AMBA_IMAGE_MAIN_EIS_IMU_DATA_s;
#endif

typedef struct _AMBA_IMAGE_MAIN_EIS_SYNC_DATA_s_ {
    UINT32  Sof;
    UINT32  Pts;
} AMBA_IMAGE_MAIN_EIS_SYNC_DATA_s;

typedef struct _AMBA_IMAGE_MAIN_IMU_INFO_s_ {
    AMBA_IMAGE_MAIN_EIS_SYNC_DATA_s     Sync;
    AMBA_IMAGE_MAIN_EIS_IMU_DATA_s      Data;
} AMBA_IMAGE_MAIN_IMU_INFO_s;

typedef enum _AMBA_IMAGE_MAIN_EIS_CMD_e_ {
    AMBA_EIS_CMD_STOP = 0,
    AMBA_EIS_CMD_START,
    AMBA_EIS_CMD_LOCK,
    AMBA_EIS_CMD_UNLOCK,
    AMBA_EIS_CMD_PAUSE,
    AMBA_EIS_CMD_TOTAL
} AMBA_IMAGE_MAIN_EIS_CMD_e;

int AmbaEis_TaskCreate(UINT32 Priority, UINT32 CoreExclusion);
int AmbaEis_TaskDelete(void);

void AmbaImageMainEis_Init(UINT32 Priority, UINT32 CoreInclusion);
void AmbaImageMainEis_Algo2Init(UINT32 Priority, UINT32 CoreInclusion);

int AmbaImageMainEis_Active(void);
int AmbaImageMainEis_Inactive(void);
int AmbaImageMainEis_IdleWait(void);

int AmbaImageMainEis_Enable(void);
int AmbaImageMainEis_Disable(void);

int AmbaImageMainEis_Lock(void);
int AmbaImageMainEis_PauseWait(void);
int AmbaImageMainEis_Unlock(void);
int AmbaImageMainEis_ResumeWait(void);

#endif /* _AMBA_IMAGE_MAIN_EIS_H_ */
