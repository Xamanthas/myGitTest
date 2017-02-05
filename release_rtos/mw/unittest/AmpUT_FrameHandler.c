 /**
  * @file \src\unittest\MWUnitTest\AmpUT_YUVTask.c
  *
  * YUV task unit test
  *
  * History:
  *    2014/11/18 - [Bill Chou] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#include "AmpUnitTest.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vin/vin.h>
#include <recorder/Encode.h>
#include <recorder/VideoEnc.h>
#include <util.h>
#include "AmbaUtility.h"
#include "AmbaTimer.h"
//#include "Config.h"
#include "ambava.h"
#include "ambava_frmhdlr.h"
#include "ambava_3ahdlr.h"
extern AMBA_KAL_BYTE_POOL_t G_MMPL;
static UINT8 YuvStack[FRM_HDLR_DEF_TASK_STACK_SIZE];            /**< Stack for Yuvinput */
static UINT8 TriAStack[TRIA_HDLR_DEF_TASK_STACK_SIZE];            /**< Stack for 3ainput */

int AmpUT_FrameHandlerTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("AmpUT_FrameHandlerTest cmd: %s", argv[1]);
    if ( strcmp(argv[1],"yuvt") == 0 ) {
        {
            AMBA_VA_CFG cfg= {0};
            AmbaVA_FrmHdlr_GetDefCfg(&cfg);
            cfg.TaskStack = YuvStack;
            AmbaVA_FrmHdlr_Init(&cfg);
        }

    } else if ( strcmp(argv[1],"3at") == 0 ) {
        {
            AMBA_TRIA_CFG t3cfg= {0};
            AmbaVA_TriAHdlr_GetDefCfg(&t3cfg);
            t3cfg.TaskStack = TriAStack;
            AmbaVA_TriAHdlr_Init(&t3cfg);
        }
    } else if ( strcmp(argv[1],"prt") == 0 ) {

    }
    else if ( strcmp(argv[1],"release") == 0 ) {

        AmbaPrint("release done !\n");
    } else {
        AmbaPrint("Usage: t fh cmd ...");
        AmbaPrint("    cmd:");
        AmbaPrint("       yuvt: yuv frame handler task init");
        AmbaPrint("       3at: 3a handler task init");
    }
    return 0;
}

int AmpUT_FrameHandlerTestAdd(void)
{
    AmbaPrint("Adding AmpUT_FrameHandlerTestAdd");
    // hook command
    AmbaTest_RegisterCommand("frmhdlr", AmpUT_FrameHandlerTest);
    AmbaTest_RegisterCommand("fh", AmpUT_FrameHandlerTest);
    return AMP_OK;
}

