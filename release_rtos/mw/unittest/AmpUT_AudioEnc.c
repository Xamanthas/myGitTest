/**
 * @file src/app/sample/unittest/AmpUT_AudioEnc.c
 *
 * audio encode unit test
 *
 * History:
 *    2013/05/02 - [Jenghung Luo] created file
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
#include <recorder/Encode.h>
#include <recorder/AudioEnc.h>
#include "AmbaUtility.h"
#include <util.h>

#include <cfs/AmpCfs.h>

void *UT_AudioEncodefopen(const char *pFileName, const char *pMode)
{
    AMP_CFS_FILE_PARAM_s cfsParam;
    AmpCFS_GetFileParam(&cfsParam);

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

int UT_AudioEncodefclose(void *pFile)
{
    return AmpCFS_fclose((AMP_CFS_FILE_s *)pFile);
}

UINT64 UT_AudioEncodefwrite(const void *pBuf, UINT64 Size, UINT64 Count, void *pFile)
{
    return AmpCFS_fwrite(pBuf, Size, Count, (AMP_CFS_FILE_s *)pFile);
}

int UT_AudioEncodefsync(void *pFile)
{
    return AmpCFS_FSync((AMP_CFS_FILE_s *)pFile);
}

static char DefaultSlot[] = "C";

// global var for AudioEnc
static AMP_AVENC_HDLR_s *AudioEncPriHdlr = NULL;
static AMP_AVENC_HDLR_s *AudioEncSecHdlr = NULL;
static AMP_ENC_PIPE_HDLR_s *AudioEncPipe = NULL;
static UINT8 *AudioMainWorkBuf = NULL;
#define STATUS_AUDIO_NONE     1
#define STATUS_AUDIO_IDLE     2
#define STATUS_AUDIO_ENCODE   3
#define STATUS_AUDIO_PAUSE    4
static UINT8 AudioStatus = STATUS_AUDIO_NONE;

static UINT8 *AudworkNCBuf = NULL;
static UINT8 *AudworkCacheBuf = NULL;
static UINT8 *OriAudworkCacheBuf = NULL;
static UINT8 *AudworkNCRawBuf = NULL;
static UINT8 *OriAudworkNCRawBuf = NULL;
static UINT8 *AudworkCacheRawBuf = NULL;
static UINT8 *OriAudworkCacheRawBuf = NULL;
static INT32 *AudworkEffectCalibBuffer = NULL;
static INT32 *OriAudworkEffectCalibBuffer = NULL;
static INT8 *AudCalibCurv = NULL;
static UINT8 PriEncodeType = 0;
static UINT8 SecEncodeType = 0;

// static for multi-stream encode
static AMP_AUDIOENC_INSTANCE_s AudioEncInstance[2] = {0};
static AMP_AUDIOENC_INSTANCE_s AudioEncSecInstance[2] = {0};
static UINT8 AudioMultiMode = 0;
#define AUDENC_MULTI_AIN    0x1
#define AUDENC_MULTI_STREAM 0x2

/* muxer */
#define AUDENC_BISFIFO_SIZE 4096*1536 //6MB
static UINT8 *AudPriBitsBuf;
static UINT8 *OriAudPriBitsBuf;
static UINT8 *AudSecBitsBuf;
static UINT8 *OriAudSecBitsBuf;
#define AUDENC_DESC_SIZE 40*128
static UINT8 *AudPriDescBuf;
static UINT8 *OriAudPriDescBuf;
static UINT8 *AudSecDescBuf;
static UINT8 *OriAudSecDescBuf;
static UINT8 AudioLogMux = 0;
static UINT8 AudioEncDumpSkip = 0;                     // Encode Skip dump file, write data to SD card or just lies to fifo that it has muxed pending data
#define AUDENC_SKIP_PRI         (0x1<<0)
#define AUDENC_SKIP_SEC         (0x1<<1)

static AMP_FIFO_HDLR_s *AudioEncPriVirtualFifoHdlr = NULL;
static AMP_FIFO_HDLR_s *AudioEncSecVirtualFifoHdlr = NULL;
static AMBA_KAL_TASK_t AudioEncPriMuxTask = {0};
static AMBA_KAL_TASK_t AudioEncSecMuxTask = {0};
static AMBA_KAL_SEM_t AudioEncPriSem = {0};
static AMBA_KAL_SEM_t AudioEncSecSem = {0};
static AMP_CFS_FILE_s *OutputPriFile = NULL;
static AMP_CFS_FILE_s *outputPriIdxFile = NULL;
static AMP_CFS_FILE_s *OutputSecFile = NULL;
static AMP_CFS_FILE_s *OutputPriHeaderFile = NULL;
static AMP_CFS_FILE_s *OutputSecHeaderFile = NULL;
static AMP_CFS_FILE_s *OutputPriUDTAFile = NULL;
static char *AmpUT_AudioEncPriStack;
#define AUDIO_ENC_PRI_MUX_TASK_STACK_SIZE (8192)
static char *AmpUT_AudioEncSecStack;
#define AUDIO_ENC_SEC_MUX_TASK_STACK_SIZE (8192)
static int Fno = 0;

static UINT32 AencPriTotalFrames = 0;    // total frames primary stream muxer received
static UINT32 AencSecTotalFrames = 0;    // total frames secondary stream muxer received

static UINT32 AudioSampleRate[2] = {48000};
static UINT16 AudioChannels[2] = {2};
static UINT16 AudioBitsPerSample[2] = {16};
static UINT16 AudioFrmsz_a[2] = {1024};
static UINT32 AudioBitrate[2] = {0};

static AMBA_AUDIO_AAC_BS_TYPE_e AacFormat = AAC_BS_ADTS;

/* Audio Effect structure */
typedef struct _AIN_EFFECT_PROC_s {
	UINT16 mute; // mute output, 1: mute, 0: normal
} AIN_EFFECT_PROC_t;

AMBA_AUDIO_PLUGIN_EFFECT_CS_s AudioAinEffectControl; //main control structure
AMBA_AUDIO_PLUGIN_EFFECT_CS_s AudioAinEffectControlShadow; //shadow of the "main control structure"
AIN_EFFECT_PROC_t AudioAinEffectConfig; //self control structure
AIN_EFFECT_PROC_t AudioAinEffectConfigShadow; //shadow of the "self control structure"

void AmpUT_AudioEnc_PriMuxTask(UINT32 info)
{
    AMP_BITS_DESC_s *desc;
    int er;
    NHNT_HEADER_s NhntHeader = {0};
    NHNT_SAMPLE_HEADER_s NhntSample = {0};
    UINT8 *bitsLimit = AudPriBitsBuf + AUDENC_BISFIFO_SIZE - 1;
    UINT64 FileOffset = 0;

    AmbaPrint("AmpUT_AudioEnc_PriMuxTask Start");

    while (1) {
        AmbaKAL_SemTake(&AudioEncPriSem, AMBA_KAL_WAIT_FOREVER);

        if (!(AudioEncDumpSkip & AUDENC_SKIP_PRI)) {
            if (OutputPriHeaderFile == NULL) {
                char fn[80];
                char mdASCII[3] = {'w','+','\0'};

                sprintf(fn, "%s:\\Pri_%04d.bin", DefaultSlot, Fno);

                OutputPriHeaderFile = UT_AudioEncodefopen((const char *)fn, (const char *)mdASCII);
                AmbaPrint("%s opened", fn);
            }
            if (OutputPriFile == NULL) {
                AUDIO_USER_DATA_s Udta;
                char fn[80];
                char mdASCII[3] = {'w','+','\0'};

                if (PriEncodeType == 0 || PriEncodeType == 2 || PriEncodeType == 3)
                    sprintf(fn,"%s:\\Pri_%04d.aac", DefaultSlot, Fno);
                else if (PriEncodeType == 1 || PriEncodeType == 4)
                    sprintf(fn,"%s:\\Pri_%04d.pcm", DefaultSlot, Fno);
                else if (PriEncodeType == 5)
                    sprintf(fn,"%s:\\Pri_%04d.ac3", DefaultSlot, Fno);
                else if (PriEncodeType == 6)
                    sprintf(fn,"%s:\\Pri_%04d.mp3", DefaultSlot, Fno);
                else if (PriEncodeType == 7)
                    sprintf(fn,"%s:\\Pri_%04d.opus", DefaultSlot, Fno);

                OutputPriFile = UT_AudioEncodefopen((const char *)fn, (const char *)mdASCII);
                AmbaPrint("%s opened", fn);

                sprintf(fn,"%s:\\Pri_%04d.nhnt", DefaultSlot, Fno);

                outputPriIdxFile = UT_AudioEncodefopen((const char *)fn, (const char *)mdASCII);
                AmbaPrint("%s opened", fn);

                NhntHeader.Signature[0]='n';
                NhntHeader.Signature[1]='h';
                NhntHeader.Signature[2]='n';
                NhntHeader.Signature[3]='t';
                UT_AudioEncodefwrite((const void *)&NhntHeader, 1, sizeof(NhntHeader), (void *)outputPriIdxFile);

                bitsLimit = AudPriBitsBuf + AUDENC_BISFIFO_SIZE - 1;

                sprintf(fn,"%s:\\Pri_%04d.udta", DefaultSlot, Fno);

                OutputPriUDTAFile = UT_AudioEncodefopen((const char *)fn, (const char *)mdASCII);
                AmbaPrint("%s opened", fn);
                Udta.SampleRate = AudioSampleRate[0];
                Udta.Channels = AudioChannels[0];
                Udta.BitsPerSample = AudioBitsPerSample[0];
                Udta.Frame_Size = AudioFrmsz_a[0];
                Udta.Bitrate = AudioBitrate[0];
                UT_AudioEncodefwrite((const void *)&Udta, 1, sizeof(AUDIO_USER_DATA_s), (void *)OutputPriUDTAFile);
                UT_AudioEncodefclose((void *)OutputPriUDTAFile);

                Fno++;
            }
        }

        er = AmpFifo_PeekEntry(AudioEncPriVirtualFifoHdlr, &desc, 0);
        if (er == 0) {
            if (AudioLogMux) {
                AmbaPrint("[Pri %d] size:%5d@0x%08X", desc->SeqNum, desc->Size, desc->StartAddr);
            }
        } else {
            while (er != 0) {
                AmbaPrint("[Pri]Muxer PeekEntry: Empty...");
                AmbaKAL_TaskSleep(30);
                er = AmpFifo_PeekEntry(AudioEncPriVirtualFifoHdlr, &desc, 0);
            }
        }

        if (!(AudioEncDumpSkip & AUDENC_SKIP_PRI)) {
            if (OutputPriHeaderFile) {
                UINT32 size = desc->Size;
                UT_AudioEncodefwrite((const void *)&size, 1, sizeof(UINT32), (void *)OutputPriHeaderFile);

                if (desc->Size == AMP_FIFO_MARK_EOS) {
                    UT_AudioEncodefsync((void *)OutputPriHeaderFile);
                    UT_AudioEncodefclose((void *)OutputPriHeaderFile);
                    OutputPriHeaderFile = NULL;
                    AmbaPrint("[Pri]Header met EOS, TotalFrames %d", AencPriTotalFrames);
                    AencPriTotalFrames = 0;
                }
            }

            if (OutputPriFile) {
                NhntSample.CompositionTimeStamp = desc->Pts;
                NhntSample.DecodingTimeStamp = desc->Pts;
                NhntSample.DataSize = desc->Size;
                NhntSample.FileOffset = FileOffset;
                FileOffset += desc->Size;
                NhntSample.FrameType = desc->Type;

                UT_AudioEncodefwrite((const void *)&NhntSample, 1, sizeof(NhntSample), (void *)outputPriIdxFile);
                //AmbaPrint("[Pri]Write: 0x%x sz %d limit %X",  desc->StartAddr, desc->Size, bitsLimit);
                if (desc->Size == AMP_FIFO_MARK_EOS) {
                    UT_AudioEncodefsync((void *)OutputPriFile);
                    UT_AudioEncodefclose((void *)OutputPriFile);
                    OutputPriFile = NULL;
                    UT_AudioEncodefsync((void *)outputPriIdxFile);
                    UT_AudioEncodefclose((void *)outputPriIdxFile);
                    outputPriIdxFile = NULL;
                    FileOffset = 0;
                    AmbaPrint("[Pri]Muxer met EOS");
                } else if (desc->Size == AMP_FIFO_MARK_EOS_PAUSE) {
                    // do nothing
                } else {
                    if (desc->StartAddr + desc->Size <= bitsLimit) {
                        UT_AudioEncodefwrite((const void *)desc->StartAddr, 1, desc->Size, (void *)OutputPriFile);
                    }else{
                        AmbaPrint("[Pri]Muxer Wrap Around");
                        UT_AudioEncodefwrite((const void *)desc->StartAddr, 1, bitsLimit - desc->StartAddr + 1, (void *)OutputPriFile);
                        UT_AudioEncodefwrite((const void *)AudPriBitsBuf, 1, desc->Size - (bitsLimit - desc->StartAddr + 1), (void *)OutputPriFile);
                    }
                }
            }
        } else {
            AmbaKAL_TaskSleep(1);
            if (desc->Type == AMP_FIFO_TYPE_EOS) AmbaPrint("[Pri]Muxer met EOS");
        }

        AmpFifo_RemoveEntry(AudioEncPriVirtualFifoHdlr, 1);
        AencPriTotalFrames++;
    }
}

