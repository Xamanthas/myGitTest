/**
  * @file app/connected/app/system/app_pref.c
  *
  * Implementation of Application Preference
  *
  * History:
  *    2013/08/16 - [Martin Lai] created file
  *    2013/12/27 - [Hsunying Huang] modified
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */
#include <string.h>
#include <stdio.h>
#include <AmbaDataType.h>
#include <AmbaNAND_Def.h>
#include <AmbaNFTL.h>
#include "app_pref.h"
#include "status.h"
#ifdef CONFIG_ENABLE_EMMC_BOOT
#include <AmbaPartition_Def.h>
#endif

//#define APP_PREF_DEBUG
#ifdef APP_PREF_DEBUG
#define AppPrefPrint        AmbaPrint
#else
#define AppPrefPrint(...)
#endif

// Unit: Bytes
#define SYSTEM_PREF_SIZE    (512)
static UINT8 DefaultTable[SYSTEM_PREF_SIZE] = {
    /* System */
    0x00, 0x10, 0x00, 0x00,
    /* Video */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Photo */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    /* Image */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Audio */
    0x00, 0x00, 0x00, 0x00,
    /* Playback*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    /* Setup */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x41, 0x41, 0x7D, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* VA*/
    0x00, 0x37, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Reserved*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static UINT8 SettingTable[SYSTEM_PREF_SIZE];
APP_PREF_USER_s *UserSetting;

/*************************************************************************
 * App status definitons
 ************************************************************************/
APP_STATUS_s app_status;

/*************************************************************************
 * App preference APIs
 ************************************************************************/
#ifdef CONFIG_ENABLE_EMMC_BOOT
    extern int AmbaEMMC_MediaPartRead(int ID, UINT8 *pBuf, UINT32 Sec, UINT32 Secs);
    extern int AmbaEMMC_MediaPartWrite(int ID, UINT8 *pBuf, UINT32 Sec, UINT32 Secs);
#endif

/**
 *  @brief Check the version of preference
 *
 *  Check the version of preference
 *
 *  @param[in] Ori the original preference data
 *  @param[in] New the new preference data
 *
 *  @return 1 the same, 0 NOT the same version
 */
static UINT8 AppPref_IsSameVersion(UINT8* ori, UINT8* new)
{
    UINT8 Ret = 0;
    UINT16 SysVerOld = 0;
    UINT16 SysVernew = 0;

    SysVerOld = (ori[1] << 8) + ori[0];
    SysVernew = (new[1] << 8) + new[0];
    if (SysVernew != SysVerOld) {
        AppPrefPrint("[APP PREF] System version doesn't match.");
        AppPrefPrint("[APP PREF] Old System version: %d.%d.%d",
                      GET_VER_MAJOR(SysVerOld),
                      GET_VER_MINOR(SysVerOld),
                      GET_VER_PATCH(SysVerOld));
        AppPrefPrint("[APP PREF] Default System version: %d.%d.%d",
                      GET_VER_MAJOR(SysVernew),
                      GET_VER_MINOR(SysVernew),
                      GET_VER_PATCH(SysVernew));
        Ret = 0;
    } else {
        AppPrefPrint("[APP PREF] System version is the same.");
        Ret = 1;
    }
    return Ret;
}

static int _AppPref_Init(void)
{
    int Rval = 0;

#if defined(CONFIG_ENABLE_EMMC_BOOT)
    // nothing
#else
    if (AmbaNFTL_IsInit(NFTL_ID_PRF) != OK) {
        #if 0
        NandRval = AmbaNFTL_InitLock(NFTL_ID_PRF);
        if (NandRval == OK) {
            AppPrefPrint("[PREF] Init OK" );
        } else {
            AppPrefPrint("[PREF] Init NG" );
        }
        #endif
        Rval = AmbaNFTL_Init(NFTL_ID_PRF, NFTL_MODE_NO_SAVE_TRL_TBL);
        if (Rval == OK) {
            AppPrefPrint("[APP PREF] NAND Init OK" );
        } else {
            AppPrefPrint("[APP PREF] NAND Init NG" );
            Rval = -1;
        }
    } else {
        AppPrefPrint("[APP PREF] NANS Init OK" );
    }
#endif
    return Rval;
}

static int _AppPref_Read(UINT8 *pBuf)
{
    int Rval = 0;

#ifdef CONFIG_ENABLE_EMMC_BOOT
    Rval = AmbaEMMC_MediaPartRead(MP_UserSetting, pBuf, 0, 1);
#else
    Rval = AmbaNFTL_Read(NFTL_ID_PRF, pBuf, 0, 1);
#endif
    return Rval;
}

static int _AppPref_Write(UINT8 *pBuf)
{
    int Rval = 0;

#ifdef CONFIG_ENABLE_EMMC_BOOT
    Rval |= AmbaEMMC_MediaPartWrite(MP_UserSetting, pBuf, 0, 1);
#else
    Rval |= AmbaNFTL_Write(NFTL_ID_PRF, pBuf, 0, 1);
#endif
    return Rval;
}

/**
 *  @brief Initialize the preferece part in NAND
 *
 *  Initialize the preferece part in NAND
 *
 *  @return >=0 valid item, <0 failure
 */
INT8 AppPref_InitPref(void)
{
    INT8 ReturnValue = 0;

    AppPrefPrint("[APP PREF] _AppPref_InitPref");
    UserSetting = (APP_PREF_USER_s *)&SettingTable;
    _AppPref_Init();
    return ReturnValue;
}

/**
 *  @brief Load the preferece data from NAND
 *
 *  Load the preferece data from NAND
 *
 *  @return PREF_RET_STATUS_LOAD_RESET re-load happened, PREF_RET_STATUS_LOAD_NORMAL load normal
 */
APP_PREF_RET_STATUS_e AppPref_Load(void)
{
    APP_PREF_RET_STATUS_e ReturnValue = PREF_RET_STATUS_LOAD_NORMAL;
    UINT8 reload = 0;
    int NandRval = OK;
    UINT32 idx = 0;

    /* Load data from NAND */
    NandRval = _AppPref_Read(SettingTable);

    AppPrefPrint("=== Load (%d Bytes) ===", sizeof(SettingTable));
    for (idx=0; idx<sizeof(SettingTable); idx+=4) {
        AppPrefPrint("0x%X, 0x%X, 0x%X, 0x%X", SettingTable[idx], SettingTable[idx+1], SettingTable[idx+2], SettingTable[idx+3]);
    }

    if (NandRval == OK) {
        AppPrefPrint("[APP PREF] load success");
        /* Check whether the firmware is updated */
        if (AppPref_IsSameVersion(DefaultTable, SettingTable)) {
            reload = 0;
        } else {
            reload = 1;
        }
    } else {
        AppPrefPrint("[APP PREF] load fail");
        reload = 1;
    }

    /* Save and re-Load the user preference */
    if (reload) {
        NandRval = OK;
        AppPrefPrint("[APP PREF] Save default preference");
        NandRval = _AppPref_Write(DefaultTable);

        if (NandRval == OK) {
            AppPrefPrint("[APP PREF] save success");
        } else {
            AppPrefPrint("[APP PREF] save fail");
            K_ASSERT(NandRval >= 0);
        }

        AppPrefPrint("[APP PREF] Re-load default preference");
        NandRval = _AppPref_Read(SettingTable);

        if (NandRval == OK) {
            AppPrefPrint("[APP PREF] load success");
        } else {
            AppPrefPrint("[APP PREF] load fail");
            K_ASSERT(NandRval >= 0);
        }
    }

    if (reload) {
        ReturnValue = PREF_RET_STATUS_LOAD_RESET;
    } else {
        ReturnValue = PREF_RET_STATUS_LOAD_NORMAL;
    }
    return ReturnValue;
}

/**
 *  @brief Save the preferece data to NAND
 *
 *  Save the preferece data to NAND
 *
 *  @return >=0 valid item, <0 failure
 */
INT8 AppPref_Save(void)
{
    INT8 ReturnValue = 0;
    int NandRval = OK;

    /** Save and re-Load the user preference */
    NandRval = OK;
    AppPrefPrint("[APP PREF] Save preference");

    NandRval = _AppPref_Write(SettingTable);

    if (NandRval == OK) {
        UINT32 idx = 0;
        ReturnValue = 0;

        AppPrefPrint("[APP PREF] save success");
        AppPrefPrint("=== Write (%d Bytes) ===", sizeof(SettingTable));
        for (idx=0; idx<sizeof(SettingTable); idx+=4) {
            AppPrefPrint("0x%X, 0x%X, 0x%X, 0x%X", SettingTable[idx], SettingTable[idx+1], SettingTable[idx+2], SettingTable[idx+3]);
        }
    } else {
        AppPrefPrint("[APP PREF] save fail");
        ReturnValue = -1;
    }
    return ReturnValue;
}

