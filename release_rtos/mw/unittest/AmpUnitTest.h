 /**
  * @file src/app/sample/unittest/AmpUnitTest.h
  *
  * Unit test header
  *
  * History:
  *    2013/02/25 - [Peter Weng] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#ifndef AMPUNITTEST_H_
#define AMPUNITTEST_H_

#include <stdio.h>
#include <string.h>

#include "AmbaDataType.h"
#include "AmbaRTSL_GPIO.h"

#include "AmbaKAL.h"

#include "AmbaUART_Def.h"
#include "AmbaUART.h"
#include "AmbaGPIO.h"

#include <AmbaPrint.h>

#include <AmbaShell.h>
#include <AmbaTest.h>
#include <AmbaFS.h>
#include <AmbaAudio.h>

#include "AmbaRTSL_MMU.h"

#include <mw.h>
#include <fifo/Fifo.h>
#include <display/Display.h>
#include "AmbaNAND_Def.h"
#include "AmbaNFTL.h"
#include "cfs/AmpCfs.h"
//#include "extSrc/sample_calib/vignette/AmbaUTCalibVig.h"

#ifdef CONFIG_SOC_A12
#if (defined(CONFIG_DRAM_DDR3_K4B4G1646BHCK0_16BIT_396MHZ) || \
    defined(CONFIG_DRAM_DDR3_K4B4G1646BHCK0_32BIT_396MHZ) || \
    defined(CONFIG_DRAM_DDR3_CT49488DD962D1_32BIT_396MHZ)) //0.9V in MHz unit
    #define A12_IDSP_PLL_MAX    288
    #define A12_CORE_PLL_MAX    288
    #define A12_CORTEX_PLL_MAX  504
#else //1.0V in MHz unit
    #define A12_IDSP_PLL_MAX    504
    #define A12_CORE_PLL_MAX    396
    #define A12_CORTEX_PLL_MAX  792
#endif
#endif

#pragma pack(1)
typedef struct _NHNT_HEADER_s_{
    char Signature[4];
    UINT64 Version:8;
    UINT64 StreamType:8;
    UINT64 ObjectTypeIndication:8;
    UINT64 Reserved:16;
    UINT64 BufferSizeDB:24;
    UINT32 AvgBitRate;
    UINT32 MaxBitRate;
    UINT32 TimeStampResolution;
}NHNT_HEADER_s;
#pragma pack()

#pragma pack(1)
typedef struct _NHNT_SAMPLE_HEADER_s_{
    UINT32 DataSize:24;
    UINT32 RandomAccessPoint:1;
    UINT32 AuStartFlag:1;
    UINT32 AuEndFlag:1;
    UINT32 Reserved:1;//3->1
    UINT32 FrameType:4;// 2->4
//    UINT32 mixFields;
    UINT32 FileOffset;
    UINT32 CompositionTimeStamp;
    UINT32 DecodingTimeStamp;
}NHNT_SAMPLE_HEADER_s;
#pragma pack()

typedef struct _FORMAT_USER_DATA_s_{
    UINT32 nIdrInterval;
    UINT32 nTimeScale;
    UINT32 nTickPerPicture;
    UINT32 nN;
    UINT32 nM;
    UINT32 nVideoWidth;
    UINT32 nVideoHeight;
    UINT32 nInterlaced;
} FORMAT_USER_DATA_s;

typedef struct {
    UINT32 SampleRate;
    UINT16 Channels;
    UINT16 BitsPerSample;
    UINT16 Frame_Size;
    UINT32 Bitrate;
	UINT16 reserve;
} AUDIO_USER_DATA_s;

extern AMBA_KAL_BYTE_POOL_t G_MMPL;
extern AMBA_KAL_BYTE_POOL_t G_NC_MMPL;

// Device handler for every unit test
extern AMP_DISP_HDLR_s *LCDHdlr;
extern AMP_DISP_HDLR_s *TVHdlr;
extern void* dispModuleMemPool;

extern UINT8 *dspWorkAreaResvStart;
extern UINT8 *dspWorkAreaResvLimit;
extern UINT32 dspWorkAreaResvSize;

extern AMP_CFS_EVENT_HDLR_FP CfsEventHandler;

#define AMP_UT_CFS_MAX_STREAM_NUM   3
#define AMP_UT_CFS_FILE_PER_STREAM  3
#define AMP_UT_CFS_BANK_PER_FILE    2

#endif /* AMPUNITTEST_H_ */

