/**
 *  @file AmpUT_AudioDec.c
 *  
 *  **History**
 *      |Date       |Name        |Comments       |
 *      |-----------|------------|---------------|
 *      |2013/7/20  |cyweng       |Created        |
 *
 *  @copyright 2013 Ambarella Corporation. All rights reserved.
 *                 No part of this file may be reproduced, stored in a retrieval system,
 *                 or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                 recording, or otherwise, without the prior consent of Ambarella Corporation.
 */

#include <player/Decode.h>
#include <player/AudioDec.h>
#include "AmpUnitTest.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cfs/AmpCfs.h>
#include <AmbaUtility.h>
#include <util.h>
#include "DecCFSWrapper.h"

static void* AudCodecModuleBufOri = NULL;   ///< Original buffer address of audio codec module
static void* AudRawBufOri = NULL;           ///< Original buffer address of audio decode raw file
static void* AudRawBuf = NULL;              ///< Aligned buffer address of audio decode raw file
static void* AudDescBufOri = NULL;          ///< Original buffer address of audio descripter
static void* AudCodecBufOri = NULL;         ///< Original buffer address of audio codec
static AMP_AVDEC_HDLR_s *AudDecHdlr = NULL;         ///< audio decode codec handler
static AMP_DEC_PIPE_HDLR_s *DecPipeHdlr = NULL;     ///< deocder pipe handler
static AMP_FIFO_HDLR_s *AudDecFifoHdlr = NULL;      ///< fifo to feed audio file
static AMP_AUDIODEC_HDLR_CFG_s AudCodecCfg;         ///< config for audio codec

static AMP_CFS_FILE_s *AudRawFile;                    ///< file pointer for audio file
static UINT32 AudRawFileSize = 0;                   ///< size of audio file

static AMBA_KAL_TASK_t AudioDecDemuxTask = {
        0 };     ///< task for audio file feeding
static AMBA_KAL_MUTEX_t AudioDecFrmMutex = {
        0 };     ///< mutex for frame feeding

#define AMP_UT_AUDIODEC_STACK_SIZE (8192)           ///< audio task stack size
UINT8 AmpUT_AudioDecStack[AMP_UT_AUDIODEC_STACK_SIZE];  ///< audio task stack

#define AUDIODEC_RAW_SIZE (10<<20)  ///< 10MB raw buffer
#define AUDIODEC_RAW_DESC_NUM (128) ///< descriptor number = 128
#define AUDIO_OUTPUT_TASK_PRIORITY (5)
#define AUDIO_DECODE_TASK_PRIORITY (6)

/**
 * The function is a task that feeding audio file to fifo.\n
 *
 * @param info - not used
 */
