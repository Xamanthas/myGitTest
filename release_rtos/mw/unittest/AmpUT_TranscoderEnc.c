/**
  * @file src/app/sample/unittest/AmpUT_TranscoderEnc.c
  *
  * transcoder encode unit test
  *
  * History:
  *    2014/04/24 - [Wisdom Hung] created file
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
#include <transcoder/Transcoder.h>
#include "AmbaUtility.h"
#include <cfs/AmpCfs.h>
#include <util.h>

static AMP_CFS_FILE_PARAM_s cfsParam;

void *UT_TranscoderEncodefopen(const char *pFileName, const char *pMode)
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

int UT_TranscoderEncodefclose(void *pFile)
{
    return AmpCFS_fclose((AMP_CFS_FILE_s *)pFile);
}

UINT64 UT_TranscoderEncodefwrite(const void *pBuf, UINT64 Size, UINT64 Count, void *pFile)
{
    return AmpCFS_fwrite(pBuf, Size, Count, (AMP_CFS_FILE_s *)pFile);
}

int UT_TranscoderEncodefsync(void *pFile)
{
    return AmpCFS_FSync((AMP_CFS_FILE_s *)pFile);
}

// global var for TranscoderEnc
static void *TranscoderHdlr = NULL;
static UINT8 *TranscoderMainWorkBuf = NULL;
#define STATUS_TRANSCODER_NONE     1
#define STATUS_TRANSCODER_IDLE     2
#define STATUS_TRANSCODER          3
static UINT8 TranscoderStatus = STATUS_TRANSCODER_NONE;

/* muxer */
#define TRANSCODER_INFO_SIZE 128
#define TRANSCODER_BISFIFO_SIZE TRANSCODER_INFO_SIZE*32
static UINT8 *TranscoderBitsBuf;
static UINT8 *OriTranscoderBitsBuf;
#define TRANSCODER_DESC_SIZE 8*32
static UINT8 *TranscoderDescBuf;
static UINT8 *OriTranscoderDescBuf;


static AMP_FIFO_HDLR_s *TranscoderVirtualFifoHdlr = NULL;
static AMP_FIFO_HDLR_s *SrcFifoHdlr = NULL;

static AMP_CFS_FILE_s *OutputPriFile = NULL;

static UINT8 src_hcodec;

static UINT32 transcoder_no=0;

static int AmpUT_Transcoder_FifoCB(void *hdlr, UINT32 event, void* info)
{
    UINT32 *numFrames = info;
    AmbaPrintColor(RED,"AmpUT_Transcoder_FifoCB on Event: 0x%x 0x%x", event, *numFrames);
    if (event == AMP_FIFO_EVENT_DATA_READY) {
        int i;

        for(i = 0; i < *numFrames; i++) {

            AMP_BITS_DESC_s *desc;
            int er;
            UINT8 *bitsLimit = TranscoderBitsBuf + TRANSCODER_BISFIFO_SIZE - 1;

            if (OutputPriFile == NULL) {
                char fn[80];
                char mdASCII[3] = {'w','+','\0'};

                sprintf(fn,"C:\\transcoder_%04d.dat", (int)transcoder_no);

                OutputPriFile = UT_TranscoderEncodefopen((const char *)fn, (const char *)mdASCII);
                AmbaPrint("%s opened", fn);
                transcoder_no++;
                bitsLimit = TranscoderBitsBuf + TRANSCODER_BISFIFO_SIZE - 1;
            }
            er = AmpFifo_PeekEntry(TranscoderVirtualFifoHdlr, &desc, 0);
            if (er == 0) {
                AmbaPrint("Muxer PeekEntry: pts:%8lld size:%5d@0x%08x", desc->Pts, desc->Size, desc->StartAddr);
            } else {
                while (er != 0) {
                    AmbaPrint("[Transcoder]Muxer PeekEntry: Empty...");
                    AmbaKAL_TaskSleep(30);
                    er = AmpFifo_PeekEntry(TranscoderVirtualFifoHdlr, &desc, 0);
                }
            }
            if (OutputPriFile) {
                AmbaPrint("[Transcoder]Write: 0x%x sz %d limit %X",  desc->StartAddr, desc->Size, bitsLimit);

                if ((desc->Type == AMP_FIFO_TYPE_EOS)||(desc->Size== AMP_FIFO_MARK_EOS)) {
                    UT_TranscoderEncodefsync((void *)OutputPriFile);
                    UT_TranscoderEncodefclose((void *)OutputPriFile);
                    OutputPriFile = NULL;
                    AmbaPrint("[Transcoder]Muxer met EOS");
                } else {
                    if (desc->StartAddr + desc->Size <= bitsLimit) {
                        UT_TranscoderEncodefwrite((const void *)desc->StartAddr, 1, desc->Size, (void *)OutputPriFile);
                    } else {
                        //AmbaPrint("[Pri]Muxer Wrap Around");
                        UT_TranscoderEncodefwrite((const void *)desc->StartAddr, 1, bitsLimit - desc->StartAddr + 1, (void *)OutputPriFile);
                        UT_TranscoderEncodefwrite((const void *)TranscoderBitsBuf, 1, desc->Size - (bitsLimit - desc->StartAddr + 1), (void *)OutputPriFile);
                    }
                }
            }
            AmpFifo_RemoveEntry(TranscoderVirtualFifoHdlr, 1);
        }
    } else if (event == AMP_FIFO_EVENT_DATA_EOS) {
            UT_TranscoderEncodefsync((void *)OutputPriFile);
            UT_TranscoderEncodefclose((void *)OutputPriFile);
            OutputPriFile = NULL;
            AmbaPrint("[Transcoder]Muxer met EOS");
            AmpFifo_RemoveEntry(TranscoderVirtualFifoHdlr, 1);
    }

    return 0;
}

