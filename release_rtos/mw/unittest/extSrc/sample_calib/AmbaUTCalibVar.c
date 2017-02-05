/**
 * @file mw/unittest/extSrc/sample_calib/AmbaSampleCalibVar.c
 *
 * sample code for register Calibration sites
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

#include "AmbaUTCalibMgr.h"

/* ---------- */
// The DRAM shadow of each item is put here
/* 00 */ static UINT8 DataStatus[CAL_STATUS_SIZE];
/* 01 */ static UINT8 DataAF[CAL_AF_SIZE];
/* 02 */ static UINT8 DataGyro[CAL_GYRO_SIZE];
/* 03 */ static UINT8 DataMshutter[CAL_MSHUTTER_SIZE];
/* 04 */ static UINT8 DataIris[CAL_IRIS_SIZE];
/* 05 */ static UINT8 DataVignette[CAL_VIGNETTE_SIZE];
/* 06 */ static UINT8 DataWarp[CAL_WARP_SIZE];
/* 07 */ static UINT8 DataBPC[CAL_BPC_SIZE];
/* 08 */ static UINT8 DataWB[CAL_WB_SIZE];
/* 09 */ static UINT8 DataIso[CAL_ISO_SIZE];
/* 10 */ static UINT8 DataBLC[CAL_BLC_SIZE];
/* 11 */ static UINT8 DataFlash[CAL_FLASH_SIZE];
/* 12 */ static UINT8 DataAudio[CAL_AUDIO_SIZE];
/* 13 */ static UINT8 DataCA[CAL_CA_SIZE];
/* 14 */ static UINT8 DataLensShift[CAL_LENSSHIFT_SIZE];



extern int AmpUTCalibVignette_Func(int argc, char *argv[], char *OutputStr, Cal_Stie_Status_s *CalSite, Cal_Obj_s *CalObj);
extern int AmpUTCalibVignette_Init (Cal_Obj_s *CalObj);
extern int AmpUTCalibBPC_Init(Cal_Obj_s *CalObj);
extern int AmpUTCalibBPC_Func(int argc, char *argv[], char *OutputStr, Cal_Stie_Status_s *CalSite, Cal_Obj_s *CalObj);
extern int AmpUTCalibBLC_Init(Cal_Obj_s *CalObj);
extern int AmpUTCalibBLC_Func(int Argc, char *Argv[], char *OutputStr, Cal_Stie_Status_s *CalSite, Cal_Obj_s *CalObj);
extern int AmpUTCalibBLC_Upgrade(Cal_Obj_s *CalObj, Cal_Stie_Status_s* CalSite);
extern int AmpUTCalibWB_Init(Cal_Obj_s *CalObj);
extern int AmpUTCalibWB_Func(int Argc, char *Argv[], char *OutputStr, Cal_Stie_Status_s *CalSite, Cal_Obj_s *CalObj);
extern int AmpUTCalibWB_Upgrade(Cal_Obj_s *CalObj, Cal_Stie_Status_s* CalSite);
extern int AmpUTCalibWarp_Init(Cal_Obj_s *CalObj);
extern int AmpUTCalibWarp_Func(int Argc, char *Argv[], char *OutputStr, Cal_Stie_Status_s *CalSite, Cal_Obj_s *CalObj);
extern int AmpUTCalibWarp_Upgrade(Cal_Obj_s *CalObj, Cal_Stie_Status_s* CalSite);
extern int AmpUTCalibCA_Init(Cal_Obj_s *CalObj);
extern int AmpUTCalibCA_Func(int Argc, char *Argv[], char *OutputStr, Cal_Stie_Status_s *CalSite, Cal_Obj_s *CalObj);
extern int AmpUTCalibCA_Upgrade(Cal_Obj_s *CalObj, Cal_Stie_Status_s* CalSite);
extern int AmpUTCalibAudio_Init(Cal_Obj_s *CalObj);
extern int AmpUTCalibAudio_Upgrade(Cal_Obj_s *CalObj, Cal_Stie_Status_s *CalSite);
extern int AmpUTCalibAudio_Func(int argc, char *Argv[], char *OutputStr, Cal_Stie_Status_s *CalSite, Cal_Obj_s *CalObj);

