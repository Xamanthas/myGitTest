/**
 *  @file AmpUT_DummyEnc.c
 *
 *  **History**
 *      |Date       |Name        |Comments       |
 *      |-----------|------------|---------------|
 *      |2013/3/15  |cyweng      |Created        |
 *
 *  @copyright 2013 Ambarella Corporation. All rights reserved.
 *                 No part of this file may be reproduced, stored in a retrieval system,
 *                 or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                 recording, or otherwise, without the prior consent of Ambarella Corporation.
 */

#include <recorder/Encode.h>
#include <recorder/DummyEnc.h>
#include "AmpUnitTest.h"
#include "AmbaUtility.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define DUMMYENC_RAW_SIZE (5<<20) // 5MB raw buffer
#define DUMMYENC_RAW_DESC_NUM (128) // descriptor number = 128

// global var for dmyenc
static void* rawBuf = NULL;
static void* descBuf = NULL;
static AMP_AVENC_HDLR_s *dmyEncHdlr = NULL;
static AMP_ENC_PIPE_HDLR_s *encPipeHdlr = NULL;
static AMP_FIFO_HDLR_s *dmyEncFifoHdlr = NULL;
static AMBA_KAL_TASK_t DummyEncMuxTask = {0};
static AMBA_KAL_MUTEX_t DummyEncFrmMutex = {0};
static UINT32 DummyEncFrmCnt = 0;

static UINT32 DummyEncRandSeed = 0x123cdaf;
static AMBA_FS_FILE *outputFile = NULL;

static char AmpUT_DummyEncStack[1024];
void AmpUT_DummyEnc_MuxTask(UINT32 info)
{
    AMP_BITS_DESC_s *desc;
    UINT32 totalSz = 0;
    UINT8* rawBufLimit = (UINT8*)((UINT32)rawBuf + (UINT32)DUMMYENC_RAW_SIZE - 1);

    AmbaPrint("AmpUT_DummyEnc_MuxTask Start");
    outputFile = AmbaFS_fopen("C:\\out.h264", "wb");

    srand(DummyEncRandSeed);
    while (1) {
        AmbaKAL_TaskSleep(50);
        AmbaKAL_MutexTake(&DummyEncFrmMutex, AMBA_KAL_WAIT_FOREVER);
        if (DummyEncFrmCnt == 0) {
            AmbaKAL_MutexGive(&DummyEncFrmMutex);
            continue;
        }
        DummyEncFrmCnt --;
        AmbaKAL_MutexGive(&DummyEncFrmMutex);
        //AmbaPrint("AmpUT_DummyEnc_MuxTask remain frm: %d", DummyEncFrmCnt);
        AmpFifo_PeekEntry(dmyEncFifoHdlr, &desc, 0);
        if (desc != NULL) {
            AmbaPrint("peeked frame: pts:%8lld size:%5d@0x%08x", desc->Pts, desc->Size, desc->StartAddr);
        }
        if (desc->Pts == 0xFFFFFF) {
            // EOS
            AmbaPrint("Muxer EOS!!!!!!!!!!!!!");
            break;
        }
        if (outputFile) {
            totalSz += desc->Size;
            AmbaPrint("Write to file:%8d   total:%8d", desc->Size, totalSz);
            if (desc->StartAddr + desc->Size <= rawBufLimit) {
                AmbaFS_fwrite(desc->StartAddr, desc->Size, 1, outputFile);
            }else{
                AmbaFS_fwrite(desc->StartAddr, rawBufLimit - desc->StartAddr + 1, 1, outputFile);
                AmbaFS_fwrite(rawBuf, desc->Size - (rawBufLimit - desc->StartAddr + 1), 1, outputFile);
            }
        }
        AmpFifo_RemoveEntry(dmyEncFifoHdlr, 1);
    }
    if (outputFile) {
        AmbaFS_fclose(outputFile);
    }
    outputFile = NULL;
}

int AmpUT_DummyEnc_FifoCB(void *hdlr, UINT32 event, void* info)
{
    AmbaPrint("AmpUT_DummyEnc_FifoCB on Event: 0x%x 0x%x", event, AMP_FIFO_EVENT_DATA_READY);
    if (event == AMP_FIFO_EVENT_DATA_READY) {
        AmbaKAL_MutexTake(&DummyEncFrmMutex, AMBA_KAL_WAIT_FOREVER);
        DummyEncFrmCnt++;
        AmbaPrint("AmpUT_DummyEnc_FifoCB DummyEncFrmCnt: %d", DummyEncFrmCnt);
        AmbaKAL_MutexGive(&DummyEncFrmMutex);
    } else if (event == AMP_FIFO_EVENT_DATA_EOS) {
        ;
    }
    return 0;
}

int AmpUT_DummyEnc_CodecCB(void *hdlr, UINT32 event, void* info)
{
    if (event == AMP_ENC_EVENT_DATA_OVER_RUNOUT_THRESHOLD) {
        AmbaPrint("DummyEnc: !!!!!!!!!!! AMP_ENC_CALLBACK_EVENT_DATA_OVER_THRESHOLD !!!!!!!!!!");
    }
    return 0;
}