/*~muxer*/
static int AmpUT_Transcoder_event_Callback(void *hdlr,UINT32 event, void *info)
{
    switch (event) {
        case AMP_TRANSCODER_EVENT_DATA_OVER_RUNOUT_THRESHOLD:
            AmbaPrint("AmpUT_Transcoder: !!!!!!!!!!! AMP_TRANSCODER_EVENT_DATA_OVER_RUNOUT_THRESHOLD !!!!!!!!!!");
            break;
        case AMP_TRANSCODER_EVENT_DATA_OVERRUN:
            AmbaPrint("AmpUT_Transcoder: !!!!!!!!!!! AMP_TRANSCODER_EVENT_DATA_OVERRUN !!!!!!!!!!");
            break;
        case AMP_TRANSCODER_EVENT_DESC_OVERRUN:
            AmbaPrint("AmpUT_Transcoder: !!!!!!!!!!! AMP_TRANSCODER_EVENT_DESC_OVERRUN !!!!!!!!!!");
            break;
        default:
            AmbaPrint("AmpUT_Transcoder: Unknown %X info: %x", event, info);
            break;
    }
    return 0;
}

/*~Transcoder data*/
static char transcoder_info[TRANSCODER_INFO_SIZE];
static int AmpUT_Transcoder_Callback(UINT32 ori_size, UINT8* ori_data, UINT32 *transcode_size, UINT8** transcode_data)
{
    //AmbaPrint("AmpUT_TranscoderEnc_info_Callback");
    memset(transcoder_info, 0, 128);
    sprintf(transcoder_info,"X0000.0000Y0000.0000Z0000.0000G0000.0000$GPRMC,000125,V,,,,,000.0,,280908,002.1,N*71~, %d  \n",AmbaSysTimer_GetTickCount());

    (*transcode_size) = strlen(transcoder_info);
    (*transcode_data) = (unsigned char*) transcoder_info;

    return 0;
}

int AmpUT_Transcoder_Init(void)
{
    int er;
    void *TmpbufRaw = NULL;
    // Init TRANSCODERENC module
    {
        AMP_TRANSCODER_INIT_CFG_s encInitCfg;

        AmpTranscoder_GetInitDefaultCfg(&encInitCfg);
        if (TranscoderMainWorkBuf == NULL) {
            er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&TranscoderMainWorkBuf, (void **)&TmpbufRaw, encInitCfg.MemoryPoolSize, 32);
            if (er != OK) {
                AmbaPrint("Out of memory for ext!!");
            }
        }

        encInitCfg.MemoryPoolAddr = TranscoderMainWorkBuf;

        AmpTranscoder_Init(&encInitCfg);
    }

    return 0;
}

