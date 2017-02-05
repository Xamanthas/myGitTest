/**
  * @file src/app/sample/unittest/AmpUT_ExtEnc.c
  *
  * ext encode unit test
  *
  * History:
  *    2014/03/24 - [Wisdom Hung] created file
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
#include <recorder/Encode.h>
#include <recorder/ExtEnc.h>
#include <display/Osd.h>
#include <display/Display.h>
#include "AmbaUtility.h"
#include <cfs/AmpCfs.h>
#include <util.h>

static AMP_CFS_FILE_PARAM_s cfsParam;

void *UT_ExtEncodefopen(const char *pFileName, const char *pMode)
{
    if (pMode[0] == 'w' && pMode[1] == '+') {
        cfsParam.Mode = AMP_CFS_FILE_MODE_WRITE_READ;
    } else if (pMode[0] == 'w') {
        cfsParam.Mode = AMP_CFS_FILE_MODE_WRITE_ONLY;
    } else if (pMode[0] == 'r' && pMode[1] == '+') {
        cfsParam.Mode = AMP_CFS_FILE_MODE_READ_WRITE;
    } else if (pMode[0] == 'r') {
        cfsParam.Mode = AMP_CFS_FILE_MODE_READ_ONLY;
    } else if (pMode[0] == 'a' && pMode[1] == '+') {
        cfsParam.Mode = AMP_CFS_FILE_MODE_APPEND_READ;
    } else if (pMode[0] == 'a') {
        cfsParam.Mode = AMP_CFS_FILE_MODE_APPEND_ONLY;
    }

    if(AmbaFS_GetCodeMode() == AMBA_FS_UNICODE) {
        //strcpy(cfsParam.Filename, pFileName);
        //return (void *)AmpCFS_fopen(&cfsParam);
        AmbaPrint("[%s]: Unicode should be abandoned", __func__);
        return NULL;
    } else {
        strcpy((char *)cfsParam.Filename, pFileName);
        return (void *)AmpCFS_fopen(&cfsParam);
    }
}

int UT_ExtEncodefclose(void *pFile)
{
    return AmpCFS_fclose((AMP_CFS_FILE_s *)pFile);
}

UINT64 UT_ExtEncodefwrite(const void *pBuf, UINT64 Size, UINT64 Count, void *pFile)
{
    return AmpCFS_fwrite(pBuf, Size, Count, (AMP_CFS_FILE_s *)pFile);
}

int UT_ExtEncodefsync(void *pFile)
{
    return AmpCFS_FSync((AMP_CFS_FILE_s *)pFile);
}

// global var for ExtEnc
static AMP_AVENC_HDLR_s *ExtEncHdlr = NULL;
static AMP_ENC_PIPE_HDLR_s *ExtEncPipe = NULL;
static UINT8 *ExtMainWorkBuf = NULL;
#define STATUS_EXT_NONE     1
#define STATUS_EXT_IDLE     2
#define STATUS_EXT_ENCODE   3
static UINT8 ExtStatus = STATUS_EXT_NONE;

/* muxer */
#define EXTENC_INFO_SIZE 128
#define EXTENC_BISFIFO_SIZE EXTENC_INFO_SIZE*32
static UINT8 *ExtBitsBuf;
static UINT8 *OriExtBitsBuf;
#define EXTENC_DESC_SIZE 8*32
static UINT8 *ExtDescBuf;
static UINT8 *OriExtDescBuf;


static AMP_FIFO_HDLR_s *ExtEncVirtualFifoHdlr = NULL;
static AMBA_KAL_TASK_t ExtEncMuxTask = {0};
static AMBA_KAL_SEM_t ExtEncSem = {0};
static AMP_CFS_FILE_s *OutputPriFile = NULL;
static char *AmpUT_ExtEncStack;
static int Fno = 0;
static UINT8 LogMuxer = 0;

#define EXTENC_STACK_SIZE 0x2000