int AmpUT_DummyEnc_MuxInit(void *hdlr)
{
    AMP_FIFO_CFG_s fifoDefCfg;

    // create fifo
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = hdlr;
    fifoDefCfg.IsVirtual = 1;
    fifoDefCfg.NumEntries = 128;
    fifoDefCfg.cbEvent = AmpUT_DummyEnc_FifoCB;
    AmpFifo_Create(&fifoDefCfg, &dmyEncFifoHdlr);

    /* Create task */
    AmbaKAL_TaskCreate(&DummyEncMuxTask, /* pTask */
    "UT_DummyEnc_FWriteTask", /* pTaskName */
    7, /* Priority */
    AmpUT_DummyEnc_MuxTask, /* void (*EntryFunction)(UINT32) */
    0x0, /* EntryArg */
    (void *) AmpUT_DummyEncStack, /* pStackBase */
    1024, /* StackByteSize */
    AMBA_KAL_AUTO_START); /* AutoStart */
    /* create mutex  for frame read and write to fifo*/
    AmbaKAL_MutexCreate(&DummyEncFrmMutex);

    return 0;
}


int AmpUT_DummyEnc_init(char* idrItvl)
{
    AMP_DUMMYENC_CFG_s codecCfg;
    AMP_ENC_PIPE_CFG_s pipeCfg;

    /// create codec hdlr
    AmpDummyEnc_GetDefaultCfg(&codecCfg);
    // alloc memory
    AmbaKAL_BytePoolAllocate(&G_MMPL, &rawBuf, DUMMYENC_RAW_SIZE, 100);
    codecCfg.rawBuffer = rawBuf;
    codecCfg.rawBufferSize = DUMMYENC_RAW_SIZE;
    codecCfg.memRunOutAlarmThr = DUMMYENC_RAW_SIZE >> 2;
    AmbaKAL_BytePoolAllocate(&G_MMPL, &descBuf, DUMMYENC_RAW_DESC_NUM*sizeof(AMP_BITS_DESC_s), 100);
    codecCfg.descBuffer = descBuf;
    codecCfg.numDescBuffer = DUMMYENC_RAW_DESC_NUM;
    codecCfg.idrItvl = atoi(idrItvl);
    AmbaPrint("Open file:1.medis with idr itvl=%d", codecCfg.idrItvl);
    AmbaUtility_Ascii2Unicode("C:\\OUT_0000.h264", codecCfg.rawFn);
    AmbaUtility_Ascii2Unicode("C:\\OUT_0000.nhnt", codecCfg.idxFn);
    codecCfg.frmDly = 100;
    codecCfg.cbCodecEvent = AmpUT_DummyEnc_CodecCB;
    AmpDummyEnc_Create(&codecCfg, &dmyEncHdlr);
    /// create enc mgr
    AmpEnc_GetDefaultCfg(&pipeCfg);
    //pipeCfg.cbEvent
    pipeCfg.encoder[0] = dmyEncHdlr;
    pipeCfg.numEncoder = 1;
    pipeCfg.type = AMP_ENC_AV_PIPE;
    AmpEnc_Create(&pipeCfg, &encPipeHdlr);
    // active pipe
    AmpEnc_Add(encPipeHdlr);

    AmpUT_DummyEnc_MuxInit(dmyEncHdlr);

    return 0;
}

int AmpUT_DummyEnc_start(void)
{
    AmpEnc_StartRecord(encPipeHdlr, 0);
    return 0;
}

int AmpUT_DummyEnc_pause(void)
{
    AmpEnc_PauseRecord(encPipeHdlr, 0);
    return 0;
}

int AmpUT_DummyEnc_resume(void)
{
    AmpEnc_ResumeRecord(encPipeHdlr, 0);
    return 0;
}

int AmpUT_DummyEnc_stop(void)
{
    AmpEnc_StopRecord(encPipeHdlr, 0);
    // deinit
    AmpEnc_Remove(encPipeHdlr);
    AmpEnc_Delete(encPipeHdlr);
    encPipeHdlr = NULL;

    AmpFifo_Delete(dmyEncFifoHdlr);
    dmyEncFifoHdlr = NULL;

    AmpDummyEnc_Delete(dmyEncHdlr);
    dmyEncHdlr = NULL;
    AmbaKAL_BytePoolFree(rawBuf);
    AmbaKAL_BytePoolFree(descBuf);
    rawBuf = NULL;
    descBuf = NULL;

    if (outputFile) {
        AmbaFS_fclose(outputFile);
    }
    outputFile = NULL;
    return 0;
}

int AmpUT_DummyEnc_rand(char* seed, char* off)
{
    DummyEncRandSeed = atoi(seed);
    return 0;
}

int AmpUT_DummyEncTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("AmpUT_DummyEncTest cmd: %s", argv[1]);
    if ( strcmp(argv[1],"init") == 0) {
        AmpUT_DummyEnc_init((char*)argv[2]);
    }else if ( strcmp(argv[1],"start") == 0) {
        AmpUT_DummyEnc_start();
    }else if ( strcmp(argv[1],"pause") == 0) {
        AmpUT_DummyEnc_pause();
    }else if ( strcmp(argv[1],"resume") == 0) {
        AmpUT_DummyEnc_resume();
    }else if ( strcmp(argv[1],"stop") == 0) {
        AmpUT_DummyEnc_stop();
    }else if ( strcmp(argv[1],"rand") == 0) {
        AmpUT_DummyEnc_rand((char*)argv[2], (char*)argv[3]);
    }else if ( strcmp(argv[1],"end") == 0) {

    }
    return 0;
}

int AmpUT_DummyEncInit(void)
{
    // hook command
    AmbaTest_RegisterCommand("dmyenc", AmpUT_DummyEncTest);

    return AMP_OK;
}