void AmpUT_AudioEnc_SecMuxTask(UINT32 info)
{
    AMP_BITS_DESC_s *desc;
    int er;
    UINT8 *bitsLimit = AudSecBitsBuf + AUDENC_BISFIFO_SIZE - 1;

    AmbaPrint("AmpUT_AudioEnc_SecMuxTask Start");

    while (1) {
        AmbaKAL_SemTake(&AudioEncSecSem, AMBA_KAL_WAIT_FOREVER);

        if (!(AudioEncDumpSkip & AUDENC_SKIP_SEC)) {
            if (OutputSecHeaderFile == NULL) {
                char fn[80];
                char mdASCII[3] = {'w','+','\0'};

                sprintf(fn,"%s:\\Sec_%04d.bin", DefaultSlot, Fno);
                OutputSecHeaderFile = UT_AudioEncodefopen((const char *)fn, (const char *)mdASCII);
                AmbaPrint("%s opened", fn);
            }
            if (OutputSecFile == NULL) {
                char fn[80];
                char mdASCII[3] = {'w','+','\0'};

                if (SecEncodeType == 0 || SecEncodeType == 2 || SecEncodeType == 3)
                    sprintf(fn,"%s:\\Sec_%04d.aac", DefaultSlot, Fno);
                else if (SecEncodeType == 1 || SecEncodeType == 4)
                    sprintf(fn,"%s:\\Sec_%04d.pcm", DefaultSlot, Fno);
                else if (SecEncodeType == 5)
                    sprintf(fn,"%s:\\Sec_%04d.ac3", DefaultSlot, Fno);
                else if (SecEncodeType == 6)
                    sprintf(fn,"%s:\\Sec_%04d.mp3", DefaultSlot, Fno);
                else if (SecEncodeType == 7)
                    sprintf(fn,"%s:\\Sec_%04d.opus", DefaultSlot, Fno);

                OutputSecFile = UT_AudioEncodefopen((const char *)fn, (const char *)mdASCII);
                AmbaPrint("%s opened", fn);
                bitsLimit = AudSecBitsBuf + AUDENC_BISFIFO_SIZE - 1;
            }
        }

        er = AmpFifo_PeekEntry(AudioEncSecVirtualFifoHdlr, &desc, 0);
        if (er == 0) {
            if (AudioLogMux) {
                AmbaPrint("[Sec %d] size:%5d@0x%08X", desc->SeqNum, desc->Size, desc->StartAddr);
            }
        } else {
            while (er != 0) {
                AmbaPrint("Sec]Muxer PeekEntry: Empty...");
                AmbaKAL_TaskSleep(30);
                er = AmpFifo_PeekEntry(AudioEncSecVirtualFifoHdlr, &desc, 0);
            }
        }

        if (desc->Size == AMP_FIFO_MARK_EOS) {
            // EOS
            if (!(AudioEncDumpSkip & AUDENC_SKIP_SEC)) {
                if (OutputSecHeaderFile) {
                    UT_AudioEncodefsync((void *)OutputSecHeaderFile);
                    UT_AudioEncodefclose((void *)OutputSecHeaderFile);
                    OutputSecHeaderFile = NULL;
                    AmbaPrint("[Sec]Header met EOS, TotalFrames %d", AencSecTotalFrames);
                    AencSecTotalFrames = 0;
                }

                if (OutputSecFile) {
                    UT_AudioEncodefsync((void *)OutputSecFile);
                    UT_AudioEncodefclose((void *)OutputSecFile);
                    OutputSecFile = NULL;
                }
            }
            AmbaPrint("[Sec]Muxer met EOS");
        } else if (desc->Size == AMP_FIFO_MARK_EOS_PAUSE) {
            // do nothing
        } else {
            if (!(AudioEncDumpSkip & AUDENC_SKIP_SEC)) {
                if (OutputSecHeaderFile) {
                    UINT32 size = desc->Size;
                    UT_AudioEncodefwrite((const void *)&size, 1, sizeof(UINT32), (void *)OutputSecHeaderFile);
                }

                if (OutputSecFile) {
                    if (desc->StartAddr + desc->Size <= bitsLimit) {
                        UT_AudioEncodefwrite((const void *)desc->StartAddr, 1, desc->Size, (void *)OutputSecFile);
                    }else{
                        AmbaPrint("[Sec]Muxer Wrap Around");
                        UT_AudioEncodefwrite((const void *)desc->StartAddr, 1, bitsLimit - desc->StartAddr + 1, (void *)OutputSecFile);
                        UT_AudioEncodefwrite((const void *)AudSecBitsBuf, 1, desc->Size - (bitsLimit - desc->StartAddr + 1), (void *)OutputSecFile);
                    }
                }
            } else {
                AmbaKAL_TaskSleep(1);
            }
        }
        AmpFifo_RemoveEntry(AudioEncSecVirtualFifoHdlr, 1);
    }
}


static int AmpUT_AudioEnc_FifoCB(void *hdlr, UINT32 event, void* info)
{
    UINT32 *numFrames = info;
    AMBA_KAL_SEM_t *pSem;

    pSem = (hdlr == AudioEncPriVirtualFifoHdlr)? &AudioEncPriSem: &AudioEncSecSem;

    //AmbaPrint("AmpUT_AudioEnc_FifoCB on Event: 0x%x 0x%x", event, *numFrames);
    if (event == AMP_FIFO_EVENT_DATA_READY) {
        int i;

        for(i = 0; i < *numFrames; i++) {
            AmbaKAL_SemGive(pSem);
        }
    } else if (event == AMP_FIFO_EVENT_DATA_EOS) {
            AmbaKAL_SemGive(pSem);
    }

    return 0;
}


