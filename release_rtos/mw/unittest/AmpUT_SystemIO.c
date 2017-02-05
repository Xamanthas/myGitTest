/**
 * @file src/app/sample/unittest/AmpUT_SystemIO.c
 *
 * System IO unit test
 *
 * History:
 *    2014/08/14 - [Edgar Lai] created file
 *
 * Copyright (C) 2014, Ambarella, Inc.
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
#include "AmbaPLL.h"
#include "AmbaPLL_Def.h"

int SystemIOTest_PllCmd(AMBA_SHELL_ENV_s *env, int argc, char **argv)
{
    if (strcmp(argv[2], "core") == 0) {
        if (strcmp(argv[3], "set") == 0) {
            if (argc < 5) {
                return -1;
            } else {               
                UINT32 Freq = atoi(argv[4]);
                
                AmbaPrint("[Set]Core : %dMhz", Freq);
                Freq *= 1000000; 
                AmbaPLL_SetCoreClk(Freq);
                
                Freq = AmbaPLL_GetCoreClk();
                AmbaPrint("[Get]Core : %dMhz", Freq/1000000);
            }
        } else if (strcmp(argv[3], "get") == 0) {
            UINT32 Freq;

            Freq = AmbaPLL_GetCoreClk();
            AmbaPrint("[Get]Core : %dMhz", Freq/1000000);
        } else if (strcmp(argv[3], "sweep") == 0) {
            if (argc < 7) {
                return -1;
            } else {
                UINT32 Start = atoi(argv[4]);
                UINT32 End = atoi(argv[5]);
                UINT32 Int = atoi(argv[6]);
                UINT32 ActualFreq = 0;
                
                if (Start > End) {
                    AmbaPrint("[Sweep]Core : Wrong Boundary (%d -> %d)", Start, End);
                    return -1;
                }
                
                for (UINT32 i = Start; i <= End; i+=Int) {
                    AmbaPLL_SetCoreClk(i*1000000);
                    ActualFreq = AmbaPLL_GetCoreClk();
                    AmbaPrint("[Sweep]Core %dMhz, Actual %dMhz", i, ActualFreq/1000000);
                    AmbaKAL_TaskSleep(10);
                }
            }
        }
    } else if (strcmp(argv[2], "idsp") == 0) {
        if (strcmp(argv[3], "set") == 0) {
            UINT32 Freq = atoi(argv[4]);

            AmbaPrint("[Set]Idsp : %dMhz", Freq);
            Freq *= 1000000; 
            AmbaPLL_SetIdspClk(Freq);

            Freq = AmbaPLL_GetIdspClk();
            AmbaPrint("[Get]Idsp : %dMhz", Freq/1000000);
        } else if (strcmp(argv[3], "get") == 0) {
            UINT32 Freq;

            Freq = AmbaPLL_GetIdspClk();
            AmbaPrint("[Get]Idsp : %dMhz", Freq/1000000);
        } else if (strcmp(argv[3], "sweep") == 0) {
            UINT32 Start = atoi(argv[4]);
            UINT32 End = atoi(argv[5]);
            UINT32 Int = atoi(argv[6]);
            UINT32 ActualFreq = 0;

            if (Start > End) {
                AmbaPrint("[Sweep]Idsp : Wrong Boundary (%d -> %d)", Start, End);
                return -1;
            }

            for (UINT32 i = Start; i <= End; i+=Int) {
                AmbaPLL_SetIdspClk(i*1000000);
                ActualFreq = AmbaPLL_GetIdspClk();
                AmbaPrint("[Sweep]Idsp %dMhz, Actual %dMhz", i, ActualFreq/1000000);
                AmbaKAL_TaskSleep(10);
            }
        }
    } else {
        AmbaPrint(" SystemIOTest PllCmd Usage: t systemio pll [item] [cmd]");
        AmbaPrint("     [item]  : which item to be operated");
        AmbaPrint("               - core : core freq");
        AmbaPrint("               - idsp : idsp freq");
        AmbaPrint("     [cmd]   : operations");
        AmbaPrint("               - set : set freq");
        AmbaPrint("               - get : get freq");
        AmbaPrint("               - sweep : sweep freq");
        AmbaPrint("                 - [strat] [end] [int] : start/end freq in Mhz, interval frequency in Mhz");

    }
    return 0;
}

int AmpUT_SystemIOTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("AmpUT_SystemTest cmd: %s", argv[1]);

    if (argc < 2) {
        goto Error;
    }

    if (strcmp(argv[1], "pll") == 0) {
        SystemIOTest_PllCmd(env, argc, argv);
    } else if (strcmp(argv[1], "pllop") == 0) {
        AMBA_OPMODE_CLK_PARAM_s SystemClock;
        UINT32 IdspFreq = atoi(argv[2]);
        UINT32 CoreFreq = atoi(argv[3]);
        UINT32 CortexFreq = atoi(argv[4]);
        AmbaPrint("[Desire]IDSP   freq: %d", IdspFreq);
        AmbaPrint("[Desire]Core   freq: %d", CoreFreq);
        AmbaPrint("[Desire]Cortex freq: %d", CortexFreq);
        SystemClock.IdspClkFreq = IdspFreq;
        SystemClock.CoreClkFreq = CoreFreq;
        SystemClock.CortexClkFreq = CortexFreq;
        AmbaPLL_SetOpMode(&SystemClock);
        AmbaPrint("[Actual]IDSP   freq: %d", AmbaPLL_GetIdspClk());
        AmbaPrint("[Actual]Core   freq: %d", AmbaPLL_GetCoreClk());
        AmbaPrint("[Actual]Cortex freq: %d", AmbaPLL_GetCortexClk());
    } else {
Error:
        AmbaPrint("Usage: t systemio [func]");
        AmbaPrint("       pll: pll category");
        AmbaPrint("       pllop: pll opmode setting");
    }

    return 0;
}

int AmpUT_SystemIOTestAdd(void)
{
    AmbaPrint("Adding AmpUT_SystemIO");

    // hook command
    AmbaTest_RegisterCommand("sio", AmpUT_SystemIOTest);
    AmbaTest_RegisterCommand("systemio", AmpUT_SystemIOTest);


    return AMP_OK;
}
