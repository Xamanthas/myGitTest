/**
 *  @file AmpUT_DummyDec.c
 *  
 *  **History**
 *      |Date       |Name        |Comments       |
 *      |-----------|------------|---------------|
 *      |2013/3/25 |cyweng      |Created        |
 *
 *  @copyright 2013 Ambarella Corporation. All rights reserved.
 *                 No part of this file may be reproduced, stored in a retrieval system,
 *                 or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                 recording, or otherwise, without the prior consent of Ambarella Corporation.
 */

#include "AmpUnitTest.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <AmbaGDMA.h>
#include <util.h>

#define ALUOpTaskName "MapPrfALUOpTask"
static AMBA_KAL_TASK_t ALUOpTask = {0};
static char ALUOpStack[1024];

#define MemOpTaskName "MapPrfMemOpTask"
static AMBA_KAL_TASK_t MemOpTask = {0};
static char MemOpStack[1024];

#define GDMAOpTaskName "MapPrfGDMAOpTask"
static AMBA_KAL_TASK_t GDMAOpTask = {0};
static char GDMAOpStack[1024];

UINT8* MemOpTaskRawBuf;
UINT8* GDMAOpTaskRawBuf;


static int GDMAOpPri = 0, MemOpPri = 0, ALUOpPri = 0;
static int GDMAOpLog = 0, MemOpLog = 0;

static int GDMACnt = 3102, MemCnt = 100, ALUCnt = 10000;

static UINT32 a, b, c, d, f;
void AmpUT_Map_ALUOpTask(UINT32 info)
{
    int t;
    AmbaPrint("AmpUT_Map_ALUOpTask");
    while (1) {
        for(t=0;t<ALUCnt;t++) {
            a=a+b;
            b=b*c;
            if (c!=0) d=a/c;
            f = d - a;
        }
        AmbaKAL_TaskSleep(1);
    }
}

void AmpUT_Map_MemOpTask(UINT32 info)
{
    int t;
    int cnt = 0;
    AmbaPrint("AmpUT_Map_MemOpTask");
    while (1) {
        for(t=0;t<MemCnt;t++) {
            memcpy((void*)((UINT32)MemOpTaskRawBuf), (void*)((UINT32)MemOpTaskRawBuf+(1<<19)), 128<<10 );           
            cnt++;
            if (cnt == (1<<20)/128) {
                cnt = 0;
                if (MemOpLog)
                    AmbaPrint("AmpUT_Map_MemOpTask move 1000MB");
            }
        }
        AmbaKAL_TaskSleep(1);
    }
    AmbaPrint("AmpUT_Map_MemOpTaskEnd");
}

void AmpUT_Map_GDMAOpTask(UINT32 info)
{
    AMBA_GDMA_LINEAR_CTRL_s GDMACtrl;
    int t;
    int cnt = 0;

    AmbaPrint("AmpUT_Map_GDMAOpTask");
    GDMACtrl.PixelFormat = AMBA_GDMA_8_BIT;
    GDMACtrl.NumPixels = 128<<10;
    
    while (1) {
        for(t=0;t<GDMACnt;t++) {
          GDMACtrl.pDest = (void*)((UINT32)GDMAOpTaskRawBuf);
            GDMACtrl.pSrc = (void*)((UINT32)GDMAOpTaskRawBuf+(1<<19));
            AmbaGDMA_LinearCopy(&GDMACtrl, 100);
            cnt++;
            if (cnt == (2<<20)/128) {
                cnt = 0;
                if (GDMAOpLog)
                    AmbaPrint("AmpUT_Map_GDMAOpTask move 1000MB");
            }
        }
        AmbaKAL_TaskSleep(1);
    }
}


int AmpUT_MapInit(void)
{
    UINT8 *Pool;
    AmpUtil_GetAlignedPool(&G_NC_MMPL, (void**)&MemOpTaskRawBuf, (void**)&Pool, 1<<20 , 64);
    AmpUtil_GetAlignedPool(&G_NC_MMPL, (void**)&GDMAOpTaskRawBuf, (void**)&Pool, 1<<20 , 64);

    if (GDMAOpPri) {
        AmbaKAL_TaskCreate(&GDMAOpTask, /* pTask */
        GDMAOpTaskName, /* pTaskName */
        GDMAOpPri, /* Priority */
        AmpUT_Map_GDMAOpTask, /* void (*EntryFunction)(UINT32) */
        0x0, /* EntryArg */
        (void *) GDMAOpStack, /* pStackBase */
        1024, /* StackByteSize */
        AMBA_KAL_DO_NOT_START); /* AutoStart */
    }

    if (MemOpPri) {
        AmbaKAL_TaskCreate(&MemOpTask, /* pTask */
        MemOpTaskName, /* pTaskName */
        MemOpPri, /* Priority */
        AmpUT_Map_MemOpTask, /* void (*EntryFunction)(UINT32) */
        0x0, /* EntryArg */
        (void *) MemOpStack, /* pStackBase */
        1024, /* StackByteSize */
        AMBA_KAL_DO_NOT_START); /* AutoStart */
    }

    if (ALUOpPri) {
        AmbaKAL_TaskCreate(&ALUOpTask, /* pTask */
        ALUOpTaskName, /* pTaskName */
        ALUOpPri, /* Priority */
        AmpUT_Map_ALUOpTask, /* void (*EntryFunction)(UINT32) */
        0x0, /* EntryArg */
        (void *) ALUOpStack, /* pStackBase */
        1024, /* StackByteSize */
        AMBA_KAL_DO_NOT_START); /* AutoStart */
    }

    return 0;
}

void AmpUT_MapStart(void)
{
    if (GDMAOpPri) {
        AmbaKAL_TaskResume(&GDMAOpTask); /* pTask */
    }

    if (MemOpPri) {
        AmbaKAL_TaskResume(&MemOpTask); /* pTask */
    }

    if (ALUOpPri) {
        AmbaKAL_TaskResume(&ALUOpTask); /* pTask */
    }
}

int AmpUT_MapTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("AmpUT_MapTest cmd: %s", argv[1]);
    if ( strcmp(argv[1],"init") == 0) {
        GDMAOpPri = atoi(argv[2]);
        MemOpPri = atoi(argv[3]);
        ALUOpPri = atoi(argv[4]);
        AmpUT_MapInit();
    } else if ( strcmp(argv[1],"start") == 0) {
        AmpUT_MapStart();
    } else if ( strcmp(argv[1],"set") == 0) {
        GDMACnt = atoi(argv[2]);//% of cpu of zero load
        MemCnt = atoi(argv[3]);
        ALUCnt = atoi(argv[4]);
    } else if ( strcmp(argv[1],"log") == 0) {
        GDMAOpLog = atoi(argv[2]);//% of cpu of zero load
        MemOpLog = atoi(argv[3]);
    } else{
        //AmpUT_map_usage();
    }
    return 0;
}

int AmpUT_MapTestAdd(void)
{
    // hook command
    AmbaTest_RegisterCommand("map", AmpUT_MapTest);

    return AMP_OK;
}