/*~muxer*/
static int AmpUT_AudioEncCallback(void *hdlr,UINT32 event, void *info)
{
    switch (event) {
        case AMP_ENC_EVENT_DATA_OVER_RUNOUT_THRESHOLD:
            AmbaPrint("AmpUT_AudioEnc: !!!!!!!!!!! AMP_ENC_EVENT_DATA_OVER_RUNOUT_THRESHOLD !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_DATA_OVERRUN:
            AmbaPrint("AmpUT_AudioEnc: !!!!!!!!!!! AMP_ENC_EVENT_DATA_OVERRUN !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_DESC_OVERRUN:
            AmbaPrint("AmpUT_AudioEnc: !!!!!!!!!!! AMP_ENC_EVENT_DESC_OVERRUN !!!!!!!!!!");
            break;
        case AMP_ENC_EVENT_DATA_FULLNESS_NOTIFY:
            if (0) {
                UINT32 *Percetage = (UINT32 *) info;
                AmbaPrint("AmpUT_AudioEnc: !!!!!!!!!!! AMP_ENC_EVENT_DATA_FULLNESS_NOTIFY, Percetage=%d!!!!!!!!!!",Percetage);
            }
            break;
        default:
            AmbaPrint("AmpUT_AudioEnc: Unknown %X info: %x", event, info);
            break;
    }
    return 0;
}


int AmpUT_AudioEnc_Init(void)
{
    int er;
    void *TmpbufRaw = NULL;

#if 0
    {  // force to run at core 1.
        AMBA_KAL_TASK_t *myTask = AmbaKAL_TaskIdentify();

        AmbaKAL_TaskSmpCoreExclusionSet(myTask, 1);
    }
#endif

    // Create simple muxer semophore
    if (AmbaKAL_SemCreate(&AudioEncPriSem, 0) != OK) {
        AmbaPrint("AudioEnc UnitTest: Semaphore creation failed");
    }
    if (AmbaKAL_SemCreate(&AudioEncSecSem, 0) != OK) {
        AmbaPrint("AudioEnc UnitTest: Semaphore creation failed");
    }


    // Create simple muxer task
    er = AmpUtil_GetAlignedPool(&G_MMPL, (void**) &AmpUT_AudioEncPriStack, &TmpbufRaw, AUDIO_ENC_PRI_MUX_TASK_STACK_SIZE, 32);
    if (er != OK) {
        AmbaPrint("Out of memory for muxer stack!!");
    }

    if (AmbaKAL_TaskCreate(&AudioEncPriMuxTask, "Audio Encoder UnitTest Primary Muxing Task", 50, \
         AmpUT_AudioEnc_PriMuxTask, 0x0, AmpUT_AudioEncPriStack, AUDIO_ENC_PRI_MUX_TASK_STACK_SIZE, AMBA_KAL_AUTO_START) != OK) {
         AmbaPrint("AudioEnc UnitTest: Primary Muxer task creation failed");
    }

    er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AmpUT_AudioEncSecStack, &TmpbufRaw, AUDIO_ENC_SEC_MUX_TASK_STACK_SIZE, 32);
    if (er != OK) {
        AmbaPrint("Out of memory for muxer stack!!");
    }

    if (AmbaKAL_TaskCreate(&AudioEncSecMuxTask, "Audio Encoder UnitTest Secondary Muxing Task", 50, \
         AmpUT_AudioEnc_SecMuxTask, 0x0, AmpUT_AudioEncSecStack, AUDIO_ENC_SEC_MUX_TASK_STACK_SIZE, AMBA_KAL_AUTO_START) != OK) {
         AmbaPrint("AudioEnc UnitTest: Secondary Muxer task creation failed");
    }

    // Init AUDIOENC module
    {
        AMP_AUDIOENC_INIT_CFG_s encInitCfg;

        AmpAudioEnc_GetInitDefaultCfg(&encInitCfg);
        if (AudioMainWorkBuf == NULL) {
            er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AudioMainWorkBuf, &TmpbufRaw, encInitCfg.MemoryPoolSize, 32);
            if (er != OK) {
                AmbaPrint("Out of memory for audio!!");
            }
        }
        encInitCfg.MemoryPoolAddr = AudioMainWorkBuf;
        AmpAudioEnc_Init(&encInitCfg);
    }

    return 0;
}

#ifdef CONFIG_SOC_A12
/*Effect Bypass callback funcs */
static void AmpUT_AudioEnc_EffectBypassSetup(AMBA_AUDIO_PLUGIN_EFFECT_CS_s *audio_cs)
{
    UINT32 i = 0,j = 0;
    UINT32 *signal= (UINT32 *)audio_cs->src;
    UINT32 *result = (UINT32 *)audio_cs->dest;
    UINT32 procsize = (UINT32)audio_cs->src_size;
    UINT32 channal = audio_cs->src_ch;

    /*clean buffer*/
    for(i = 0; i < channal; i++) {
        for(j = 0; j <procsize; j++) {
            *result = 0;
            *signal = 0;
        }
    }
}

static void AmpUT_AudioEnc_EffectBypassProc(AMBA_AUDIO_PLUGIN_EFFECT_CS_s *audio_cs)
{
    UINT32 i = 0,j = 0;
    UINT32 *signal= (UINT32 *)audio_cs->src;
    UINT32 *result = (UINT32*)audio_cs->dest;
    UINT32 procsize = (UINT32)audio_cs->src_size;
    UINT32 channal = audio_cs->src_ch;
    AIN_EFFECT_PROC_t *EffectBypassCfg = audio_cs->self;

    if (EffectBypassCfg->mute == 0) {
        for(i = 0; i < channal; i++ ) {
            for(j = 0 ; j <procsize; j++) {
                *result++ = *signal++;
            }
        }
    } else {
        for(i = 0 ; i < channal; i++) {
            for(j = 0; j <procsize; j++) {
                *result++ = 0;
            }
        }
    }
}

/* Create your own effect control APIs */
/* Install effect */
static void AmpUT_AudioEnc_EffectBypassInstall(UINT8 effectId)
{
    if (effectId < AMBA_AUDIO_DEFAULT_INPUT_EFFECT_NUM) {
        AmbaPrint("Wrong EffectID(%d)", effectId);
        return;
    }

    AudioAinEffectConfig.mute = 0;
    memset(&AudioAinEffectControl, 0x0, sizeof(AMBA_AUDIO_PLUGIN_EFFECT_CS_s));

    AudioAinEffectControl.setup = AmpUT_AudioEnc_EffectBypassSetup;
    AudioAinEffectControl.proc = AmpUT_AudioEnc_EffectBypassProc;
    AudioAinEffectControl.size_of_self = sizeof(AIN_EFFECT_PROC_t);
    AudioAinEffectControl.self = &AudioAinEffectConfig;
    AudioAinEffectControl.dest_auto_assign = 1;
    AudioAinEffectControl.dest_ch_auto_assign = 1;
    AudioAinEffectControl.dest_size_auto_assign = 1;

    /* sample code to operate PriInputHandler only */
    if (AudioEncInstance[0].InputHdlr) {
        AmbaAudio_InputPluginEffectInstall(AudioEncInstance[0].InputHdlr, effectId, &AudioAinEffectControl);
    }
}

/* Enable effect*/
static void AmpUT_AudioEnc_EffectBypassEnable(UINT8 effectId, UINT8 enable)
{
    if (effectId < AMBA_AUDIO_DEFAULT_INPUT_EFFECT_NUM) {
        AmbaPrint("Wrong EffectID(%d)", effectId);
        return;
    }

    /* sample code to operate PriInputHandler only */
    if (enable) {
        AmbaAudio_InputPluginEffectEnable(AudioEncInstance[0].InputHdlr, effectId);
    } else {
        AmbaAudio_InputPluginEffectDisable(AudioEncInstance[0].InputHdlr, effectId);
    }
}

/* Change effect parameters*/
static void AmpUT_AudioEnc_EffectBypassMute(UINT8 effectId, UINT16 mute)
{
    if (effectId < AMBA_AUDIO_DEFAULT_INPUT_EFFECT_NUM) {
        AmbaPrint("Wrong EffectID(%d)", effectId);
        return;
    }

    /* save tha value into shadow of the "control structure" to change parameters */
    AudioAinEffectConfigShadow.mute = mute;
    AudioAinEffectControlShadow.self = &AudioAinEffectConfigShadow;

    /* sample code to operate PriInputHandler only */
    if (AudioEncInstance[0].InputHdlr) {
        AmbaAudio_InputPluginEffectUpdate(AudioEncInstance[0].InputHdlr, effectId, &AudioAinEffectControlShadow);
    }
}
#endif
/* add audio encode task, which share same audio encode engine */
/* flag definition */
#define AUDIOENC_TASK_ADD_ALL           0x0
#define AUDIOENC_TASK_ADD_EXCLUDE_AIN   0x1
#define AUDIOENC_TASK_ADD_EXCLUDE_AENC  (0x1<<1)
int AmpUT_AudioEnc_Task_Add(UINT8 *workCacheBuf, UINT8 *workNCBuf, UINT32 cacheSize, UINT32 NCacheSize, UINT32 *inputHdlr, UINT32 *encodeHdlr, UINT32 *encBuffHdlr, UINT8 flag)
{
    UINT32 *retHdlr;
    AMBA_AUDIO_IO_CREATE_INFO_s inputInfo;
    AMBA_AUDIO_TASK_CREATE_INFO_s encInfo;
    AMBA_ABU_CREATE_INFO_s abuInfo;
    AMBA_AUDIO_BUF_INFO_s inC, inNonC;
    UINT32 inputCachedSize, inputNonCachedSize, encSize, abuSize;
    UINT32 *encAddr, *abuAddr;
    AMBA_AUDIO_COMBINE_INFO_s combine;
    UINT8 *Caddr, *NCaddr, *CaddrEnd, *NCaddrEnd;

    Caddr = (UINT8 *) ALIGN_4((UINT32)workCacheBuf);
    NCaddr = (UINT8 *) ALIGN_4((UINT32)workNCBuf);
    CaddrEnd = Caddr + cacheSize;
    NCaddrEnd = NCaddr + NCacheSize;

    if (flag&AUDIOENC_TASK_ADD_EXCLUDE_AIN) {
        //share ain
    } else {
        inputInfo.I2sIndex = 0; // depends on project?
        inputInfo.MaxChNum = 2;
        inputInfo.MaxDmaDescNum = 16; // depends on project?
        inputInfo.MaxDmaSize = 1024; // depends on project?
        inputInfo.MaxSampleFreq = 48000;
        inputCachedSize = AmbaAudio_InputCachedSizeQuery(&inputInfo);
        inputNonCachedSize = AmbaAudio_InputNonCachedSizeQuery(&inputInfo);
        inC.pHead = (UINT32 *)Caddr;
        inC.MaxSize = inputCachedSize;
        Caddr += inputCachedSize;
        if (Caddr > CaddrEnd) {
            AmbaPrint("AIN input Cache buffer fail");
        }

        inNonC.pHead = (UINT32 *)NCaddr;
        inNonC.MaxSize = inputNonCachedSize;
        NCaddr += inputNonCachedSize;
        if (NCaddr > NCaddrEnd) {
            AmbaPrint("AIN input NonCache buffer fail");
        }
        retHdlr = AmbaAudio_InputCreate(&inputInfo, &inC, &inNonC);
        if ((int)retHdlr == NG) {
            AmbaPrint("AIN cre fail");
        } else *inputHdlr = (UINT32)retHdlr;
    }

    if (flag&AUDIOENC_TASK_ADD_EXCLUDE_AENC) {
        //share aenc
    } else {
        encInfo.MaxSampleFreq = 48000;
        encInfo.MaxChNum = 2;
        encInfo.MaxFrameSize = 2048;
        encSize = AmbaAudio_EncSizeQuery(&encInfo);
        Caddr = (UINT8 *) ALIGN_4((UINT32)Caddr);
        encAddr = (UINT32 *)Caddr;
        Caddr += encSize;
        if (Caddr > CaddrEnd) {
            AmbaPrint("AENC buffer fail");
        }
        retHdlr = AmbaAudio_EncCreate(&encInfo, encAddr, encSize);
        if ((int)retHdlr == NG) {
            AmbaPrint("AENC cre fail");
        } else *encodeHdlr = (UINT32)retHdlr;
    }

    abuInfo.MaxSampleFreq = 48000;
    abuInfo.MaxChNum = 2;
    abuInfo.MaxChunkNum = 16; // depends on project?
    abuSize = AmbaAudio_BufferSizeQuery(&abuInfo);
    Caddr = (UINT8 *) ALIGN_4((UINT32)Caddr);
    abuAddr = (UINT32 *)Caddr;
    Caddr += abuSize;
    if (Caddr > CaddrEnd) {
        AmbaPrint("ABU buffer fail");
    }
    retHdlr = AmbaAudio_BufferCreate(&abuInfo, abuAddr, abuSize);
    if ((INT32)retHdlr == NG) {
        AmbaPrint("ABU cre fail");
    } else *encBuffHdlr = (UINT32)retHdlr;

    combine.pAbu = (UINT32 *)(*encBuffHdlr);
    combine.pSrcApu = (UINT32 *)(*inputHdlr);
    combine.pDstApu = (UINT32 *)(*encodeHdlr);
    if (AmbaAudio_Combine(&combine) != OK) {
        AmbaPrint("ACOMB fail");
    }

    return 0;
}

