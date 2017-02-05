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

#include <player/Decode.h>
#include <player/DummyDec.h>
#include "AmpUnitTest.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define DUMMYDEC_RAW_SIZE (5<<20) // 5MB raw buffer
#define DUMMYDEC_RAW_DESC_NUM (128) // descriptor number = 128

// global var
void* rawBuf = NULL;
void* descBuf = NULL;
static AMP_AVDEC_HDLR_s *dmyDecHdlr = NULL;
static AMP_DEC_PIPE_HDLR_s *decPipeHdlr = NULL;
static AMP_FIFO_HDLR_s *dmyDecFifoHdlr = NULL;

static AMBA_KAL_TASK_t DummyDecDemuxTask = {0};
static AMBA_KAL_MUTEX_t DummyDecFrmMutex = {0};
static UINT32 DummyDecFrmCnt = 0;

#define BYTE_ORDER_CONV(x) ((x>>24) | ((x&0x00FF0000)>>8)  | ((x&0x0000FF00)<<8) | ((x&0x000000FF)<<24))


static char AmpUT_DummyDecStack[1024];
void AmpUT_DummyDec_DemuxTask(UINT32 info)
{
    AMP_BITS_DESC_s desc;
    AmbaPrint("AmpUT_DummyDec_DemuxTask Start!");
    while (1) {
        AmbaKAL_TaskSleep(50);
        AmbaKAL_MutexTake(&DummyDecFrmMutex, AMBA_KAL_WAIT_FOREVER);
        if (DummyDecFrmCnt > 64) {
            AmbaKAL_MutexGive(&DummyDecFrmMutex);
            continue;
        }
        DummyDecFrmCnt ++;
        AmbaKAL_MutexGive(&DummyDecFrmMutex);

        // prepare entry
        AmpFifo_PrepareEntry(dmyDecFifoHdlr, &desc);

        // read frame

        AmpFifo_WriteEntry(dmyDecFifoHdlr, &desc);
        AmbaPrint("AmpUT_DummyDec_DemuxTask write");
    }
}

int AmpUT_DummyDec_FifoCB(void *hdlr, UINT32 event, void* info)
{
    AmbaPrint("AmpUT_DummyDec_FifoCB on Event: 0x%x 0x%x", event, AMP_FIFO_EVENT_DATA_READY);
    if (event == AMP_FIFO_EVENT_DATA_CONSUMED) {
        AmbaKAL_MutexTake(&DummyDecFrmMutex, AMBA_KAL_WAIT_FOREVER);
        DummyDecFrmCnt--;
        AmbaPrint("AmpUT_DummyDec_FifoCB DummyDecFrmCnt: %d", DummyDecFrmCnt);
        AmbaKAL_MutexGive(&DummyDecFrmMutex);
    }
    return 0;
}