void AmpUT_ExtEnc_MuxTask(UINT32 info)
{
    AMP_BITS_DESC_s *desc;
    int er;
    UINT8 *bitsLimit = ExtBitsBuf + EXTENC_BISFIFO_SIZE - 1;

    AmbaPrint("AmpUT_ExtEnc Start");

    while (1) {
        AmbaKAL_SemTake(&ExtEncSem, AMBA_KAL_WAIT_FOREVER);
        if (OutputPriFile == NULL) {
            char fn[80];
            char mdASCII[3] = {'w','+','\0'};
            sprintf(fn,"C:\\Ext_%04d.ext", Fno);
            OutputPriFile = UT_ExtEncodefopen((const char *)fn, (const char *)mdASCII);
            AmbaPrint("%s opened", fn);
            Fno++;
            bitsLimit = ExtBitsBuf + EXTENC_BISFIFO_SIZE - 1;
        }

        er = AmpFifo_PeekEntry(ExtEncVirtualFifoHdlr, &desc, 0);
        if (er == 0) {
            if (LogMuxer) {
                AmbaPrint("Ext[%d] Ty 0x%X pts:%8lld 0x%08x 0x%X", desc->SeqNum, desc->Type, desc->Pts, desc->StartAddr, desc->Size);
                //AmbaPrint("Bits %s", desc->StartAddr);
            }
        } else {
            while (er != 0) {
                AmbaPrint("[Ext]Muxer PeekEntry: Empty...");
                AmbaKAL_TaskSleep(30);
                er = AmpFifo_PeekEntry(ExtEncVirtualFifoHdlr, &desc, 0);
            }
        }

        if (OutputPriFile) {
            //AmbaPrint("[Ext]Write: 0x%x sz %d limit %X,desc=0x%x",  desc->StartAddr, desc->Size, bitsLimit,desc);
            if ((desc->Type == AMP_FIFO_TYPE_EOS) || (desc->Size == AMP_FIFO_MARK_EOS)) {
                UT_ExtEncodefsync((void *)OutputPriFile);
                UT_ExtEncodefclose((void *)OutputPriFile);
                OutputPriFile = NULL;
                AmbaPrint("[Ext]Muxer met EOS");
            } else {
                if (desc->StartAddr + desc->Size <= bitsLimit) {
                    UT_ExtEncodefwrite((const void *)desc->StartAddr, 1, desc->Size, (void *)OutputPriFile);
                } else {
                    //AmbaPrint("[Pri]Muxer Wrap Around");
                    UT_ExtEncodefwrite((const void *)desc->StartAddr, 1, bitsLimit - desc->StartAddr + 1, (void *)OutputPriFile);
                    UT_ExtEncodefwrite((const void *)ExtBitsBuf, 1, desc->Size - (bitsLimit - desc->StartAddr + 1), (void *)OutputPriFile);
                }
            }
        }
        AmpFifo_RemoveEntry(ExtEncVirtualFifoHdlr, 1);
    }
}

static int AmpUT_ExtEnc_FifoCB(void *hdlr, UINT32 event, void* info)
{
    UINT32 *numFrames = info;
    AMBA_KAL_SEM_t *pSem;

    pSem = &ExtEncSem;

    //AmbaPrint("AmpUT_AudioEnc_FifoCB on Event: 0x%x 0x%x", event, *numFrames);
    if (event == AMP_FIFO_EVENT_DATA_READY) {
        int i;

        for(i = 0; i < *numFrames; i++) {
            AmbaKAL_SemGive(pSem);
        }
    }else if (event == AMP_FIFO_EVENT_DATA_EOS) {
        AmbaKAL_SemGive(pSem);
    }

    return 0;
}

/*~muxer*/
static int AmpUT_ExtEnc_event_Callback(void *hdlr,UINT32 event, void *info)
{
    switch (event) {
        case AMP_ENC_EVENT_DATA_OVER_RUNOUT_THRESHOLD:
            AmbaPrint("AmpUT_ExtEnc: !!!!!!!!!!! AMP_ENC_EVENT_DATA_OVER_RUNOUT_THRESHOLD !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_DATA_OVERRUN:
            AmbaPrint("AmpUT_ExtEnc: !!!!!!!!!!! AMP_ENC_EVENT_DATA_OVERRUN !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_DESC_OVERRUN:
            AmbaPrint("AmpUT_ExtEnc: !!!!!!!!!!! AMP_ENC_EVENT_DESC_OVERRUN !!!!!!!!!!");
            break;
        default:
            AmbaPrint("AmpUT_ExtEnc: Unknown %X info: %x", event, info);
            break;
    }
    return 0;
}

/*~feed Ext data*/
static char ext_info[EXTENC_INFO_SIZE];
static int AmpUT_ExtEnc_info_Callback(UINT32 *size, UINT8** ptr)
{
    //AmbaPrint("AmpUT_ExtEnc_info_Callback");
    memset(ext_info, 0, 128);
    sprintf(ext_info,"X0000.0000Y0000.0000Z0000.0000G0000.0000$GPRMC,000125,V,,,,,000.0,,280908,002.1,N*71~, %d  \n",AmbaSysTimer_GetTickCount());

    (*size) = strlen(ext_info);
    (*ptr) = (UINT8 *)ext_info;

    return 0;
}


int AmpUT_ExtEnc_Init(void)
{
    int er;
    void *TmpbufRaw = NULL;

    // Create simple muxer semophore
    if (AmbaKAL_SemCreate(&ExtEncSem, 0) != OK) {
        AmbaPrint("ExtEnc UnitTest: Semaphore creation failed");
    }

    // Create simple muxer task
    er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AmpUT_ExtEncStack, (void **)&TmpbufRaw, EXTENC_STACK_SIZE, 32);
    if (er != OK) {
        AmbaPrint("Out of memory for muxer stack!!");
    }

    if (AmbaKAL_TaskCreate(&ExtEncMuxTask, "Ext Encoder UnitTest Primary Muxing Task", 30, \
         AmpUT_ExtEnc_MuxTask, 0x0, AmpUT_ExtEncStack, EXTENC_STACK_SIZE, AMBA_KAL_AUTO_START) != OK) {
         AmbaPrint("ExtEnc UnitTest: Muxer task creation failed");
    }

    // Init EXTENC module
    {
        AMP_EXTENC_INIT_CFG_s encInitCfg;

        AmpExtEnc_GetInitDefaultCfg(&encInitCfg);
        if (ExtMainWorkBuf == NULL) {
            er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&ExtMainWorkBuf, (void **)&TmpbufRaw, encInitCfg.MemoryPoolSize, 32);
            if (er != OK) {
                AmbaPrint("Out of memory for ext!!");
            }
        }
        //AmbaPrintColor(BLUE,"[ExtEnc] AmpUT_ExtEnc_Init...%x",ExtMainWorkBuf);
        encInitCfg.MemoryPoolAddr = ExtMainWorkBuf;

        AmpExtEnc_Init(&encInitCfg);
    }

    return 0;
}