/* create audio pipe */
int AmpUT_AudioEnc_Create(UINT8 PriEncType, UINT8 SecEnType)
{
    int er;
    AMP_AUDIOENC_HDLR_CFG_s encCfg={0}, encSecCfg={0};
    AMBA_AUDIO_AACENC_CONFIG_s aacCfg = { AAC_BS_ADTS, 128000 }; // Encode a directly playable ADTS file
    AMBA_AUDIO_PCM_CONFIG_s pcmCfg = { .BitsPerSample = 16, // Encode a Intel 16bit PCM
                                       .DataFormat = 0,
                                       .FrameSize = 1024 };
    AMBA_AUDIO_ADPCM_CONFIG_s adpcmCfg = { .AdpcmFrameSize = 2048 };
    AMBA_AUDIO_AC3ENC_CONFIG_s ac3Cfg = { .Ac3EncAcmod = 2,
                                          .Ac3EncBitrate = 192000,
                                          .Ac3EncAgcEnable = 0,
                                          .Ac3EncAgcCh2Enable = 0,
                                          .Ac3EncDrcMode = 0,
                                          .Ac3EncLfeEnable = 0,
                                          .Ac3EncLfeFilterEnable = 0,
                                          .Ac3EncTestMode = 0,
                                          .Ac3EncSurroundDelayEnable = 0,
                                          .Ac3EncBsEndian = Audio_BS_Intel };
    AMBA_AUDIO_OPUSENC_CONFIG_s opusCfg = { .Bitrate = 128000,
                                          .FrameSize = (48000*20/1000),
                                          .BitstreamType = OPUS_BS_RTP };
    UINT32 Csize, NCsize;
    UINT32 retInputHdlr, retEncodeHdlr, retEncBuffHdlr;
    UINT8 audioEncMultiAin = (AudioMultiMode&AUDENC_MULTI_AIN)? 1: 0;
    UINT8 audioEncMultiStream = (AudioMultiMode&AUDENC_MULTI_STREAM)? 1: 0;

    aacCfg.BitstreamType = AacFormat;

    if (AudioEncPipe) {
        AmbaPrint("[Audio] Current Not Support multi audio pipe 0x%X", AudioEncPipe);
        return -1;
    }

    if (AudioMultiMode) {
        // Create Audio encoder object
        AmpAudioEnc_GetDefaultCfg(&encCfg);

        // Encoder setup
        encCfg.SrcChannelMode = 2; // currently audio_lib treat this as channel number
        encCfg.SrcSampleRate = 48000;
        AudioChannels[0] = encCfg.DstChannelMode = 2; // currently audio_lib treat this as channel number
        AudioSampleRate[0] = encCfg.DstSampleRate = 48000;

        if (PriEncType == 0) {
            encCfg.EncType = AMBA_AUDIO_AAC;
            encCfg.Spec.AACCfg = aacCfg;
            AudioBitsPerSample[0] = 16;
            AudioFrmsz_a[0] = 1024;
            AudioBitrate[0] = aacCfg.Bitrate;
        } else if (PriEncType == 1) {
            encCfg.EncType = AMBA_AUDIO_PCM;
            encCfg.Spec.PCMCfg = pcmCfg;
            AudioBitsPerSample[0] = pcmCfg.BitsPerSample;
            AudioFrmsz_a[0] = pcmCfg.FrameSize;
        } else if (PriEncType == 2) {
            encCfg.EncType = AMBA_AUDIO_AAC_PLUS;
            encCfg.Spec.AACCfg = aacCfg;
            AudioBitsPerSample[0] = 16;
            AudioFrmsz_a[0] = 1024;
            AudioBitrate[0] = aacCfg.Bitrate;
        } else if (PriEncType == 3) {
            encCfg.EncType = AMBA_AUDIO_AAC_PLUS_V2;
            encCfg.Spec.AACCfg = aacCfg;
            AudioBitsPerSample[0] = 16;
            AudioFrmsz_a[0] = 1024;
            AudioBitrate[0] = aacCfg.Bitrate;
        } else if (PriEncType == 4) {
            encCfg.EncType = AMBA_AUDIO_ADPCM;
            encCfg.Spec.ADPCMCfg = adpcmCfg;
            AudioBitsPerSample[0] = 16;
            AudioFrmsz_a[0] = adpcmCfg.AdpcmFrameSize;
        } else if (PriEncType == 5) {
            encCfg.EncType = AMBA_AUDIO_AC3;
            encCfg.Spec.AC3Cfg = ac3Cfg;
            AudioBitsPerSample[0] = 16;
            AudioFrmsz_a[0] = 1536;
            AudioBitrate[0] = ac3Cfg.Ac3EncBitrate;
        } else if (PriEncType == 6) {
            encCfg.EncType = AMBA_AUDIO_MPEG;
            // need to ask Audio
            AudioBitsPerSample[0] = 16;
            AudioFrmsz_a[0] = 1024;
        } else if (PriEncType == 7) {
            encCfg.EncType = AMBA_AUDIO_OPUS;
            encCfg.Spec.OpusCfg = opusCfg;
            AudioBitsPerSample[0] = 16;
            AudioFrmsz_a[0] = opusCfg.FrameSize;
            AudioBitrate[0] = opusCfg.Bitrate;
        }
        PriEncodeType = PriEncType;

        if (audioEncMultiStream) {
            encSecCfg.SrcChannelMode = 2; // currently audio_lib treat this as channel number
            encSecCfg.SrcSampleRate = 48000;
            AudioChannels[1] = encSecCfg.DstChannelMode = 2; // currently audio_lib treat this as channel number
            AudioSampleRate[1] = encSecCfg.DstSampleRate = 48000;
            if (SecEnType == 0) {
                encSecCfg.EncType = AMBA_AUDIO_AAC;
                encSecCfg.Spec.AACCfg = aacCfg;
                AudioBitsPerSample[1] = 16;
                AudioFrmsz_a[1] = 1024;
                AudioBitrate[1] = aacCfg.Bitrate;
            } else if (SecEnType == 1) {
                encSecCfg.EncType = AMBA_AUDIO_PCM;
                encSecCfg.Spec.PCMCfg = pcmCfg;
                AudioBitsPerSample[1] = pcmCfg.BitsPerSample;
                AudioFrmsz_a[1] = pcmCfg.FrameSize;
            } else if (SecEnType == 2) {
                encCfg.EncType = AMBA_AUDIO_AAC_PLUS;
                encCfg.Spec.AACCfg = aacCfg;
                AudioBitsPerSample[1] = 16;
                AudioFrmsz_a[1] = 1024;
                AudioBitrate[1] = aacCfg.Bitrate;
            } else if (SecEnType == 3) {
                encCfg.EncType = AMBA_AUDIO_AAC_PLUS_V2;
                encCfg.Spec.AACCfg = aacCfg;
                AudioBitsPerSample[1] = 16;
                AudioFrmsz_a[1] = 1024;
                AudioBitrate[1] = aacCfg.Bitrate;
            } else if (SecEnType == 4) {
                encCfg.EncType = AMBA_AUDIO_ADPCM;
                encCfg.Spec.ADPCMCfg = adpcmCfg;
                AudioBitsPerSample[1] = 16;
                AudioFrmsz_a[1] = adpcmCfg.AdpcmFrameSize;
            } else if (SecEnType == 5) {
                encCfg.EncType = AMBA_AUDIO_AC3;
                encCfg.Spec.AC3Cfg = ac3Cfg;
                AudioBitsPerSample[1] = 16;
                AudioFrmsz_a[1] = 1536;
                AudioBitrate[1] = ac3Cfg.Ac3EncBitrate;
            } else if (SecEnType == 6) {
                encCfg.EncType = AMBA_AUDIO_MPEG;
                AudioBitsPerSample[1] = 16;
                AudioFrmsz_a[1] = 1024;
            } else if (SecEnType == 7) {
                encCfg.EncType = AMBA_AUDIO_OPUS;
                encCfg.Spec.OpusCfg = opusCfg;
                AudioBitsPerSample[1] = 16;
                AudioFrmsz_a[1] = opusCfg.FrameSize;
                AudioBitrate[1] = opusCfg.Bitrate;
            }
            SecEncodeType = SecEnType;
        }

        // Task priority
        encCfg.EncoderTaskPriority = 15;
        encCfg.EncoderTaskCoreSelection = 1; //single core
        encCfg.InputTaskPriority = 12;
        encCfg.InputTaskCoreSelection = 1; //single core
        encCfg.EventDataReadySkipNum = 0;
        encCfg.FadeInTime = 0;
        encCfg.FadeOutTime = 0;
        if (audioEncMultiStream) {
            encSecCfg.EncoderTaskPriority = 15;
            encSecCfg.EncoderTaskCoreSelection = 1; //single core
            encSecCfg.InputTaskPriority = 12;
            encSecCfg.InputTaskCoreSelection = 1; //single core
            encSecCfg.EventDataReadySkipNum = 0;
            encSecCfg.FadeInTime = 0;
            encSecCfg.FadeOutTime = 0;
        }

        // Query working size
        {
            AMBA_AUDIO_IO_CREATE_INFO_s input;
            AMBA_AUDIO_TASK_CREATE_INFO_s enc;
            AMBA_ABU_CREATE_INFO_s abu;

            // Cache need 4_align, NonCache need 8_align
            // audio input
            input.I2sIndex = 0; // depends on HW design
            input.MaxChNum = 2;
            input.MaxDmaDescNum = 16; // depends chip/project, means more buffer
            input.MaxDmaSize = 1024; // depends chip/project, means more buffer
            input.MaxSampleFreq = 48000;
            Csize = ALIGN_4(AmbaAudio_InputCachedSizeQuery(&input));
            NCsize = ALIGN_8(AmbaAudio_InputNonCachedSizeQuery(&input));

            // audio encode
            enc.MaxSampleFreq = 48000;
            enc.MaxChNum = 2;
            enc.MaxFrameSize = 2048;
            Csize += ALIGN_4(AmbaAudio_EncSizeQuery(&enc));

            abu.MaxSampleFreq = 48000;
            abu.MaxChNum = 2;
            abu.MaxChunkNum = 16; // depends preject, means more buffer
            Csize += ALIGN_4(AmbaAudio_BufferSizeQuery(&abu));
        }

        if (AudCalibCurv) {
            if (AudworkNCRawBuf) {
                AmbaKAL_BytePoolFree((void *)OriAudworkNCRawBuf);
                AudworkNCBuf = AudworkNCRawBuf = NULL;
            }
            if (AudworkCacheRawBuf) {
                AmbaKAL_BytePoolFree((void *)OriAudworkCacheRawBuf);
                AudworkCacheBuf = AudworkCacheRawBuf = NULL;
            }
            encCfg.CalibCurveAddr = AudCalibCurv;
            encCfg.CalibworkBuffer = AudworkEffectCalibBuffer;
        }

        // Assign working buffer
        if (AudworkCacheRawBuf == NULL) {
            er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AudworkCacheRawBuf,(void **)&OriAudworkCacheRawBuf, Csize*2, 32);
            if (er != OK) {
                AmbaPrint("Out of Cache memory for audio working!!");
            }
            AudworkCacheBuf = AudworkCacheRawBuf;
        }

        if (AudworkNCRawBuf == NULL) {
            er = AmpUtil_GetAlignedPool(&G_NC_MMPL, (void **)&AudworkNCRawBuf,(void **)&OriAudworkNCRawBuf, NCsize*2, 32);
            if (er != OK) {
                AmbaPrint("Out of NC memory for audio working!!");
            }
            AudworkNCBuf = AudworkNCRawBuf;
        }

        // Assign callback
        encCfg.cbEvent = AmpUT_AudioEncCallback;

        // Assign bitstream/descriptor buffer
        if (AudPriBitsBuf == NULL) {
            er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AudPriBitsBuf,(void **)&OriAudPriBitsBuf, AUDENC_BISFIFO_SIZE, 32);
            if (er != OK) {
                AmbaPrint("Out of Cache memory for bitsFifo!!");
            }
        }
        encCfg.BitsBufCfg.BitsBufAddr = AudPriBitsBuf;
        encCfg.BitsBufCfg.BitsBufSize = AUDENC_BISFIFO_SIZE;

        if (AudPriDescBuf == NULL) {
            er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AudPriDescBuf, (void **)&OriAudPriDescBuf, AUDENC_DESC_SIZE, 32);
            if (er != OK) {
                AmbaPrint("Out of Cache memory for descFifo!!");
            }
        }

        encCfg.BitsBufCfg.DescBufAddr = AudPriDescBuf;
        encCfg.BitsBufCfg.DescBufSize = AUDENC_DESC_SIZE;
        encCfg.BitsBufCfg.BitsRunoutThreshold = AUDENC_BISFIFO_SIZE - 2*1024*1024; // leave 2MB
        AmbaPrint("[Pri]Bits 0x%X size %d Desc 0x%X size %d", AudPriBitsBuf, AUDENC_BISFIFO_SIZE, AudPriDescBuf, AUDENC_DESC_SIZE);

        if (audioEncMultiStream) {
            if (AudSecBitsBuf == NULL) {
                er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AudSecBitsBuf, (void **)&OriAudSecBitsBuf, AUDENC_BISFIFO_SIZE, 32);
                if (er != OK) {
                    AmbaPrint("Out of Cache memory for bitsFifo!!");
                }
            }
            encSecCfg.BitsBufCfg.BitsBufAddr = AudSecBitsBuf;
            encSecCfg.BitsBufCfg.BitsBufSize = AUDENC_BISFIFO_SIZE;

            if (AudSecDescBuf == NULL) {
                er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AudSecDescBuf, (void **)&OriAudSecDescBuf, AUDENC_DESC_SIZE, 32);
                if (er != OK) {
                    AmbaPrint("Out of Cache memory for descFifo!!");
                }
            }

            encSecCfg.BitsBufCfg.DescBufAddr = AudSecDescBuf;
            encSecCfg.BitsBufCfg.DescBufSize = AUDENC_DESC_SIZE;
            encSecCfg.BitsBufCfg.BitsRunoutThreshold = AUDENC_BISFIFO_SIZE - 2*1024*1024; // leave 2MB
            AmbaPrint("[Sec]Bits 0x%X size %d Desc 0x%X size %d", AudSecBitsBuf, AUDENC_BISFIFO_SIZE, AudSecDescBuf, AUDENC_DESC_SIZE);
        }

        //Create Audio Input, Encode and EncodeBuffer resource of 1st task
        if (audioEncMultiStream) {
            encCfg.NumInstance = 1;
            encSecCfg.NumInstance = 1;
            AmpUT_AudioEnc_Task_Add(AudworkCacheBuf, AudworkNCBuf, Csize, NCsize, &retInputHdlr, &retEncodeHdlr, &retEncBuffHdlr, AUDIOENC_TASK_ADD_ALL);
            AudioEncInstance[0].InputHdlr = (UINT32 *)retInputHdlr;
            AudioEncInstance[0].EncodeHdlr = (UINT32 *)retEncodeHdlr;
            AudioEncInstance[0].EncBuffHdlr = (UINT32 *)retEncBuffHdlr;

            AudworkCacheBuf += Csize;
            AudworkNCBuf += NCsize;
            AmpUT_AudioEnc_Task_Add(AudworkCacheBuf, AudworkNCBuf, Csize, NCsize, &retInputHdlr, &retEncodeHdlr, &retEncBuffHdlr, AUDIOENC_TASK_ADD_EXCLUDE_AIN);
            AudioEncSecInstance[0].InputHdlr = AudioEncInstance[0].InputHdlr;
            AudioEncSecInstance[0].EncodeHdlr = (UINT32 *)retEncodeHdlr;
            AudioEncSecInstance[0].EncBuffHdlr = (UINT32 *)retEncBuffHdlr;

            encCfg.AudioInstance = AudioEncInstance;
            encSecCfg.AudioInstance = AudioEncSecInstance;
        } else if (audioEncMultiAin) {
            AmpUT_AudioEnc_Task_Add(AudworkCacheBuf, AudworkNCBuf, Csize, NCsize, &retInputHdlr, &retEncodeHdlr, &retEncBuffHdlr, AUDIOENC_TASK_ADD_ALL);
            AudioEncInstance[0].InputHdlr = (UINT32 *)retInputHdlr;
            AudioEncInstance[0].EncodeHdlr = (UINT32 *)retEncodeHdlr;
            AudioEncInstance[0].EncBuffHdlr = (UINT32 *)retEncBuffHdlr;

            AudworkCacheBuf += Csize;
            AudworkNCBuf += NCsize;
            if (audioEncMultiStream) {
                encCfg.NumInstance = 1;
                encSecCfg.NumInstance = 1;
                AmpUT_AudioEnc_Task_Add(AudworkCacheBuf, AudworkNCBuf, Csize, NCsize, &retInputHdlr, &retEncodeHdlr, &retEncBuffHdlr, AUDIOENC_TASK_ADD_EXCLUDE_AIN);
                AudioEncSecInstance[0].InputHdlr = AudioEncInstance[0].InputHdlr;
                AudioEncSecInstance[0].EncodeHdlr = (UINT32 *)retEncBuffHdlr;
                AudioEncSecInstance[0].EncBuffHdlr = (UINT32 *)retEncBuffHdlr;
                encCfg.AudioInstance = AudioEncInstance;
                encSecCfg.AudioInstance = AudioEncSecInstance;
            } else {
                encCfg.NumInstance = 2;
                AmpUT_AudioEnc_Task_Add(AudworkCacheBuf, AudworkNCBuf, Csize, NCsize, &retInputHdlr, &retEncodeHdlr, &retEncBuffHdlr, AUDIOENC_TASK_ADD_EXCLUDE_AIN|AUDIOENC_TASK_ADD_EXCLUDE_AENC);
                AudioEncInstance[1].InputHdlr = AudioEncInstance[0].InputHdlr;
                AudioEncInstance[1].EncodeHdlr = AudioEncInstance[0].EncodeHdlr;
                AudioEncInstance[1].EncBuffHdlr = (UINT32 *)retEncBuffHdlr;
                encCfg.AudioInstance = AudioEncInstance;
            }
        } else {
            AmbaPrint("[Audio] what kind of multi mode you are?");
        }
    } else {
        // Create Audio encoder object
        AmpAudioEnc_GetDefaultCfg(&encCfg);

        // Encoder setup
        encCfg.SrcChannelMode = 2; // currently audio_lib treat this as channel number
        encCfg.SrcSampleRate = 48000;
        AudioChannels[0] = encCfg.DstChannelMode = 2; // currently audio_lib treat this as channel number
        AudioSampleRate[0] = encCfg.DstSampleRate = 48000;

        if (PriEncType == 0) {
            encCfg.EncType = AMBA_AUDIO_AAC;
            encCfg.Spec.AACCfg = aacCfg;
            AudioBitsPerSample[0] = 16;
            AudioFrmsz_a[0] = 1024;
            AudioBitrate[0] = aacCfg.Bitrate;
        } else if (PriEncType == 1) {
            encCfg.EncType = AMBA_AUDIO_PCM;
            encCfg.Spec.PCMCfg = pcmCfg;
            AudioBitsPerSample[0] = pcmCfg.BitsPerSample;
            AudioFrmsz_a[0] = pcmCfg.FrameSize;
        } else if (PriEncType == 2) {
            encCfg.EncType = AMBA_AUDIO_AAC_PLUS;
            encCfg.Spec.AACCfg = aacCfg;
            AudioBitsPerSample[0] = 16;
            AudioFrmsz_a[0] = 1024;
            AudioBitrate[0] = aacCfg.Bitrate;
        } else if (PriEncType == 3) {
            encCfg.EncType = AMBA_AUDIO_AAC_PLUS_V2;
            encCfg.Spec.AACCfg = aacCfg;
            AudioBitsPerSample[0] = 16;
            AudioFrmsz_a[0] = 1024;
            AudioBitrate[0] = aacCfg.Bitrate;
        } else if (PriEncType == 4) {
            encCfg.EncType = AMBA_AUDIO_ADPCM;
            encCfg.Spec.ADPCMCfg = adpcmCfg;
            AudioBitsPerSample[0] = 16;
            AudioFrmsz_a[0] = adpcmCfg.AdpcmFrameSize;
        } else if (PriEncType == 5) {
            encCfg.EncType = AMBA_AUDIO_AC3;
            encCfg.Spec.AC3Cfg = ac3Cfg;
            AudioBitsPerSample[0] = 16;
            AudioFrmsz_a[0] = 1536;
            AudioBitrate[0] = ac3Cfg.Ac3EncBitrate;
        } else if (PriEncType == 6) {
            encCfg.EncType = AMBA_AUDIO_MPEG;
            // need to ask Audio
            AudioBitsPerSample[0] = 16;
            AudioFrmsz_a[0] = 1024;
        } else if (PriEncType == 7) {
            encCfg.EncType = AMBA_AUDIO_OPUS;
            encCfg.Spec.OpusCfg = opusCfg;
            AudioBitsPerSample[0] = 16;
            AudioFrmsz_a[0] = opusCfg.FrameSize;
            AudioBitrate[0] = opusCfg.Bitrate;
        }

        PriEncodeType = PriEncType;

        // Task priority, input task should have higher priority than encode task
        encCfg.EncoderTaskPriority = 15;
        encCfg.EncoderTaskCoreSelection = 1; //single core
        encCfg.InputTaskPriority = 12;
        encCfg.InputTaskCoreSelection = 1; //single core
        encCfg.EventDataReadySkipNum = 0;
        encCfg.FadeInTime = 0;
        encCfg.FadeOutTime = 0;

        // Query working size
        {
            AMBA_AUDIO_IO_CREATE_INFO_s input;
            AMBA_AUDIO_TASK_CREATE_INFO_s enc;
            AMBA_ABU_CREATE_INFO_s abu;

            // Cache need 4_align, NonCache need 8_align
            // audio input
            input.I2sIndex = 0; // depends on HW design
            input.MaxChNum = 2;
            input.MaxDmaDescNum = 16; // depends chip/project, means more buffer
            input.MaxDmaSize = 1024; // depends chip/project, means more buffer
            input.MaxSampleFreq = 48000;
            Csize = ALIGN_4(AmbaAudio_InputCachedSizeQuery(&input));
            NCsize = ALIGN_8(AmbaAudio_InputNonCachedSizeQuery(&input));

            // audio encode
            enc.MaxSampleFreq = 48000;
            enc.MaxChNum = 2;
            enc.MaxFrameSize = 2048;
            Csize += ALIGN_4(AmbaAudio_EncSizeQuery(&enc));

            abu.MaxSampleFreq = 48000;
            abu.MaxChNum = 2;
            abu.MaxChunkNum = 16; // depends preject, means more buffer
            Csize += ALIGN_4(AmbaAudio_BufferSizeQuery(&abu));
        }

        // Assign working buffer
        if (AudworkCacheRawBuf == NULL) {
            er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AudworkCacheRawBuf, (void **)&OriAudworkCacheRawBuf, Csize, 32);
            if (er != OK) {
                AmbaPrint("Out of Cache memory for audio working!!");
            }
            AudworkCacheBuf = AudworkCacheRawBuf;
        }

        if (AudworkNCRawBuf == NULL) {
            er = AmpUtil_GetAlignedPool(&G_NC_MMPL, (void **)&AudworkNCRawBuf, (void **)&OriAudworkNCRawBuf, NCsize, 32);
            if (er != OK) {
                AmbaPrint("Out of NC memory for audio working!!");
            }
            AudworkNCBuf = AudworkNCRawBuf;
        }

        // Assign callback
        encCfg.cbEvent = AmpUT_AudioEncCallback;

        // Assign bitstream/descriptor buffer
        if (AudPriBitsBuf == NULL) {
            er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AudPriBitsBuf, (void **)&OriAudPriBitsBuf, AUDENC_BISFIFO_SIZE, 32);
            if (er != OK) {
                AmbaPrint("Out of Cache memory for bitsFifo!!");
            }
        }
        encCfg.BitsBufCfg.BitsBufAddr = AudPriBitsBuf;
        encCfg.BitsBufCfg.BitsBufSize = AUDENC_BISFIFO_SIZE;

        if (AudPriDescBuf == NULL) {
            er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AudPriDescBuf, (void **)&OriAudPriDescBuf, AUDENC_DESC_SIZE, 32);
            if (er != OK) {
                AmbaPrint("Out of Cache memory for descFifo!!");
            }
        }

        encCfg.BitsBufCfg.DescBufAddr = AudPriDescBuf;
        encCfg.BitsBufCfg.DescBufSize = AUDENC_DESC_SIZE;
        encCfg.BitsBufCfg.BitsRunoutThreshold = AUDENC_BISFIFO_SIZE - 2*1024*1024; // leave 2MB

        AmbaPrint("Bits 0x%X size %d Desc 0x%X size %d", AudPriBitsBuf, AUDENC_BISFIFO_SIZE, AudPriDescBuf, AUDENC_DESC_SIZE);

        //Create Audio Input, Encode and EncodeBuffer resource
        encCfg.NumInstance = 1;
        AmpUT_AudioEnc_Task_Add(AudworkCacheBuf, AudworkNCBuf, Csize, NCsize, &retInputHdlr, &retEncodeHdlr, &retEncBuffHdlr, AUDIOENC_TASK_ADD_ALL);
        AudioEncInstance[0].InputHdlr = (UINT32 *)retInputHdlr;
        AudioEncInstance[0].EncodeHdlr = (UINT32 *)retEncodeHdlr;
        AudioEncInstance[0].EncBuffHdlr = (UINT32 *)retEncBuffHdlr;

        encCfg.AudioInstance = AudioEncInstance;
    }

    // Assign bitstream-specific configs
    AmpAudioEnc_Create(&encCfg, &AudioEncPriHdlr);
    if (audioEncMultiStream) {
        AmpAudioEnc_Create(&encSecCfg, &AudioEncSecHdlr);
    }

    // Register pipeline
    {
        AMP_ENC_PIPE_CFG_s pipeCfg;
        // Register pipeline, audio pipe can support up to 16 encoder
        AmpEnc_GetDefaultCfg(&pipeCfg);
        //pipeCfg.cbEvent
        if (audioEncMultiStream) {
            pipeCfg.encoder[0] = AudioEncPriHdlr;
            pipeCfg.encoder[1] = AudioEncSecHdlr;
            pipeCfg.numEncoder = 2;
        } else {
            pipeCfg.encoder[0] = AudioEncPriHdlr;
            pipeCfg.numEncoder = 1;
        }
        pipeCfg.type = AMP_ENC_AV_PIPE;
        AmpEnc_Create(&pipeCfg, &AudioEncPipe);

        AmpEnc_Add(AudioEncPipe);
    }
    return 0;
}