// Calibration manager working table
Cal_Obj_s CalObjTable[NVD_CALIB_MAX_OBJS];



/* ---------- */
// Ambarella's calibration hookup function

Cal_Obj_s CalObjAmbarella[] = {
    /* 00 */ {1, CAL_STATUS_ID,   CAL_STATUS_SIZE,   CAL_STATUS_VER,   "STATUS",   DataStatus,               NULL,                NULL,              NULL },
    /* 01 */ {0, CAL_AF_ID,       CAL_AF_SIZE,       CAL_AF_VER,       "AF",       DataAF,             NULL,       NULL,       NULL },
    /* 02 */ {0, CAL_GYRO_ID,     CAL_GYRO_SIZE,     CAL_GYRO_VER,     "GYRO",     DataGyro,             NULL,       NULL,       NULL },
    /* 03 */ {0, CAL_MSHUTTER_ID, CAL_MSHUTTER_SIZE, CAL_MSHUTTER_VER, "MSHUTTER", DataMshutter,             NULL,       NULL,       NULL },
    /* 04 */ {0, CAL_IRIS_ID,     CAL_IRIS_SIZE,     CAL_IRIS_VER,     "IRIS",     DataIris,             NULL,       NULL,       NULL },
    /* 05 */ {1, CAL_VIGNETTE_ID, CAL_VIGNETTE_SIZE, CAL_VIGNETTE_VER, "VIGNETTE", DataVignette, AmpUTCalibVignette_Init, NULL, AmpUTCalibVignette_Func },
    /* 06 */ {1, CAL_WARP_ID,     CAL_WARP_SIZE,     CAL_WARP_VER,     "WARP",     DataWarp,    AmpUTCalibWarp_Init,     AmpUTCalibWarp_Upgrade,AmpUTCalibWarp_Func },
    /* 07 */ {1, CAL_BPC_ID,      CAL_BPC_SIZE,      CAL_BPC_VER,      "BPC",      DataBPC,     AmpUTCalibBPC_Init,      NULL,      AmpUTCalibBPC_Func },
    /* 08 */ {1, CAL_WB_ID,       CAL_WB_SIZE,       CAL_WB_VER,       "WB",       DataWB,     AmpUTCalibWB_Init,  AmpUTCalibWB_Upgrade,   AmpUTCalibWB_Func},
    /* 09 */ {0, CAL_ISO_ID,      CAL_ISO_SIZE,      CAL_ISO_VER,      "ISO",      DataIso,             NULL,       NULL,       NULL },
    /* 10 */ {1, CAL_BLC_ID,      CAL_BLC_SIZE,      CAL_BLC_VER,      "BLC",      DataBLC,    AmpUTCalibBLC_Init,AmpUTCalibBLC_Upgrade, AmpUTCalibBLC_Func },
    /* 11 */ {0, CAL_FLASH_ID,    CAL_FLASH_SIZE,    CAL_FLASH_VER,    "FLASH",    DataFlash,             NULL,       NULL,       NULL },
    /* 12 */ {1, CAL_AUDIO_ID,    CAL_AUDIO_SIZE,    CAL_AUDIO_VER,    "AUDIO",    DataAudio, AmpUTCalibAudio_Init, AmpUTCalibAudio_Upgrade, AmpUTCalibAudio_Func },
    /* 13 */ {1, CAL_CA_ID,       CAL_CA_SIZE,       CAL_CA_VER,       "CA",       DataCA,             AmpUTCalibCA_Init,       AmpUTCalibCA_Upgrade,       AmpUTCalibCA_Func },
    /* 14 */ {0, CAL_LENSSHIFT_ID,CAL_LENSSHIFT_SIZE,CAL_LENSSHIFT_VER,"LENSSHIFT",DataLensShift,      NULL,       NULL,       NULL },
};

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibSiteRegister
 *
 *  @Description:: register calibration site
 *
 *  @Input      ::
 *          CalObj: calibration object
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          INT: OK: 0
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibSiteRegister(Cal_Obj_s *CalObj)
{
    UINT32 Index;

    Index = CalObj->Id;
    CalObjTable[Index] = *CalObj;

    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibSiteRegisterAmbarella
 *
 *  @Description:: register ambarella calibration site
 *
 *  @Input      ::
 *          None
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          INT: OK: 0
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibSiteRegisterAmbarella(void)
{
    UINT32 Index;
    UINT32 NumItem = sizeof(CalObjAmbarella) / sizeof(Cal_Obj_s);
    for (Index=0; Index<NumItem; Index++) {
        AmpUT_CalibSiteRegister(&CalObjAmbarella[Index]);
    }
    return 0;
}


/* ---------- */
// Calibration table management code

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibInitStage0
 *
 *  @Description:: initial calibration sites for stage 0
 *
 *  @Input      ::
 *          None
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          INT: OK: 0
\*-----------------------------------------------------------------------------------------------*/
void AmpUT_CalibInitStage0(void)
{

    AmpUT_CalibInitFunc(CAL_STATUS_ID,CALIB_LOAD,CALIB_FULL_LOAD);
    AmpUT_CalibInitFunc(CAL_VIGNETTE_ID,CALIB_LOAD,CALIB_FULL_LOAD);
    AmpUT_CalibInitFunc(CAL_WARP_ID,CALIB_LOAD,CALIB_FULL_LOAD);
    AmpUT_CalibInitFunc(CAL_CA_ID,CALIB_LOAD,CALIB_FULL_LOAD);

}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibInitStage1
 *
 *  @Description:: initial calibration sites for stage 1
 *
 *  @Input      ::
 *          None
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          INT: OK: 0
\*-----------------------------------------------------------------------------------------------*/
void AmpUT_CalibInitStage1(void)
{
    AmpUT_CalibInitFunc(CAL_VIGNETTE_ID,CALIB_INIT,0);
    AmpUT_CalibInitFunc(CAL_WARP_ID,CALIB_INIT,0);
    AmpUT_CalibInitFunc(CAL_CA_ID,CALIB_INIT,0);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibInitStage2
 *
 *  @Description:: initial calibration sites for stage 2
 *
 *  @Input      ::
 *          None
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          INT: OK: 0
\*-----------------------------------------------------------------------------------------------*/
void AmpUT_CalibInitStage2(void)
{
    AmpUT_CalibInitFunc(CAL_BPC_ID,CALIB_LOAD,CALIB_FULL_LOAD);
    AmpUT_CalibInitFunc(CAL_WB_ID,CALIB_LOAD,CALIB_FULL_LOAD);
    AmpUT_CalibInitFunc(CAL_BLC_ID,CALIB_LOAD,CALIB_FULL_LOAD);

    AmpUT_CalibInitFunc(CAL_WB_ID,CALIB_INIT,0);
    AmpUT_CalibInitFunc(CAL_BLC_ID,CALIB_INIT,0);
    AmpUT_CalibInitFunc(CAL_BPC_ID,CALIB_INIT,0);

}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmpUT_CalibSiteInit
 *
 *  @Description:: register calibration site
 *
 *  @Input      ::
 *          None
 *
 *  @Output     ::
 *          None
 *  @Return     ::
 *          INT: OK: 0
\*-----------------------------------------------------------------------------------------------*/
int AmpUT_CalibSiteInit(void)
{
    // Register Ambarella's calibration sites
    AmpUT_CalibSiteRegisterAmbarella();
    //initial Nand
    AmpUT_CalibNandInit();

    return 0;
}