void AmpUT_AudioDec_DemuxTask(UINT32 info)
{
    AMP_BITS_DESC_s Desc;
    AMP_BITS_DESC_s TmpDesc = {
            0 };
    UINT32 FreeToEnd;
    UINT32 Remain;
    UINT32 FeedSz = 0;

    UINT8* RawBase = AudRawBuf;
    UINT8* RawLimit = (UINT8*) ((UINT32) AudRawBuf + AUDIODEC_RAW_SIZE - 1);

    static UINT32 TotalFeed = 0;
//#define FEED_MAX_SIZE   (8192)
#define FEED_MAX_SIZE   (32<<10)
    AmbaPrint("AmpUT_AudioDec_DemuxTask Start!");
    while (1) {
        UINT8 IsFeedEOS = 0; // Whether to feed EOS
        AmbaKAL_TaskSleep(1);
        AmbaKAL_MutexTake(&AudioDecFrmMutex, AMBA_KAL_WAIT_FOREVER );
        if (AudRawFile == NULL || AudRawFileSize == 0) {
            AmbaKAL_MutexGive(&AudioDecFrmMutex);
            TotalFeed = 0;
            AmbaKAL_TaskSleep(10);
            continue;
        }

        // Initialize descriptor
        SET_ZERO(TmpDesc);

        // prepare entry
        AmpFifo_PrepareEntry(AudDecFifoHdlr, &Desc);
        TmpDesc.StartAddr = Desc.StartAddr;
        //AmpCFS_fseek(audRawFile, 0, AMBA_FS_SEEK_START);
        if (Desc.Size < FEED_MAX_SIZE) {
            AmbaKAL_MutexGive(&AudioDecFrmMutex);
            AmbaKAL_TaskSleep(10);
            continue;
        }

        if (AudRawFileSize > FEED_MAX_SIZE) {
            FeedSz = FEED_MAX_SIZE;
            AudRawFileSize -= FEED_MAX_SIZE;
            IsFeedEOS = 0;
        } else {
            FeedSz = AudRawFileSize;
            AudRawFileSize = 0;
            IsFeedEOS = 1;
        }
        TmpDesc.Size = FeedSz;
        TotalFeed += FeedSz;
        AmbaPrint("Feed:%d Total:%d", FeedSz, TotalFeed);

        TmpDesc.Type = AMP_FIFO_TYPE_AUDIO_FRAME;

        if ((TmpDesc.StartAddr + TmpDesc.Size - 1) <= RawLimit) {
            AmpCFS_fread(TmpDesc.StartAddr, TmpDesc.Size, 1, AudRawFile);
        } else {
            FreeToEnd = RawLimit - TmpDesc.StartAddr + 1;
            Remain = TmpDesc.Size - FreeToEnd;
            AmpCFS_fread(TmpDesc.StartAddr, FreeToEnd, 1, AudRawFile);
            AmpCFS_fread(RawBase, Remain, 1, AudRawFile);
        }

        // write to fifo
        AmpFifo_WriteEntry(AudDecFifoHdlr, &TmpDesc);
        TmpDesc.SeqNum++;

        if (IsFeedEOS) {
            TmpDesc.Type = AMP_FIFO_TYPE_EOS;
            TmpDesc.Size = AMP_FIFO_MARK_EOS;
            TmpDesc.StartAddr = 0;
            TmpDesc.Pts = 0;

            // write to fifo
            AmpFifo_WriteEntry(AudDecFifoHdlr, &TmpDesc);
            TmpDesc.SeqNum++;
        }

        AmbaKAL_MutexGive(&AudioDecFrmMutex);
        //AmbaKAL_TaskSleep(1);  // TODO: wait FIFO fix....
    }
}

/**
 * fifo callback function.\n
 * we do not need information from this callback for now.\n
 * Please check fifo document for more detail it could provide.
 */
int AmpUT_AudioDec_DummyDmx_FifoCB(void *hdlr,
                                   UINT32 event,
                                   void* info)
{

    return 0;
}

/**
 * dummy demuxer function\n
 * init dummy audio demuxer.\n
 * the function will create dummy demuxer task in prepare fifo.
 */
static int AmpUT_AudioDec_DummyDmx_Init(void)
{
    AMP_FIFO_CFG_s FifoDefCfg = {
            0 };
    static UINT8 Init = 0;
    int Er = 0;

    if (Init != 0) {
        return 0;
    }
    Init = 1;

    // create fifo
    Er = AmpFifo_GetDefaultCfg(&FifoDefCfg);
    if (Er != OK) {
        AmbaPrint("AmpFifo_GetDefaultCfg failed: %d", Er);
        return -1;
    }
    FifoDefCfg.hCodec = AudDecHdlr;
    FifoDefCfg.IsVirtual = 1;
    FifoDefCfg.NumEntries = 128;
    FifoDefCfg.cbEvent = AmpUT_AudioDec_DummyDmx_FifoCB;
    Er = AmpFifo_Create(&FifoDefCfg, &AudDecFifoHdlr);
    if (Er != OK) {
        AmbaPrint("FIFO create failed: %d", Er);
        return -1;
    }

    /* Create task */
    Er = AmbaKAL_TaskCreate(&AudioDecDemuxTask, /* pTask */
    "UT_AudioDec_FWriteTask", /* pTaskName */
    17, /* Priority */
    AmpUT_AudioDec_DemuxTask, /* void (*EntryFunction)(UINT32) */
    0x0, /* EntryArg */
    (void *) AmpUT_AudioDecStack, /* pStackBase */
    AMP_UT_AUDIODEC_STACK_SIZE, /* StackByteSize */
    AMBA_KAL_AUTO_START); /* AutoStart */

    if (Er != OK) {
        AmbaPrint("Task create failed: %d", Er);
        return -1;
    }

    /* create mutex  for frame read and write to fifo*/
    Er = AmbaKAL_MutexCreate(&AudioDecFrmMutex);
    if (Er != OK) {
        AmbaPrint("Mutex create failed: %d", Er);
        return -1;
    }

    return 0;
}