/* create ext pipe */
int AmpUT_ExtEnc_Create(void)
{
    int er;
    AMP_EXTENC_HDLR_CFG_s encCfg;

    if (ExtEncPipe) {
        AmbaPrint("[Ext] Current Not Support multi ext pipe 0x%X", ExtEncPipe);
        return -1;
    }

    // Create Ext encoder object
    AmpExtEnc_GetDefaultCfg(&encCfg);

    // Task priority
    encCfg.EncoderTaskPriority = 15;
    encCfg.InputTaskPriority = 12;
    encCfg.EventDataReadySkipNum = 0;
    encCfg.MainTaskInfo.Priority = 31;
    encCfg.MainTaskInfo.StackSize = EXTENC_STACK_SIZE;
    encCfg.rate = 1000;
    encCfg.scale = 1000;

    // Assign callback
    encCfg.cbExtGetInfo = AmpUT_ExtEnc_info_Callback;
    encCfg.cbEvent = AmpUT_ExtEnc_event_Callback;

    // Assign bitstream/descriptor buffer
    er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&ExtBitsBuf, (void **)&OriExtBitsBuf, EXTENC_BISFIFO_SIZE, 32);
    if (er != OK) {
        AmbaPrint("Out of Cache memory for bitsFifo!!");
    }
    encCfg.BitsBufCfg.BitsBufAddr = ExtBitsBuf;
    encCfg.BitsBufCfg.BitsBufSize = EXTENC_BISFIFO_SIZE;

    er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&ExtDescBuf, (void **)&OriExtDescBuf, EXTENC_DESC_SIZE, 32);
    if (er != OK) {
        AmbaPrint("Out of Cache memory for descFifo!!");
    }

    encCfg.BitsBufCfg.DescBufAddr = ExtDescBuf;
    encCfg.BitsBufCfg.DescBufSize = EXTENC_DESC_SIZE;
    encCfg.BitsBufCfg.BitsRunoutThreshold = EXTENC_BISFIFO_SIZE - (EXTENC_BISFIFO_SIZE*20/100); // leave 20%
    AmbaPrint("[Pri]Bits 0x%X size %d Desc 0x%X size %d", ExtBitsBuf, EXTENC_BISFIFO_SIZE, ExtDescBuf, EXTENC_DESC_SIZE);

    // Assign bitstream-specific configs
    AmpExtEnc_Create(&encCfg, &ExtEncHdlr);


    // Register pipeline
    {
        AMP_ENC_PIPE_CFG_s pipeCfg;
        // Register pipeline, ext pipe can support up to 16 encoder
        AmpEnc_GetDefaultCfg(&pipeCfg);
        //pipeCfg.cbEvent
        pipeCfg.encoder[0] = ExtEncHdlr;
        pipeCfg.numEncoder = 1;
        pipeCfg.type = AMP_ENC_AV_PIPE;
        AmpEnc_Create(&pipeCfg, &ExtEncPipe);

        AmpEnc_Add(ExtEncPipe);
    }
    return 0;
}