/* create transcoder pipe */
int AmpUT_Transcoder_Create(void)
{
    int er;
    AMP_TRANSCODER_HDLR_CFG_s encCfg;
    AMP_FIFO_CFG_s fifosrcCfg;

	// Create Transcoder encoder object
    AmpTranscoder_GetDefaultCfg(&encCfg);
    AmpFifo_GetDefaultCfg(&fifosrcCfg);
    fifosrcCfg.hCodec = &src_hcodec;
    fifosrcCfg.IsVirtual = 0;
    fifosrcCfg.NumEntries = 32;
    fifosrcCfg.cbEvent = AmpUT_Transcoder_event_Callback;
    AmpFifo_Create(&fifosrcCfg, &SrcFifoHdlr);
    encCfg.src_fifo = &src_hcodec;//(void *)(&src_hcodec);

    encCfg.SrcNumEntries = fifosrcCfg.NumEntries;
    // Assign callback
    encCfg.cbTranscoder = AmpUT_Transcoder_Callback;
    encCfg.cbEvent = AmpUT_Transcoder_event_Callback;
    encCfg.EventDataReadySkipNum = fifosrcCfg.EventDataReadySkipNum;

    AmbaPrint("[AmpUT_Transcoder_Create] src = 0x%X", &src_hcodec);

    // Assign bitstream/descriptor buffer
    er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&TranscoderBitsBuf, (void **)&OriTranscoderBitsBuf, TRANSCODER_BISFIFO_SIZE, 32);
    if (er != OK) {
        AmbaPrint("Out of Cache memory for bitsFifo!!");
    }
    encCfg.BitsBufCfg.BitsBufAddr = TranscoderBitsBuf;
    encCfg.BitsBufCfg.BitsBufSize = TRANSCODER_BISFIFO_SIZE;

    er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&TranscoderDescBuf, (void **)&OriTranscoderBitsBuf, TRANSCODER_DESC_SIZE, 32);
    if (er != OK) {
        AmbaPrint("Out of Cache memory for descFifo!!");
    }

    encCfg.BitsBufCfg.DescBufAddr = TranscoderDescBuf;
    encCfg.BitsBufCfg.DescBufSize = TRANSCODER_DESC_SIZE;
    encCfg.BitsBufCfg.BitsRunoutThreshold = TRANSCODER_BISFIFO_SIZE - 2*TRANSCODER_INFO_SIZE; // leave 2 remnants

    // Assign bitstream-specific configs
    AmpTranscoder_Create(&encCfg, &TranscoderHdlr);

    AmpTranscoder_Open(TranscoderHdlr);

    return 0;
}
int AmpUT_Transcoder_Start(void)
{
    AMP_FIFO_CFG_s fifoDefCfg;

    if (TranscoderStatus != STATUS_TRANSCODER_IDLE)
        return -1;

    if (TranscoderVirtualFifoHdlr == NULL) {
        // create Primary virtual fifo
        AmpFifo_GetDefaultCfg(&fifoDefCfg);
        fifoDefCfg.hCodec = TranscoderHdlr;
        fifoDefCfg.IsVirtual = 1;
        fifoDefCfg.NumEntries = 32;
        fifoDefCfg.cbEvent = AmpUT_Transcoder_FifoCB;
        AmpFifo_Create(&fifoDefCfg, &TranscoderVirtualFifoHdlr);
    }

    AmpTranscoder_Start(TranscoderHdlr);

    TranscoderStatus = STATUS_TRANSCODER;

    return 0;
}
static char feeddata[32] = {0};
static UINT32 frm=0;
int AmpUT_Transcoder_feedframe(void)
{
    AMP_BITS_DESC_s feed_desc={0};

    sprintf(feeddata,"transcoder_test,fr=%d, %d  \n", (int)frm, AmbaSysTimer_GetTickCount());

    feed_desc.Size = strlen(feeddata);
    feed_desc.StartAddr = (unsigned char*) feeddata;
    frm++;

    AmpFifo_WriteEntry(SrcFifoHdlr, &feed_desc);

    return 0;
}