/**
 * dummy demuxer function\n
 * To open file for feeding
 *
 * @param fn [in] - file to open.
 * @return 0: ok other: NG
 */
int AmpUT_AudioDec_DummyDmx_open(char* fn)
{
    char Mode[3] = "rb";

    AmbaKAL_MutexTake(&AudioDecFrmMutex, AMBA_KAL_WAIT_FOREVER );
    AmbaPrint("AmpUT_AudioDec_DummyDmx_open");
    // open file
    AudRawFile = AmpDecCFS_fopen(fn, Mode);
    if (AudRawFile == NULL ) {
        AmbaPrint("AmpUT_AudioDec_DummyDmx_open: raw open failed");
        return -1;
    }
    AmpCFS_fseek(AudRawFile, 0, AMBA_FS_SEEK_END);
    AudRawFileSize = AmpCFS_ftell(AudRawFile);
    AmpCFS_fseek(AudRawFile, 0, AMBA_FS_SEEK_START);

    AmbaKAL_MutexGive(&AudioDecFrmMutex);
    AmbaPrintColor(GREEN, "%s: File = 0x%08X  Size = %u", __FUNCTION__, AudRawFile, AudRawFileSize);
    AmbaPrint("AmpUT_AudioDec_DummyDmx_open end");
    return 0;
}

/**
 * dummy demuxer function\n
 * close demuxer
 *
 * @return 0: ok
 */
int AmpUT_AudioDec_DummyDmx_close(void)
{
    AmpCFS_fclose(AudRawFile);
    AudRawFile = NULL;
    AudRawFileSize = 0;
    return 0;
}

/**
 * callback function to recieve info from codec
 *
 * @param hdlr [in] - codec handler
 * @param event [in] - callback on
 * @param info [in] - cal lback info
 * @return
 */
int AmpUT_AudioDec_CodecCB(void* hdlr,
                           UINT32 event,
                           void* info)
{
    switch (event) {
    case AMP_DEC_EVENT_PLAYBACK_EOS:
        // NOTE: This event will only be invoked when the handler is a pure audio
        AmbaPrint("%s: AMP_DEC_EVENT_PLAYBACK_EOS", __FUNCTION__);
        break;
    default:
        AmbaPrint("%s: Callback on event %u", __FUNCTION__, event);
        break;
    }

    return 0;
}

extern UINT32* CreateAudioOutputCtrl(void);

/**
 * init audio decode unit test
 *
 * @return 0: OK
 */