int AmpUT_AudioEnc_EncodeStart(void)
{
    AMP_FIFO_CFG_s fifoDefCfg;

    if (AudioStatus != STATUS_AUDIO_IDLE)
        return -1;

    if (AudioEncPriVirtualFifoHdlr == NULL) {
        // create Primary virtual fifo
        AmpFifo_GetDefaultCfg(&fifoDefCfg);
        fifoDefCfg.hCodec = AudioEncPriHdlr;
        fifoDefCfg.IsVirtual = 1;
        fifoDefCfg.NumEntries = 1024;
        fifoDefCfg.cbEvent = AmpUT_AudioEnc_FifoCB;
        AmpFifo_Create(&fifoDefCfg, &AudioEncPriVirtualFifoHdlr);
    }
    if ((AudioMultiMode&AUDENC_MULTI_STREAM)==AUDENC_MULTI_STREAM && \
        AudioEncSecVirtualFifoHdlr == NULL) {
        // create Secondary virtual fifo
        AmpFifo_GetDefaultCfg(&fifoDefCfg);
        fifoDefCfg.hCodec = AudioEncSecHdlr;
        fifoDefCfg.IsVirtual = 1;
        fifoDefCfg.NumEntries = 1024;
        fifoDefCfg.cbEvent = AmpUT_AudioEnc_FifoCB;
        AmpFifo_Create(&fifoDefCfg, &AudioEncSecVirtualFifoHdlr);
    }

    AmpEnc_StartRecord(AudioEncPipe, 0);
    AudioStatus = STATUS_AUDIO_ENCODE;

    return 0;
}