int AmpUT_DummyDec_init(char* fn)
{
    AMP_DUMMYDEC_CFG_s codecCfg;
    AMP_DEC_PIPE_CFG_s pipeCfg;
    AMP_FIFO_CFG_s fifoDefCfg;

    AmbaPrint("AmpUT_DummyDec_init: target file %s", fn);

    /// create codec hdlr
    AmpDummyDec_GetDefaultCfg(&codecCfg);
    // alloc memory
    AmbaKAL_BytePoolAllocate(&G_MMPL, &rawBuf, DUMMYDEC_RAW_SIZE, 100);
    codecCfg.RawBuffer = rawBuf;
    codecCfg.RawBufferSize = DUMMYDEC_RAW_SIZE;
    AmbaKAL_BytePoolAllocate(&G_MMPL, &descBuf, DUMMYDEC_RAW_DESC_NUM*sizeof(AMP_BITS_DESC_s), 100);
    codecCfg.DescBuffer = descBuf;
    codecCfg.NumDescBuffer = DUMMYDEC_RAW_DESC_NUM;
    //codecCfg.cbCodecEvent =
    AmpDummyDec_Create(&codecCfg, &dmyDecHdlr);

    /// create dec mgr
    AmpDec_GetDefaultCfg(&pipeCfg);
    //pipeCfg.cbEvent
    pipeCfg.Decoder[0] = dmyDecHdlr;
    pipeCfg.NumDecoder = 1;
    pipeCfg.Type = AMP_DEC_VID_PIPE;
    AmpDec_Create(&pipeCfg, &decPipeHdlr);

    // active pipe
    AmpDec_Add(decPipeHdlr);

    // dummy mux
    // create fifo
    AmpFifo_GetDefaultCfg(&fifoDefCfg);
    fifoDefCfg.hCodec = dmyDecHdlr;
    fifoDefCfg.IsVirtual = 1;
    fifoDefCfg.NumEntries = 128;
    fifoDefCfg.cbEvent = AmpUT_DummyDec_FifoCB;
    AmpFifo_Create(&fifoDefCfg, &dmyDecFifoHdlr);

    /* Create task */
    AmbaKAL_TaskCreate(&DummyDecDemuxTask, /* pTask */
    "UT_DummyDec_FReadTask", /* pTaskName */
    7, /* Priority */
    AmpUT_DummyDec_DemuxTask, /* void (*EntryFunction)(UINT32) */
    0x0, /* EntryArg */
    (void *) AmpUT_DummyDecStack, /* pStackBase */
    1024, /* StackByteSize */
    AMBA_KAL_AUTO_START); /* AutoStart */


    /* create mutex  for frame read and write to fifo*/
    AmbaKAL_MutexCreate(&DummyDecFrmMutex);

    return 0;
}

int AmpUT_DummyDec_start(void)
{
    AMP_AVDEC_TRICKPLAY_s trick;
    // pre-feed

    trick.Speed = 0x100;
    trick.TimeOffsetOfFirstFrame = 0;
    trick.Direction = AMP_VIDEO_PLAY_FW;
    AmpDec_Start(decPipeHdlr, &trick, NULL);
    return 0;
}

int AmpUT_DummyDec_pause(void)
{
    AmpDec_Pause(decPipeHdlr);
    return 0;
}

int AmpUT_DummyDec_resume(void)
{
    AmpDec_Resume(decPipeHdlr);
    return 0;
}

int AmpUT_DummyDec_stop(void)
{
    AmpDec_Stop(decPipeHdlr);

    // deinit
    AmpDec_Remove(decPipeHdlr);
    AmpDec_Delete(decPipeHdlr);
    decPipeHdlr = NULL;

    AmpFifo_Delete(dmyDecFifoHdlr);
    dmyDecFifoHdlr = NULL;

    AmpDummyDec_Delete(dmyDecHdlr);
    dmyDecHdlr = NULL;
    AmbaKAL_BytePoolFree(rawBuf);
    AmbaKAL_BytePoolFree(descBuf);
    rawBuf = NULL;
    descBuf = NULL;

    return 0;
}

int AmpUT_DummyDecTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("AmpUT_DummyDecTest cmd: %s", argv[1]);
    if ( strcmp(argv[1],"init") == 0) {
        AmpUT_DummyDec_init((char*)argv[2]);
    }else if ( strcmp(argv[1],"start") == 0) {
        AmpUT_DummyDec_start();
    }else if ( strcmp(argv[1],"pause") == 0) {
        AmpUT_DummyDec_pause();
    }else if ( strcmp(argv[1],"resume") == 0) {
        AmpUT_DummyDec_resume();
    }else if ( strcmp(argv[1],"stop") == 0) {
        AmpUT_DummyDec_stop();
    }else if ( strcmp(argv[1],"end") == 0) {

    }
    return 0;
}

int AmpUT_DummyDecInit(void)
{
    // hook command
    AmbaTest_RegisterCommand("dmydec", AmpUT_DummyDecTest);

    return AMP_OK;
}