int AmpUT_AudioDec_Init(void)
{
    AMP_AUDIODEC_INIT_CFG_s CodecInitCfg;
    AMP_DEC_PIPE_CFG_s PipeCfg;

    AMBA_AUDIO_TASK_CREATE_INFO_s DecInfo;
    AMBA_ABU_CREATE_INFO_s AbuInfo;
    UINT32 DecSize, AbuSize;
    int Er = 0;
    char *ErrMsg = "";

    AmbaPrint("%s", __FUNCTION__);

    /// init system output
    Er = AmpAudio_OutputInit(CreateAudioOutputCtrl(), AUDIO_OUTPUT_TASK_PRIORITY);
    if (Er != 0) {
        ErrMsg = "AmpAudio_OutputInit failed";
        goto ReturnError;
    }

    /// init codec module
    AmpAudioDec_GetInitDefaultCfg(&CodecInitCfg);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void**) &(CodecInitCfg.WorkBuff), &AudCodecModuleBufOri,
            CodecInitCfg.WorkBuffSize, 1 << 5) != AMP_OK) {
        AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
        ErrMsg = "AmpUtil_GetAlignedPool failed";
        goto ReturnError;
    }
    Er = AmpAudioDec_Init(&CodecInitCfg);
    if (Er != 0) {
        ErrMsg = "AmpAudioDec_Init failed";
        goto ReturnError;
    }

    /// create codec hdlr
    Er = AmpAudioDec_GetDefaultCfg(&AudCodecCfg);
    if (Er != 0) {
        ErrMsg = "AmpAudioDec_GetDefaultCfg failed";
        goto ReturnError;
    }
    // alloc memory
    if (AmpUtil_GetAlignedPool(&G_MMPL, &AudRawBuf, &AudRawBufOri, AUDIODEC_RAW_SIZE, 1 << 6) != AMP_OK) {
        AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
        ErrMsg = "AmpUtil_GetAlignedPool failed";
        goto ReturnError;
    }
    if (AudRawBufOri == NULL) {
        AmbaPrint("Alloc failed 1");
    }
    AudCodecCfg.RawBuffer = (UINT8*) AudRawBuf;
    AmbaPrint("%x -> %x", AudRawBufOri, AudCodecCfg.RawBuffer);
    AudCodecCfg.RawBufferSize = AUDIODEC_RAW_SIZE;

    if (AmpUtil_GetAlignedPool(&G_NC_MMPL, (void**) &(AudCodecCfg.DescBuffer), &AudDescBufOri,
            AUDIODEC_RAW_DESC_NUM * sizeof(AMP_BITS_DESC_s), 1 << 5) != AMP_OK) {
        AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
        ErrMsg = "AmpUtil_GetAlignedPool failed";
        goto ReturnError;
    }
    if (AudDescBufOri == NULL) {
        AmbaPrint("Alloc failed 2");
    }
    AudCodecCfg.DescBufferNum = AUDIODEC_RAW_DESC_NUM;

    AudCodecCfg.CbEvent = AmpUT_AudioDec_CodecCB;

    AudCodecCfg.DecoderTaskPriority = AUDIO_DECODE_TASK_PRIORITY;

    AudCodecCfg.PureAudio = PURE_AUDIO;

    AudCodecCfg.MaxChannelNum = 2;
    AudCodecCfg.MaxFrameSize = 4096;
    AudCodecCfg.MaxSampleRate = 48000;
    AudCodecCfg.MaxChunkNum = 16;
    AudCodecCfg.I2SIndex = 0;

    // TBD from demuxer
    if (0) { //PCM
        AudCodecCfg.DecType = AMBA_AUDIO_PCM;
        AudCodecCfg.DstSampleRate = 16000; //48000;
        AudCodecCfg.SrcSampleRate = 16000; //48000;
        AudCodecCfg.DstChannelMode = 2;
        AudCodecCfg.SrcChannelMode = 2;
        AudCodecCfg.FadeInTime = 0;
        AudCodecCfg.FadeOutTime = 0;

        //audCodecCfg.Spec.AACCfg.BitstreamType = AAC_BS_ADTS;
        AudCodecCfg.Spec.PCMCfg.BitsPerSample = 16;
        AudCodecCfg.Spec.PCMCfg.DataFormat = 0;
        AudCodecCfg.Spec.PCMCfg.FrameSize = 1024;
    } else {
        AudCodecCfg.DecType = AMBA_AUDIO_AAC;
        AudCodecCfg.DstSampleRate = 48000;
        AudCodecCfg.SrcSampleRate = 48000;
        AudCodecCfg.DstChannelMode = 2;
        AudCodecCfg.SrcChannelMode = 2;
        AudCodecCfg.FadeInTime = 0;
        AudCodecCfg.FadeOutTime = 0;

        AudCodecCfg.Spec.AACCfg.BitstreamType = AAC_BS_ADTS;
    }
    DecInfo.MaxSampleFreq = AudCodecCfg.MaxSampleRate;
    DecInfo.MaxChNum = AudCodecCfg.MaxChannelNum;
    DecInfo.MaxFrameSize = AudCodecCfg.MaxFrameSize;
    DecSize = AmbaAudio_DecSizeQuery(&DecInfo);

    AbuInfo.MaxSampleFreq = AudCodecCfg.MaxSampleRate;
    AbuInfo.MaxChNum = AudCodecCfg.MaxChannelNum;
    AbuInfo.MaxChunkNum = AudCodecCfg.MaxChunkNum;
    AbuSize = AmbaAudio_BufferSizeQuery(&AbuInfo);

    AudCodecCfg.CodecCacheWorkSize = DecSize + AbuSize;
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void**) &(AudCodecCfg.CodecCacheWorkBuff), &AudCodecBufOri,
            AudCodecCfg.CodecCacheWorkSize, 1 << 6) != AMP_OK) {
        AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
        ErrMsg = "AmpUtil_GetAlignedPool failed";
        goto ReturnError;
    }
    AmbaPrint("codec buffer : 0x%x, %d", AudCodecCfg.CodecCacheWorkBuff, AudCodecCfg.CodecCacheWorkSize);

    Er = AmpAudioDec_Create(&AudCodecCfg, &AudDecHdlr);
    if (Er != 0) {
        ErrMsg = "AmpAudioDec_Create failed";
        goto ReturnError;
    }

    /// create dec mgr
    Er = AmpDec_GetDefaultCfg(&PipeCfg);
    if (Er != 0) {
        ErrMsg = "AmpDec_GetDefaultCfg failed";
        goto ReturnError;
    }
    //pipeCfg.cbEvent
    PipeCfg.Decoder[0] = AudDecHdlr;
    PipeCfg.NumDecoder = 1;
    PipeCfg.Type = AMP_DEC_VID_PIPE;
    Er = AmpDec_Create(&PipeCfg, &DecPipeHdlr);
    if (Er != 0) {
        ErrMsg = "AmpDec_Create failed";
        goto ReturnError;
    }

    // active pipe
    Er = AmpDec_Add(DecPipeHdlr);
    if (Er != 0) {
        ErrMsg = "AmpDec_Add failed";
        goto ReturnError;
    }

    // dummy mux
    Er = AmpUT_AudioDec_DummyDmx_Init();
    if (Er != 0) {
        ErrMsg = "AmpUT_AudioDec_DummyDmx_init failed";
        goto ReturnError;
    }

    AmbaPrint("%s OK", __FUNCTION__);
    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}

