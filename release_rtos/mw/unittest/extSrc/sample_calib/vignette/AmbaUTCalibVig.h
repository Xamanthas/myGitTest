/**
 * @file mw/unittest/extSrc/sample_calib/vignette/AmbaUTCalibVig.h
 *
 * header file for vignette calibration
 *
 * History:
 *    07/10/2013  Allen Chiu Created
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include "../AmbaUTCalibMgr.h"

#define ANTI_FLICKER_60HZ 1
#define ANTI_FLICKER_50HZ 2


#define VIG_DEBUG_TABLE 0xF1

#define VIG_UPDATE_NONE         0x00
#define VIG_UPDATE_VIN_ENABLE   0x01
#define VIG_UPDATE_ALL          0x03
#define VIG_UPDATE_VIN_INFO     0x08



// Turn on the flag to replace Gr and Gb with (Gr+Gb)/2 before calibration */
// #define GR_GB_MISMATCH

/* Vignette compensation */
#define CAL_VIGNETTE_BASE        (0x00000000)
#define CAL_VIGNETTE_DATA(x)        ((x) + CAL_VIGNETTE_BASE)
#define CAL_VIGNETTE_ENABLE        CAL_VIGNETTE_DATA(0x00)
#define CAL_VIGNETTE_TABLE_COUNT    CAL_VIGNETTE_DATA(0x01)
#define CAL_VIGNETTE_CHECKSUM_BASE  CAL_VIGNETTE_DATA(0x04)
//prevent exceed 512byte Maximum:127 vig table
#define CAL_VIGNETTE_CHECKSUM(x)    (CAL_VIGNETTE_CHECKSUM_BASE+(x)*sizeof(UINT32))
#define CAL_VIGNETTE_TABLE_BASE        CAL_VIGNETTE_DATA(0x200)
#define CAL_VIGNETTE_TABLE(x)        (CAL_VIGNETTE_TABLE_BASE + sizeof(Vignette_Pack_Storage_s)*(x))

typedef struct Vignette_Select_Param_s_ {
    UINT8  Enable;
    UINT8  Channel;              // Channel no.
    UINT8  Id1;
    UINT8  Id2;
    INT32  Blend;
    UINT32 VignetteLumaStrength;
    UINT32 VignetteChromaStrength;
} Vignette_Select_Param_s;
//#define MAX_GAIN_TABLE_COUNT //define in <mw/image/image_algo_vignette_a5.h>
typedef struct Vignette_Control_s_ {
    UINT8  Enable;
    UINT8  GainTableCount;
    UINT8  Debug;
    UINT8  BufferLocked;
    Vignette_Pack_Storage_s *GainTable[MAX_VIGNETTE_GAIN_TABLE_COUNT];
} Vignette_Control_s;


extern Vignette_Control_s* AmpUTCalibVig_GetVigCalData(void);
extern Vignette_Control_s AppVignetteControl;
extern double pow(double x, double y);  /* For calculating gain_magnitude */
extern int AmpCalib_ConfigVignette(UINT8 Channel, Vignette_Global_Map_Config_Info_s *VignetteConfig);
extern int AmpCalib_VigSetEnable(UINT8 Enable);
extern int AmpUT_CalibSiteInit(void);
extern int AmpUTCalibVignette_SelectVignetteTable(UINT8 Enable, UINT8 Channel, UINT8 Id1, UINT8 Id2, int Blend, UINT32 VigLumaStr,UINT32 VigChromaStr);
extern int AmbaUT_CalibGetDspMode(AMBA_DSP_IMG_MODE_CFG_s *pMode);
extern UINT8  AmpUTCalibVignette_GetVignetteCalDataEnable(void);
extern int AmpUTCalibAdjust_Init(void);
extern void AmpUTCalibAdjust_SetControlEnable(UINT8 Enable);
extern void AmpUTCalibAdjust_Debug(UINT8 Enable);