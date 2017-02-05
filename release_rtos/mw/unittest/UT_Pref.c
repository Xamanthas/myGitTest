 /**
  * @file mw/unittest/UT_Pref.c
  *
  * Unit test preference API implementation
  *
  * Copyright (C) 2014, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#include "UT_Pref.h"
#include <AmbaNFTL.h>

//#define PrefDBG(Str, ...) {AmbaPrint("[UT_Pref] [%s]: " Str, __FUNCTION__, ##__VA_ARGS__);}
#define PrefDBG(...)
#define PrefMSG(Str, ...) {AmbaPrint("[UT_Pref] [%s]: " Str, __FUNCTION__, ##__VA_ARGS__);}

static UINT8 Pref[UT_PREF_NUM];
static UINT8 Pref_Init = 0;

static int Pref_Read(UINT8 *pBuf)
{
    int Rval = 0;

    Rval = AmbaNFTL_Read(NFTL_ID_PRF, pBuf, 0, 1);
    PrefDBG("%d", Rval);
    return Rval;
}

static int Pref_Write(UINT8 *pBuf)
{
    int Rval = 0;

    Rval = AmbaNFTL_Write(NFTL_ID_PRF, pBuf, 0, 1);
    return Rval;
}

/**
 * Initialize pref.
 *
 * @return 0 - OK, others - NG
 */
int UT_Pref_Init(void)
{
    int Rval = OK;
    if ((Pref_Init == 0) && (AmbaNFTL_IsInit(NFTL_ID_PRF) != OK)) {
        memset(Pref, 0, sizeof(Pref));
        Rval = AmbaNFTL_Init(NFTL_ID_PRF, NFTL_MODE_NO_SAVE_TRL_TBL);
        Rval = Pref_Read(Pref);
        if (Rval == OK) {
            Pref_Init = 1;
            PrefDBG("NAND Init OK" );
        } else {
            PrefDBG("NAND Init NG" );
            Rval = -1;
        }
    } else {
        PrefDBG("NANS Init OK" );
    }
    PrefMSG("%d", Rval);
    return Rval;
}

/**
 * Get the value of pref with the specified item ID.\n
 * Note that pref is 255 when NAND is erased before boot.\n
 * User have to handle the case that item value is not defined.
 *
 * @param [in] PrefItem     Item ID
 *
 * @return -1 - NG, others - Item value
 */
int UT_Pref_Check(UT_PREF_e PrefItem)
{
    int Rval = OK;
    if (Pref_Init == 0)
        Rval = UT_Pref_Init();
    if (PrefItem < UT_PREF_NUM) {
        Rval = Pref[PrefItem];
        PrefDBG("%d", PrefItem);
    } else {
        PrefDBG("%d out of define", PrefItem);
        Rval = -1;
    }
    return Rval;
}

/**
 * Set the value of pref with the specified item ID.
 *
 * @param [in] PrefItem     Item ID
 * @param [in] ItemVal      Item value
 *
 * @return 0 - OK, others - NG
 */
int UT_Pref_Set(UT_PREF_e PrefItem, UINT8 ItemVal)
{
    int Rval = OK;
    if (Pref_Init == 0)
        Rval = UT_Pref_Init();
    if (PrefItem < UT_PREF_NUM) {
        PrefDBG("%d -> %d", Pref[PrefItem], ItemVal);
        Pref[PrefItem] = ItemVal;
        Rval = Pref_Write(Pref);
        PrefDBG("Pref_Write %d", Rval);
    } else {
        PrefDBG("%d out of define", PrefItem);
        Rval = -1;
    }
    return Rval;
}