/**
 * start to decode file
 *
 * @param argv [in] - argv[2]: file name argv[3]: file type argv[4]: source file sample rate
 * @return 0: ok
 */
int AmpUT_AudioDec_Start(char **argv)
{
    AMP_AVDEC_TRICKPLAY_s Trick = {
            0 };
    AMP_AUDIODEC_DECODER_CFG_s Cfg;
    int Er = 0;
    char *ErrMsg = "";

    memset(&Cfg, 0, sizeof(Cfg));

    AmbaPrint("%s", __FUNCTION__);

    // file open
    Er = AmpUT_AudioDec_DummyDmx_open(argv[2]);
    if (Er != 0) {
        ErrMsg = "AmpUT_AudioDec_DummyDmx_open failed";
        goto ReturnError;
    }

    AmbaKAL_TaskSleep(100);

    Cfg.SrcSampleRate = atoi(argv[4]);
    if (Cfg.SrcSampleRate == 0) {
        Cfg.SrcSampleRate = 48000;
    }

    if (strcmp(argv[3], "pcm") == 0) {
        Cfg.DecType = AMBA_AUDIO_PCM;
        Cfg.Spec.PCMCfg.BitsPerSample = 16;
        Cfg.Spec.PCMCfg.DataFormat = 0;
        Cfg.Spec.PCMCfg.FrameSize = 1024;
    } else if (strcmp(argv[3], "aac") == 0) {
        Cfg.DecType = AMBA_AUDIO_AAC;
        Cfg.Spec.AACCfg.BitstreamType = AAC_BS_ADTS;
    } else if (strcmp(argv[3], "opus") == 0) {
        AmbaPrint("format opus");
        Cfg.Spec.OPUSCfg.BitstreamType = OPUS_BS_RTP;
        Cfg.Spec.OPUSCfg.FrameSize = Cfg.SrcSampleRate * 20 / 1000;
        Cfg.DecType = AMBA_AUDIO_OPUS;
    } else {
        AmbaPrint("%s: Format (%s) not supported", __FUNCTION__, argv[3]);
        goto ReturnError;
    }
    Cfg.FadeInTime = 0;
    Cfg.FadeOutTime = 0;
    Cfg.SrcChannelMode = 2;

    Er = AmpAudioDec_DecoderCfg(&Cfg, AudDecHdlr);
    if (Er != 0) {
        ErrMsg = "AmpAudioDec_DecoderCfg failed";
        goto ReturnError;
    }

    Trick.Speed = 0x100;
    Er = AmpDec_Start(DecPipeHdlr, &Trick, NULL);
    if (Er != 0) {
        ErrMsg = "AmpDec_Start failed";
        goto ReturnError;
    }

    AmbaPrint("%s OK", __FUNCTION__);
    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}