int AmpUT_ExtEnc_EncodeStart(void)
{
    AMP_FIFO_CFG_s fifoDefCfg;

    if (ExtStatus != STATUS_EXT_IDLE)
        return -1;

    if (ExtEncVirtualFifoHdlr == NULL) {
        // create Primary virtual fifo
        AmpFifo_GetDefaultCfg(&fifoDefCfg);
        fifoDefCfg.hCodec = ExtEncHdlr;
        fifoDefCfg.IsVirtual = 1;
        fifoDefCfg.NumEntries = 1024;
        fifoDefCfg.cbEvent = AmpUT_ExtEnc_FifoCB;
        AmpFifo_Create(&fifoDefCfg, &ExtEncVirtualFifoHdlr);
    }

    AmpEnc_StartRecord(ExtEncPipe, 0);
    ExtStatus = STATUS_EXT_ENCODE;

    return 0;
}

int AmpUT_ExtEnc_EncStop(void)
{
    if (ExtStatus != STATUS_EXT_ENCODE)
        return -1;


    AmbaPrintColor(BLUE,"[ExtEnc] AmpUT_ExtEnc_EncStop Ext pipe...");
    AmpEnc_StopRecord(ExtEncPipe, 0);
    ExtStatus = STATUS_EXT_IDLE;

    return 0;
}

/* delete ext pipe */
int AmpUT_ExtEnc_Delete(void)
{

    AmpEnc_Delete(ExtEncPipe);
    ExtEncPipe = NULL;
    /* Free bits */
    AmbaKAL_BytePoolFree((void *)OriExtBitsBuf);
    ExtBitsBuf = NULL;
    AmbaKAL_BytePoolFree((void *)OriExtDescBuf);
    ExtDescBuf = NULL;

    return 0;
}

int AmpUT_ExtEncTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("AmpUT_ExtEncTest cmd: %s", argv[1]);
    if (strcmp(argv[1],"init") == 0) {
        AmpUT_ExtEnc_Init();
        ExtStatus = STATUS_EXT_IDLE;
    } else if ((strcmp(argv[1],"create") == 0) || (strcmp(argv[1],"cre") == 0)) {
        if (ExtStatus != STATUS_EXT_IDLE && \
            ExtStatus != STATUS_EXT_ENCODE) {
            AmbaPrint("[Ext] Non Init");
            return -1;
        }
        AmpUT_ExtEnc_Create();
    } else if ((strcmp(argv[1],"encstart") == 0) || (strcmp(argv[1],"enst") == 0)) {
        UINT32 tt = atoi(argv[2]);

        if (ExtStatus == STATUS_EXT_ENCODE) {
            AmbaPrint("[Ext] encode start during Encoding");
            return -1;
        }

        AmbaPrint("Encode for %d milliseconds", tt);
        AmpUT_ExtEnc_EncodeStart();
        if (tt != 0) {
            while (tt) {
                AmbaKAL_TaskSleep(1);
                tt--;
                if (tt == 0) {
                    AmpUT_ExtEnc_EncStop();
                    ExtStatus = STATUS_EXT_IDLE;
                }
                if (ExtStatus == STATUS_EXT_IDLE)
                    tt = 0;
            }
        }
    } else if ((strcmp(argv[1],"encstop") == 0) || (strcmp(argv[1],"ensp") == 0)) {
        if (ExtStatus != STATUS_EXT_ENCODE) {
            AmbaPrint("[Ext] encode stop duing IDLE");
            return -1;
        }
        AmpUT_ExtEnc_EncStop();
        ExtStatus = STATUS_EXT_IDLE;
    } else if (strcmp(argv[1],"delete") == 0) {
        if (ExtStatus == STATUS_EXT_ENCODE) {
            AmbaPrint("[Ext] delete during Encodeing !!");
            return -1;
        }

        AmpUT_ExtEnc_Delete();
    } else if (strcmp(argv[1],"logmuxer")==0) {
        LogMuxer = atoi(argv[2]);
        AmbaPrint("Log muxer: %s",LogMuxer?"ON":"OFF");
    } else {
        AmbaPrint("Usage: t extenc init|create|encstart|encstop");
        AmbaPrint("       init: init all");
        AmbaPrint("       create : create ext instance");
        AmbaPrint("       encstart [millisecond]: recording for N milliseconds");
        AmbaPrint("                               N = 0 will do continuous encode");
        AmbaPrint("       encstop: stop recording");
        AmbaPrint("       delete: delete ext instance");
    }
    return 0;
}

int AmpUT_ExtEncTestAdd(void)
{
    AmbaPrint("Adding AmpUT_ExtEnc");
    AmbaTest_RegisterCommand("extenc", AmpUT_ExtEncTest);
    return AMP_OK;
}