int AmpUT_AudioEnc_EncStop(void)
{
    if (AudioStatus != STATUS_AUDIO_ENCODE && AudioStatus != STATUS_AUDIO_PAUSE)
        return -1;

    AmpEnc_StopRecord(AudioEncPipe, 0);
    AudioStatus = STATUS_AUDIO_IDLE;

    return 0;
}


int AmpUT_AudioEnc_EncPause(void)
{
    if (AudioStatus != STATUS_AUDIO_ENCODE)
        return -1;

    AmpEnc_PauseRecord(AudioEncPipe, 0);
    AudioStatus = STATUS_AUDIO_PAUSE;

    return 0;
}

int AmpUT_AudioEnc_EncResume(void)
{
    if (AudioStatus != STATUS_AUDIO_PAUSE)
        return -1;

    AmpEnc_ResumeRecord(AudioEncPipe, 0);
    AudioStatus = STATUS_AUDIO_ENCODE;

    return 0;
}

/* delete audio pipe */
int AmpUT_AudioEnc_Delete(void)
{
    AMBA_AUDIO_COMBINE_INFO_s combine;
    UINT8 i=0;

    AmpEnc_Delete(AudioEncPipe);
    AudioEncPipe = NULL;

    if(AudioEncPriHdlr) {
        AmpAudioEnc_Delete(AudioEncPriHdlr);
        AudioEncPriHdlr = NULL;
    }
    if(AudioEncSecHdlr) {
        AmpAudioEnc_Delete(AudioEncSecHdlr);
        AudioEncSecHdlr = NULL;
    }

    /* detache audio handler */
    for(i=0; i<2; i++) {
        if (AudioEncInstance[i].InputHdlr && \
            AudioEncInstance[i].EncodeHdlr && \
            AudioEncInstance[i].EncBuffHdlr) {
            combine.pAbu = AudioEncInstance[i].EncBuffHdlr;
            combine.pSrcApu = AudioEncInstance[i].InputHdlr;
            combine.pDstApu = AudioEncInstance[i].EncodeHdlr;
            AmbaAudio_Detach(&combine);
        }

        if ((AudioMultiMode&AUDENC_MULTI_STREAM) == AUDENC_MULTI_STREAM) {
            if (AudioEncSecInstance[i].InputHdlr && \
                AudioEncSecInstance[i].EncodeHdlr && \
                AudioEncSecInstance[i].EncBuffHdlr) {
                combine.pAbu = AudioEncSecInstance[i].EncBuffHdlr;
                combine.pSrcApu = AudioEncSecInstance[i].InputHdlr;
                combine.pDstApu = AudioEncSecInstance[i].EncodeHdlr;
                AmbaAudio_Detach(&combine);
            }
        }
    }

    /* delete audio handler */
    if ((AudioMultiMode&AUDENC_MULTI_STREAM) == AUDENC_MULTI_STREAM) {
        //share AIN
        if (AudioEncInstance[0].InputHdlr) {
            AmbaAudio_InputDelete(AudioEncInstance[0].InputHdlr);
        }
        for(i=0; i<2; i++) {
            AudioEncInstance[i].InputHdlr = NULL;
            AudioEncSecInstance[i].InputHdlr = NULL;
        }

        for(i=0; i<2; i++) {
            if (AudioEncInstance[i].EncodeHdlr) {
                AmbaAudio_EncDelete(AudioEncInstance[i].EncodeHdlr);
                AudioEncInstance[i].EncodeHdlr = NULL;
            }
            if (AudioEncSecInstance[i].EncodeHdlr) {
                AmbaAudio_EncDelete(AudioEncSecInstance[i].EncodeHdlr);
                AudioEncSecInstance[i].EncodeHdlr = NULL;
            }
        }

        for(i=0; i<2; i++) {
            if (AudioEncInstance[i].EncBuffHdlr) {
                AmbaAudio_BufferDelete(AudioEncInstance[i].EncBuffHdlr);
                AudioEncInstance[i].EncBuffHdlr = NULL;
            }
            if (AudioEncSecInstance[i].EncBuffHdlr) {
                AmbaAudio_BufferDelete(AudioEncSecInstance[i].EncBuffHdlr);
                AudioEncSecInstance[i].EncBuffHdlr = NULL;
            }
        }
    } else if ((AudioMultiMode&AUDENC_MULTI_AIN) == AUDENC_MULTI_AIN) {
        //share AIN
        if (AudioEncInstance[0].InputHdlr) {
            AmbaAudio_InputDelete(AudioEncInstance[0].InputHdlr);
        }
        for(i=0; i<2; i++) {
            AudioEncInstance[i].InputHdlr = NULL;
        }

        //share AENC
        if (AudioEncInstance[0].EncodeHdlr) {
            AmbaAudio_EncDelete(AudioEncInstance[0].EncodeHdlr);
        }
        for(i=0; i<2; i++) {
            AudioEncInstance[i].EncodeHdlr = NULL;
        }

        for(i=0; i<2; i++) {
            if (AudioEncInstance[i].EncBuffHdlr) {
                AmbaAudio_BufferDelete(AudioEncInstance[i].EncBuffHdlr);
                AudioEncInstance[i].EncBuffHdlr = NULL;
            }
        }
    } else {
        for(i=0; i<2; i++) {
            if (AudioEncInstance[i].InputHdlr) {
                AmbaAudio_InputDelete(AudioEncInstance[i].InputHdlr);
                AudioEncInstance[i].InputHdlr = NULL;
            }
            if (AudioEncInstance[i].EncodeHdlr) {
                AmbaAudio_EncDelete(AudioEncInstance[i].EncodeHdlr);
                AudioEncInstance[i].EncodeHdlr = NULL;
            }
            if (AudioEncInstance[i].EncBuffHdlr) {
                AmbaAudio_BufferDelete(AudioEncInstance[i].EncBuffHdlr);
                AudioEncInstance[i].EncBuffHdlr = NULL;
            }
        }
    }

    /* Free audio working memory */
    AmbaKAL_BytePoolFree((void *)OriAudworkCacheRawBuf);
    AudworkCacheBuf = AudworkCacheRawBuf = NULL;
    AmbaKAL_BytePoolFree((void *)OriAudworkNCRawBuf);
    AudworkNCBuf = AudworkNCRawBuf = NULL;

    /* Free bits */
    AmbaKAL_BytePoolFree((void *)OriAudPriBitsBuf);
    AudPriBitsBuf = NULL;
    if ((AudioMultiMode&AUDENC_MULTI_STREAM) == AUDENC_MULTI_STREAM) {
        AmbaKAL_BytePoolFree((void *)OriAudSecBitsBuf);
        AudSecBitsBuf = NULL;
    }

    AmbaKAL_BytePoolFree((void *)OriAudPriDescBuf);
    AudPriDescBuf = NULL;
    if ((AudioMultiMode&AUDENC_MULTI_STREAM) == AUDENC_MULTI_STREAM) {
        AmbaKAL_BytePoolFree((void *)OriAudSecDescBuf);
        AudSecDescBuf = NULL;
    }

    return 0;
}

