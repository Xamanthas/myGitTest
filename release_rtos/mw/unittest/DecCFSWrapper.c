/**
  * @file DecCFSWrapper.c

  *
  * Implementation of video player module in application Library
  *
  * History:
  *    2014/10/6 - [cyweng] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#include "AmpUnitTest.h"
#include "AmbaUtility.h"
#include <cfs/AmpCfs.h>
#include <util.h>
#include <AmbaCache_Def.h>

static UINT8 DecCFSInit = 0;

static void InitDecCFS(void){
    //AMP_CFS_CFG_s cfsCfg;
    //void *cfshdlrAddr = NULL;

    //AmbaPrint("==%s==", __FUNCTION__);
    /** Cfs Init */
    // init at SystemInitTaskEntry
    /*
    if (AmpCFS_GetDefaultCfg(&cfsCfg) != AMP_OK) {
        AmbaPrint("%s: Can't get default config.", __FUNCTION__);
        return;
    }
    cfsCfg.BufferSize = AmpCFS_GetRequiredBufferSize(cfsCfg.SchBankSize, \
                                               cfsCfg.SchBankAmount, cfsCfg.SchStackSize, cfsCfg.SchTaskAmount, \
                                               cfsCfg.CacheEnable, cfsCfg.CacheMaxFileNum);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **) &cfsCfg.Buffer, &cfshdlrAddr, cfsCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != AMP_OK) {
        AmbaPrint("%s: Can't allocate memory.", __FUNCTION__);
        return;
    }
    if (AmpCFS_Init(&cfsCfg) != AMP_OK) {
        AmbaPrint("%s: Can't initialize.", __FUNCTION__);
        AmbaKAL_BytePoolFree(cfshdlrAddr);
        return;
    }
    */
    return;
}

AMP_CFS_FILE_s* AmpDecCFS_fopen(const char *pFile, const char *pMode)
{
    AMP_CFS_FILE_s *Rval = NULL;
    AMP_CFS_FILE_PARAM_s FileDesc = {0};
    UINT8 uMode;

    if (DecCFSInit == 0){
        DecCFSInit = 1;
        InitDecCFS();
    }

    /** Get file parameters */
    if (AmpCFS_GetFileParam(&FileDesc) != AMP_OK) {
        AmbaPrint("%s: Can't get file descriptor.", __FUNCTION__);
        return NULL;
    }

    switch (*pMode) {
    case 'r':
        uMode = AMP_CFS_FILE_MODE_READ_ONLY;
        break;
    case 'w':
        uMode = AMP_CFS_FILE_MODE_WRITE_ONLY;
        break;
    default:
        AmbaPrint("%s: Can't Open.", __FUNCTION__);
        goto Err;
        break;
    }

    /** Set file parameters */

#if 0
    {
    	WCHAR UnicodeFn[80] = {0};
        AmbaUtility_Ascii2Unicode(pFile, UnicodeFn);

    	w_strcpy(FileDesc.Filename, UnicodeFn);
		FileDesc.Mode = uMode;
		/** If the file is not opened, open it. */
		if (AmpCFS_FStatus(UnicodeFn) == AMP_CFS_STATUS_UNUSED) {
			Rval = AmpCFS_fopen(&FileDesc);
		} else {
			AmbaPrint("File is already opened!");
		}
    }
#else
    FileDesc.Mode = uMode;
    strcpy(FileDesc.Filename, pFile);
    /** If the file is not opened, open it. */
    if (AmpCFS_FStatus(pFile) == AMP_CFS_STATUS_UNUSED) {
        Rval = AmpCFS_fopen(&FileDesc);
    } else {
        AmbaPrint("File is already opened!");
    }
#endif
Err:
    return Rval;
}
