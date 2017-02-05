 /**
  * @file \src\unittest\MWUnitTest\AmpUT_YUVReadSD.h
  *
  * header of YUV Reader from SD card
  *
  * History:
  *    2015/05/05 - [Bill Chou] created file
  *
  * Copyright (C) 2015, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */
#ifndef AMPUT_YUVREADSD_H_
#define AMPUT_YUVREADSD_H_

#include "AmpUnitTest.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vin/vin.h>
#include <recorder/Encode.h>
#include <recorder/VideoEnc.h>
#include <util.h>
#include "AmbaKAL.h"
#include "AmbaCache_Def.h"
#include "AmbaDataType.h"
#include "AmbaUtility.h"
#include "ambava.h"

#define SD_FRAME_BUFFER_TOTALSIZE (7000432)
#define SD_FRAME_BUFFER_NUM (6)
#define SD_FRAME_MAX_FILENUM (16)
#define SDFRM_READER_MAX_CB  (16)

typedef struct AMBAYUV_HEADER_t_{
    ushort Width;
    ushort Height;
    ushort FPS;
    char YuvType[12];
    short ZeroPading[7];
} AMBAYUV_HEADER_t; /// AMBAYUV_HEADER_t shoud be uint16 * 16 size

typedef int (*SD_FRMRdr_CB)(UINT32,
                             AMBAVA_YUV_INFO_s*);

typedef struct _SD_CB_SLOT_t_ {
    SD_FRMRdr_CB Func;
    UINT32 Event;
} SD_CB_SLOT_t;

typedef struct _G_ReadSD_t_ {
    UINT8 Init;
    AMBA_KAL_TASK_t Task;
    AMBA_KAL_MUTEX_t Mutex;
    AMBA_KAL_MSG_QUEUE_t MsgQueue;
    SD_CB_SLOT_t CbFunc[SDFRM_READER_MAX_CB];
} G_ReadSD_t;

/// API
int AmbaVa_GetTheCurren_SDreadName(char** Des);
int AmbaSD_FrmReader_Register(UINT32 event, SD_FRMRdr_CB func);

#endif