int AmpUT_AudioInputCalibCB(INT8 *curve, INT32 *dBFs, INT32 *THD_N)
{
    int Rtval = 0;

    if(curve){
        AmbaPrint("Curve[0] %x,Curve[1] %x", *curve, *(curve+1));
        AudCalibCurv = curve;
    }
    if(dBFs){
        AmbaPrint("dBFs[0] %x,dBFs[1] %x", *dBFs, *(dBFs+1));
    }
    if(THD_N){
        AmbaPrint("THD_N[0] %x,THD_N[1] %x", *THD_N, *(THD_N+1));
    }
    AmbaPrint("%s: c=%x,dB=%x,THD=%x", __func__,curve,dBFs,THD_N);

    return Rtval;
}


int AmpUT_AudioEncTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("AmpUT_AudioEncTest cmd: %s", argv[1]);

    if (strcmp(argv[1],"init") == 0) {
        if (AudioStatus == STATUS_AUDIO_NONE) {
            AmpUT_AudioEnc_Init();
            AudioStatus = STATUS_AUDIO_IDLE;
        } else {
            AmbaPrint("[Audio] Already Init");
        }
    } else if ((strcmp(argv[1],"create") == 0) || (strcmp(argv[1],"cre") == 0)) {
        if (AudioStatus != STATUS_AUDIO_IDLE && \
            AudioStatus != STATUS_AUDIO_ENCODE) {
            AmbaPrint("[Audio] Non Init");
            return -1;
        }
        if (argc < 3) {
            AmbaPrint("Usage: t audioenc create [PriEncType] [SecEncType]");
            AmbaPrint("                EncType: encode type");
            AmbaPrint("                       0 : AAC");
            AmbaPrint("                       1 : PCM");
            AmbaPrint("                       2 : AAC_PLUS");
            AmbaPrint("                       3 : AAC_PLUS_V2");
            AmbaPrint("                       4 : ADPCM");
            AmbaPrint("                       5 : AC3");
            AmbaPrint("                       6 : MPEG3");
            AmbaPrint("                       7 : OPUS");
            AmbaPrint("             PriEncType: encode type");
            AmbaPrint("             SecEncType: encode type(optional, only enable when multi mode is set as multi-stream)");
            return -1;
        } else {
            UINT32 priEncType = atoi(argv[2]);
            UINT32 secEncType = atoi(argv[3]);
            AmpUT_AudioEnc_Create(priEncType, secEncType);
        }
    } else if ((strcmp(argv[1],"encstart") == 0) || (strcmp(argv[1],"enst") == 0)) {
        UINT32 tt = atoi(argv[2]);

        if (AudioStatus == STATUS_AUDIO_ENCODE) {
            AmbaPrint("[Audio] encode start during Encoding");
            return -1;
        }

        AmbaPrint("Encode for %d milliseconds", tt);
        AmpUT_AudioEnc_EncodeStart();
        if (tt != 0) {
            while (tt) {
                AmbaKAL_TaskSleep(1);
                tt--;
                if (tt == 0) {
                    AmpUT_AudioEnc_EncStop();
                }
                if (AudioStatus == STATUS_AUDIO_IDLE)
                    tt = 0;
            }
        }
    } else if ((strcmp(argv[1],"encstop") == 0) || (strcmp(argv[1],"ensp") == 0)) {
        if (AudioStatus != STATUS_AUDIO_ENCODE && AudioStatus != STATUS_AUDIO_PAUSE) {
            AmbaPrint("[Audio] encode stop during IDLE");
            return -1;
        }

        AmpUT_AudioEnc_EncStop();
    } else if ((strcmp(argv[1],"encpause") == 0) || (strcmp(argv[1],"enps") == 0)) {
        if (AudioStatus != STATUS_AUDIO_ENCODE) {
            AmbaPrint("[Audio] encode Pause during IDLE");
            return -1;
        }

        AmpUT_AudioEnc_EncPause();
    } else if ((strcmp(argv[1],"encresume") == 0) || (strcmp(argv[1],"enrs") == 0)) {
        if (AudioStatus != STATUS_AUDIO_PAUSE) {
            AmbaPrint("[Audio] encode Resume during BUSY/IDLE");
            return -1;
        }

        AmpUT_AudioEnc_EncResume();
    } else if ((strcmp(argv[1],"delete") == 0) || (strcmp(argv[1],"del") == 0)) {
        if (AudioStatus == STATUS_AUDIO_ENCODE) {
            AmbaPrint("[Audio] delete during Encodeing !!");
            return -1;
        }

        AmpUT_AudioEnc_Delete();
    } else if (strcmp(argv[1],"aacfmt") == 0) {
        UINT32 tt = atoi(argv[2]);
        if (AudioStatus == STATUS_AUDIO_ENCODE) {
            AmbaPrint("[Audio] set format during Encodeing !!");
            return -1;
        }
        if(tt == 0) {
            AacFormat = AAC_BS_ADTS;
        } else if(tt == 1) {
            AacFormat = AAC_BS_RAW;
        } else {
            AmbaPrint("Usage: t audioenc aacfmt [format]");
            AmbaPrint("             format: 0 : ADTS (default)");
            AmbaPrint("                     1 : RAW");
            return -1;
        }
        AmbaPrint("Change AAC format to %s", (tt == 0)?"AAC_BS_ADTS":"AAC_BS_RAW");

    } else if (strcmp(argv[1],"multi") == 0) {
        if (AudioStatus != STATUS_AUDIO_IDLE) {
            AmbaPrint("[Audio] enable multi-stream in Wrong status(%d) !!", AudioStatus);
            return -1;
        }

        if (argc < 3) {
            AmbaPrint("Usage: t audioenc multi [mode]");
            AmbaPrint("             mode: 0x1 : multi AIN, current only support up to 2 AIN");
            AmbaPrint("                   0x2 : multi stream encode, current only support up to 2 stream");
            return -1;
        } else {
            AudioMultiMode = (UINT8)atoi(argv[2]);
        }
    } else if (strcmp(argv[1],"logmuxer") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t audioenc logmuxer [en]");
            return -1;
        } else {
            AudioLogMux = (UINT8)atoi(argv[2]);
        }
    } else if (strcmp(argv[1],"dbg") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t audioenc dbg [lvl]");
            return -1;
        } else {
            UINT32 debugLevel = (UINT32)atoi(argv[2]);
            AmbaAudio_SetDebugLevel(debugLevel);
        }
    } else if (strcmp(argv[1],"dumpskip") == 0) {
        if (argc < 3) {
            AmbaPrint("Usage: t audioenc dumpskip [skipflag]");
            return -1;
        } else {
            AudioEncDumpSkip = (UINT8)atoi(argv[2]);
        }
    }