int AmpUT_Transcoder_Stop(void)
{
    if (TranscoderStatus != STATUS_TRANSCODER) {
        return -1;
    }

    {
        AMP_BITS_DESC_s desc={0};
        desc.Size = AMP_FIFO_MARK_EOS;
        desc.Type = AMP_FIFO_TYPE_EOS;
        AmpFifo_WriteEntry(SrcFifoHdlr, &desc);
    }

    AmpTranscoder_Stop(TranscoderHdlr);
    TranscoderStatus = STATUS_TRANSCODER_IDLE;
    frm=0;
    return 0;
}

/* delete ext pipe */
int AmpUT_Transcoder_Delete(void)
{

    AmpTranscoder_Delete(TranscoderHdlr);

    /* Free bits */
    AmbaKAL_BytePoolFree((void *)OriTranscoderBitsBuf);
    TranscoderBitsBuf = NULL;
    AmbaKAL_BytePoolFree((void *)OriTranscoderDescBuf);
    TranscoderDescBuf = NULL;

    return 0;
}

int AmpUT_TranscoderEncTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("AmpUT_TranscoderTest cmd: %s", argv[1]);
    if (strcmp(argv[1],"init") == 0) {
        AmpUT_Transcoder_Init();
        TranscoderStatus = STATUS_TRANSCODER_IDLE;
    } else if ((strcmp(argv[1],"create") == 0) || (strcmp(argv[1],"cre") == 0)) {
        if (TranscoderStatus != STATUS_TRANSCODER_IDLE && \
            TranscoderStatus != STATUS_TRANSCODER) {
            AmbaPrint("[Transcoder] Non Init");
            return -1;
        }
        AmpUT_Transcoder_Create();
    } else if ((strcmp(argv[1],"encstart") == 0) || (strcmp(argv[1],"enst") == 0)) {
        UINT32 tt = atoi(argv[2]);

        if (TranscoderStatus == STATUS_TRANSCODER) {
            AmbaPrint("[Transcoder] encode start during Encoding");
            return -1;
        }

        AmbaPrint("Encode for %d milliseconds", tt);
        AmpUT_Transcoder_Start();
        if (tt != 0) {
            while (tt) {
                AmbaKAL_TaskSleep(1);
                tt--;
                if (tt == 0) {
                    AmpUT_Transcoder_Stop();
                    TranscoderStatus = STATUS_TRANSCODER_IDLE;
                }
                if (TranscoderStatus == STATUS_TRANSCODER_IDLE) {
                    tt = 0;
                }
            }
        }
    } else if ((strcmp(argv[1],"feed") == 0)) {
        if (TranscoderStatus != STATUS_TRANSCODER) {
            AmbaPrint("[Transcoder] encode state at IDLE");
            return -1;
        }

        AmpUT_Transcoder_feedframe();

    } else if ((strcmp(argv[1],"encstop") == 0) || (strcmp(argv[1],"ensp") == 0)) {
        if (TranscoderStatus != STATUS_TRANSCODER) {
            AmbaPrint("[Transcoder] encode stop duing IDLE");
            return -1;
        }

        AmpUT_Transcoder_Stop();
        TranscoderStatus = STATUS_TRANSCODER_IDLE;
    } else if (strcmp(argv[1],"delete") == 0) {
        if (TranscoderStatus == STATUS_TRANSCODER) {
            AmbaPrint("[Transcoder] delete during Encodeing !!");
            return -1;
        }

        AmpUT_Transcoder_Delete();
    } else {
        AmbaPrint("Usage: t transcoderenc init|create|encstart|encstop");
        AmbaPrint("       init: init all");
        AmbaPrint("       create : create transcoder instance");
        AmbaPrint("       encstart [millisecond]: recording for N milliseconds");
        AmbaPrint("                               N = 0 will do continuous encode");
        AmbaPrint("       feed                     : feed 1 frame to transcoder");
        AmbaPrint("       encstop: stop recording");
        AmbaPrint("       delete: delete Transcoder instance");
    }
    return 0;
}

int AmpUT_TranscoderEncTestAdd(void)
{
    AmbaPrint("Adding AmpUT_TranscoderEnc");
    AmbaTest_RegisterCommand("transcoderenc", AmpUT_TranscoderEncTest);
    return AMP_OK;
}