/**
 * pause audio decode
 *
 * @return 0: ok
 */
int AmpUT_AudioDec_Pause(void)
{
    return 0;
}

/**
 * resume audio decode
 *
 * @return 0: ok
 */
int AmpUT_AudioDec_Resume(void)
{
    return 0;
}

/**
 * stop audio decode
 *
 * @return 0: ok
 */
int AmpUT_AudioDec_Stop(void)
{
    int Er = 0;
    char *ErrMsg = "";
    AmbaPrint("%s", __FUNCTION__);

    // Close dummy demuxer
    Er = AmpUT_AudioDec_DummyDmx_close();
    if (Er != 0) {
        ErrMsg = "AmpUT_AudioDec_DummyDmx_close failed";
        goto ReturnError;
    }

    Er = AmpFifo_EraseAll(AudDecFifoHdlr);
    if (Er != 0) {
        ErrMsg = "AmpFifo_EraseAll failed";
        goto ReturnError;
    }

    // Stop decoder
    Er = AmpDec_Stop(DecPipeHdlr);
    if (Er != 0) {
        ErrMsg = "AmpDec_Stop failed";
        goto ReturnError;
    }

    AmbaPrint("%s OK", __FUNCTION__);
    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}

void AmpUT_AudioDec_AudDbg(UINT32 debugLevel)
{
    AmbaAudio_SetDebugLevel(debugLevel);
}

void AmpUT_AudioDec_Usage(void)
{
    AmbaPrint("AmpUT_AudioDec");
    AmbaPrint("\t init - initialize audio decoder");
    AmbaPrint("\t start [filename] [fileType] [sampleRate] - fileType: pcm/aac");
    AmbaPrint("\t stop - stop decoding");
    AmbaPrint("\t audiodbg [lvl] - set audio debug");
}

int AmpUT_AudioDecTest(struct _AMBA_SHELL_ENV_s_ *env,
                       int argc,
                       char **argv)
{
    AmbaPrint("AmpUT_AudioDecTest cmd: %s", argv[1]);
    if (strcmp(argv[1], "init") == 0) {
        AmpUT_AudioDec_Init();
    } else if (strcmp(argv[1], "start") == 0) {
        AmpUT_AudioDec_Start(argv);
    } else if (strcmp(argv[1], "stop") == 0) {
        AmpUT_AudioDec_Stop();
    } else if (strcmp(argv[1], "exit") == 0) {
        //AmpUT_AudioDec_Stop();
    } else if (strcmp(argv[1],"audiodbg") == 0) {
        AmpUT_AudioDec_AudDbg((UINT32)atoi(argv[2]));
    } else {
        AmpUT_AudioDec_Usage();
    }
    return 0;
}

int AmpUT_AudioDecTestAdd(void)
{
    AmbaPrint("Adding AmpUT_AudioDec");

    // hook command
    AmbaTest_RegisterCommand("audiodec", AmpUT_AudioDecTest);
    AmbaTest_RegisterCommand("ad", AmpUT_AudioDecTest);

    return AMP_OK;
}