#ifdef CONFIG_SOC_A12
    else if (strcmp(argv[1],"calibcre") == 0) {
        AMBA_AUDIO_IO_CREATE_INFO_s inputInfo;
        int er;
        inputInfo.I2sIndex = 0; // depends on project?
        inputInfo.MaxChNum = 2;
        inputInfo.MaxDmaDescNum = 16; // depends on project?
        inputInfo.MaxDmaSize = 1024; // depends on project?
        inputInfo.MaxSampleFreq = 48000;

        // Assign working buffer
        if (AudworkCacheBuf == NULL) {
            er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AudworkCacheBuf, (void **)&OriAudworkCacheBuf, ALIGN_4(AmbaAudio_InputCachedSizeQuery(&inputInfo))*2, 32);
            if (er != OK) {
                AmbaPrint("Out of Cache memory for audio working!!");
            }
        }

        if (AudworkNCBuf == NULL) {
            er = AmpUtil_GetAlignedPool(&G_NC_MMPL, (void **)&AudworkNCRawBuf, (void **)&OriAudworkNCRawBuf, ALIGN_8(AmbaAudio_InputNonCachedSizeQuery(&inputInfo))*2, 32);
            if (er != OK) {
                AmbaPrint("Out of NC memory for audio working!!");
            }
            AudworkNCBuf = AudworkNCRawBuf;
        }

        er = AmpAudioCalib_Create(AudworkCacheBuf, AudworkNCBuf, &inputInfo);
        if (er != OK) {
            AmbaPrint("AudioCalib_Create fail!!");
        }

    } else if (strcmp(argv[1],"calibgetCurve") == 0) {
        AMBA_AUDIO_CALIB_CTRL_s CalibCtrl;
        int er;

        memset(&CalibCtrl, 0x0, sizeof(AMBA_AUDIO_CALIB_CTRL_s));
        //AmpAudioCalib_Disable();
        er = AmpUtil_GetAlignedPool(&G_MMPL, (void **)&AudworkEffectCalibBuffer, (void **)&OriAudworkEffectCalibBuffer, ALIGN_4(AmbaAudio_EffectCalibBufferSize()), 32);
        if (er != OK) {
            AmbaPrint("Out of EffectCalibBufferSize memory for audio working!!");
        }

        CalibCtrl.CalibMode = AUDIO_CALIB_PROC;
        CalibCtrl.CalibProcCtrl.CalibOperateMode = 0; /* 0: get audio calibration curve */
        CalibCtrl.CalibProcCtrl.pCalibNoiseThAddr = NULL;//CalibreThd; /* 1024(INT8)data array */
        CalibCtrl.CalibProcCtrl.pCalibRangeAddr = NULL;//CalibreRange; /* 1024(INT8)data array */
        CalibCtrl.CalibProcCtrl.CalibreFreqIdx = 0; /* 1k freqency idx: 42 ; 2k freqency idx: 85 */
        CalibCtrl.CalibProcCtrl.pCalibBuffer = AudworkEffectCalibBuffer; /* Process required buffers */
        AmbaPrint("AudworkEffectCalibBuffer = %x,%d",AudworkEffectCalibBuffer,ALIGN_4(AmbaAudio_EffectCalibBufferSize()));
        AmpAudioCalib_Setup(&CalibCtrl, AmpUT_AudioInputCalibCB);

    } else if (strcmp(argv[1],"calibDelete") == 0) {
        /* Free audio working memory */
        AmbaKAL_BytePoolFree((void *)OriAudworkEffectCalibBuffer);
        AudworkEffectCalibBuffer = NULL;
        AmbaKAL_BytePoolFree((void *)OriAudworkCacheBuf);
        AudworkCacheBuf = AudworkCacheRawBuf = NULL;
        AmbaKAL_BytePoolFree((void *)OriAudworkNCRawBuf);
        AudworkNCBuf = AudworkNCRawBuf = NULL;
        AudCalibCurv = NULL;
        AmpAudioCalib_Delete();
    } else if (strcmp(argv[1],"effect") == 0) {
        if (strcmp(argv[2], "install") == OK) {
            UINT8 Id = atoi(argv[3]);
            /* Install effect*/
            AmbaPrint("[%d]EffectBypassInstall", Id);
            AmpUT_AudioEnc_EffectBypassInstall(Id);
        } else if (strcmp(argv[2], "setup") == 0) {
            /*Audio effect initial setup*/
            (*AudioAinEffectControl.setup)(&AudioAinEffectControl);
        } else if (strcmp(argv[2], "enable") == 0) {
            UINT8 Id = atoi(argv[3]);
            UINT8 Enable = atoi(argv[4]);
            AmbaPrint("[%d]EffectBypassEnable = %d", Id, Enable);
            AmpUT_AudioEnc_EffectBypassEnable(Id, Enable);
        } else if (strcmp(argv[2], "set") == 0) {
            UINT8 Id = atoi(argv[3]);
            UINT8 Enable = atoi(argv[3]);
            AmbaPrint("[%d]EffectBypassMute = %d", Id, Enable);
            AmpUT_AudioEnc_EffectBypassMute(Id, Enable);/* mute output, 1: mute, 0: normal*/
        }
    }
#endif
    else {
        AmbaPrint("Usage: t audioenc init|create|encstart|encstop");
        AmbaPrint("       init: init all");
        AmbaPrint("       create [PriEncType][SecEncType]: create audio instance");
        AmbaPrint("       encstart [millisecond]: recording for N milliseconds");
        AmbaPrint("                               N = 0 will do continuous encode");
        AmbaPrint("       encstop: stop recording");
        AmbaPrint("       encpause: pause recording");
        AmbaPrint("       encresume: resume recording");
        AmbaPrint("       delete: delete audio instance");
        AmbaPrint("       multi [mode]: multi-stream encode");
#ifdef CONFIG_SOC_A12
        AmbaPrint("       calibcre: audio calib create");
        AmbaPrint("       calibgetCurve: Get Calibration for silence calibration curve");
        AmbaPrint("       calibDelete: delete audio calib");
        AmbaPrint("       effect: AIN effect control(For Cumster Reference only)");
        AmbaPrint("              install[id]: Install AIN effect (ID shall >= 5)");
        AmbaPrint("              setup: Setup AIN effect");
        AmbaPrint("              enable[id][en]: enable AIN effect or not");
        AmbaPrint("              set[id][param]: set AIN effect parameter");
#endif
    }

    return 0;
}

int AmpUT_AudioEncTestAdd(void)
{
    AmbaPrint("Adding AmpUT_AudioEnc");

    // hook command
    AmbaTest_RegisterCommand("ae", AmpUT_AudioEncTest);
    AmbaTest_RegisterCommand("audioenc", AmpUT_AudioEncTest);

   //AmpUT_AudioEnc_Init(0);

    return AMP_OK;
}
