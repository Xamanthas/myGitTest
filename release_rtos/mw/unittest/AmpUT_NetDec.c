/**
 *  @file AmpUT_NetDec.c
*
 *  @copyright 2014 Ambarella Corporation. All rights reserved.
 *  No part of this file may be reproduced, stored in a retrieval system,
 *  or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *  recording, or otherwise, without the prior consent of Ambarella Corporation.
 */

#include <net/NetFifo.h>
#include <recorder/Encode.h>
#include "AmpUnitTest.h"
#include "AmbaUtility.h"
#include <AmbaCache_Def.h>
#include "DummyVideoEnc_Net.h"
#include <util.h>
#include <net/NetUtility.h>
#include <AmbaTimer.h>

#include <AmbaLCD.h>
#include "AmbaCache.h"
#include "AmbaPLL.h"
#include "VideoDummyDemux.h"
#include "DecCFSWrapper.h"
#include "AmpUT_Display.h"
#include <net/NetFifo.h>
/* For Standby mode test */
#include "AmbaINT.h"
#include "AmbaLink.h"

#define BS_FN   "C:\\OUT_0000.h264" /**< H264 bitstream file name */
#define HDR_FN  "C:\\OUT_0000.nhnt" /**< nhnt file name */
#define UDTA_FN "C:\\OUT_0000.udta" /**< user define data file name */
#define AMPUT_NETFIFO_CODEC_HDLR ((void *)0x5678) /**< codec handler */
#define AMPUT_NETFIFO_ENC_RAW_SIZE (10 << 20) /**< 10MB raw buffer */
#define AMPUT_NETFIFO_MAX_FIFO_NUM (256) /**< number of entry of FIFO */

#define AMPUT_NETFIFO_MEDIA_STREAMID (0x0101)
#define AmpUT_NETDEC_MEDIA_STREAMID (0xaabb)

static void *g_EncBuffer = NULL; /**< Encode buffer */
static AMP_FIFO_HDLR_s *g_EncFifoHdlr = NULL; /**< encode FIFO handler */
//static AMP_FIFO_HDLR_s *g_AudEncHdlr = NULL;
static AMP_FIFO_HDLR_s *g_DefFifoHdlr = NULL; /**< default AMP_NETFIFO handler */
static FORMAT_USER_DATA_s g_Udta; /**< user define data */
static DummyVideoEnc_Net_s g_Encoder; /**< video encoder */
static int g_EncStatus = 0;
static UINT8 AmpUT_NetDecInited = 0;
static UINT8 AmpUT_enVideo = 0;
static UINT8 AmpUT_enAudio = 0;
static int AmpUT_nTrack = 0;
static int playStart = 0;
/* NetDec definitions*/

#define VIDEODEC_FRAME_COUNT_EOS (0xFFFFFFFF) ///< VideoDecFrmCnt of 0xFFFFFFFF indicates EOS
#define VIDEODEC_RAW_SIZE (32<<20) ///< 32MB raw buffer
#define VIDEODEC_RAW_DESC_NUM (128) ///< descriptor number = 128
#define AmpUT_NetVideoDec_MEDIA_STREAMID (0xaabb)

typedef struct _AMP_NETDEC_INFO_s {
    char fn[256];
    void *DmxHdlr;
    unsigned int VideoWidth;
    unsigned int VideoHeight;
    unsigned int StartTime;
    unsigned int Speed;
    unsigned int PreFeedNum;
    unsigned char Direction;
    unsigned char FadeEnable;
} AMP_NETDEC_INFO_s;

// global var
static void* AvcCodecBufferOri = NULL; ///< Original buffer address of video codec
static void* AvcRawBufferOri = NULL;   ///< Original buffer address of video decode raw file
static void* AvcRawBuffer = NULL;      ///< Aligned buffer address of video decode raw file
static void* AvcDescBufferOri = NULL;  ///< Original buffer address of video descripter
static void* AvcDescBuffer = NULL;     ///< Aligned buffer address of video descripter
static AMP_AVDEC_HDLR_s *AvcDecHdlr = NULL;     ///< video codec handler
static AMP_DEC_PIPE_HDLR_s *DecPipeHdlr = NULL; ///< decode mgr pipe handler
static UINT8 VideoDec_Init = 0; // Whether the video decoder is initialized
static UINT8 AudioDec_Init = 0; // Audio output initialized
// feature ctrl
static AMP_AVDEC_PLAY_DIRECTION_e PlayDirection = AMP_VIDEO_PLAY_FW; ///< Play direction
static AMP_NETDEC_INFO_s netdec_info = {0};

// Dummy demux
//static AMP_UT_DUMMY_DEMUX_HDLR_s* DmxHdlr = NULL;  ///< Dummy demux handler
/* end of NetDec definitions */

/* Audio NetDec definitions*/
#include <player/Decode.h>
#include <player/AudioDec.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static void* AudCodecModuleBufOri = NULL;   ///< Original buffer address of audio codec module
static void* AudRawBufOri = NULL;           ///< Original buffer address of audio decode raw file
static void* AudRawBuf = NULL;              ///< Aligned buffer address of audio decode raw file
static void* AudDescBufOri = NULL;          ///< Original buffer address of audio descripter
static void* AudCodecBufOri = NULL;         ///< Original buffer address of audio codec
static AMP_AVDEC_HDLR_s *AudDecHdlr = NULL;         ///< audio decode codec handler
//static AMP_DEC_PIPE_HDLR_s *DecPipeHdlr = NULL;     ///< deocder pipe handler
static AMP_FIFO_HDLR_s *AudDecFifoHdlr = NULL;      ///< fifo to feed audio file
static AMP_AUDIODEC_HDLR_CFG_s AudCodecCfg;         ///< config for audio codec

static AMP_CFS_FILE_s *AudRawFile;                    ///< file pointer for audio file
static UINT32 AudRawFileSize = 0;                   ///< size of audio file

static AMBA_KAL_TASK_t AudioDecDemuxTask = {
        0 };     ///< task for audio file feeding
static AMBA_KAL_MUTEX_t AudioDecFrmMutex = {
        0 };     ///< mutex for frame feeding

#define AMP_UT_AUDIODEC_STACK_SIZE (8192)           ///< audio task stack size
static UINT8 AmpUT_AudioDecStack[AMP_UT_AUDIODEC_STACK_SIZE];  ///< audio task stack

#define AUDIODEC_RAW_SIZE (10<<20)  ///< 10MB raw buffer
#define AUDIODEC_RAW_DESC_NUM (128) ///< descriptor number = 128
#define AUDIO_OUTPUT_TASK_PRIORITY (5)
#define AUDIO_DECODE_TASK_PRIORITY (6)

extern UINT32* CreateAudioOutputCtrl(void);
/* end of Audio definitions*/

#define UT_NETDEC_STACK_SIZE 0x2000
#define UT_NETDEC_MSGQUEUE_SIZE 16
static UINT8 Stack[UT_NETDEC_STACK_SIZE];  /**< Stack */
static UINT8 MsgPool[sizeof(int)*UT_NETDEC_MSGQUEUE_SIZE];   /**< Message memory pool. */
static AMBA_KAL_TASK_t Task;               /**< Task ID */
static AMBA_KAL_MSG_QUEUE_t MsgQueue;      /**< Message queue ID */

/**
 * The function is a task that feeding audio file to fifo.\n
 *
 * @param info - not used
 */
static void AmpUT_AudioDec_DemuxTask(UINT32 info)
{
    AMP_BITS_DESC_s TmpDesc = {
            0 };
    UINT32 FreeToEnd;
    UINT32 Remain;
    UINT32 FeedSz = 0;

    //UINT8* RawBase = AudRawBuf;
    //UINT8* RawLimit = (UINT8*) ((UINT32) AudRawBuf + AUDIODEC_RAW_SIZE - 1);
    UINT8* RawBase = (UINT8*) g_EncBuffer;
    UINT8* RawLimit = (UINT8*) g_EncBuffer + AMPUT_NETFIFO_ENC_RAW_SIZE;
    UINT8 *RingAddr = RawBase;
    static UINT32 TotalFeed = 0;
//#define FEED_MAX_SIZE   (8192)
//#define FEED_MAX_SIZE   (32<<10)
#define FEED_MAX_SIZE   (1400)
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
        //AmpFifo_PrepareEntry(g_EncFifoHdlr, &Desc);
        TmpDesc.StartAddr = RingAddr;
        //AmpCFS_fseek(audRawFile, 0, AMBA_FS_SEEK_START);
        /*if (Desc.Size < FEED_MAX_SIZE) {
            AmbaKAL_MutexGive(&AudioDecFrmMutex);
            AmbaPrint("Desc size to small, %d", Desc.Size);
            AmbaKAL_TaskSleep(10);
            continue;
        }*/

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
        AmbaKAL_TaskSleep(19);
        TmpDesc.Type = AMP_FIFO_TYPE_AUDIO_FRAME;

        if ((TmpDesc.StartAddr + FeedSz - 1) <= RawLimit) {
            AmpCFS_fread(TmpDesc.StartAddr, TmpDesc.Size, 1, AudRawFile);
            RingAddr += TmpDesc.Size;
        } else {
            FreeToEnd = RawLimit - TmpDesc.StartAddr + 1;
            Remain = TmpDesc.Size - FreeToEnd;
            AmpCFS_fread(TmpDesc.StartAddr, FreeToEnd, 1, AudRawFile);
            AmpCFS_fread(RawBase, Remain, 1, AudRawFile);
            RingAddr += Remain;
        }

        // write to fifo
        AmpFifo_WriteEntry(g_EncFifoHdlr, &TmpDesc);
        TmpDesc.SeqNum++;

        if (IsFeedEOS) {
            TmpDesc.Type = AMP_FIFO_TYPE_EOS;
            TmpDesc.Size = AMP_FIFO_MARK_EOS;
            TmpDesc.StartAddr = 0;
            TmpDesc.Pts = 0;

            // write to fifo
            AmpFifo_WriteEntry(g_EncFifoHdlr, &TmpDesc);
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
static int AmpUT_AudioDec_DummyDmx_FifoCB(void *hdlr,
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
int AmpUT_NetAudioDec_DummyDmx_Init(void)
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
//    FifoDefCfg.hCodec = AMPUT_NETFIFO_CODEC_HDLR; //write to encoder fifo
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
int AmpUT_NetAudioDec_DummyDmx_open(void)
{
    char Mode[3] = "rb";
    char file[32] = "c:\\adts_48k.aac";
    AmbaKAL_MutexTake(&AudioDecFrmMutex, AMBA_KAL_WAIT_FOREVER );
    AmbaPrint("AmpUT_AudioDec_DummyDmx_open %s", file);
    // open file
    AudRawFile = AmpDecCFS_fopen(file, Mode);
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
int AmpUT_NetAudioDec_DummyDmx_close(void)
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
static int AmpUT_AudioDec_CodecCB(void* hdlr,
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



/**
 * init audio decode unit test
 *
 * @return 0: OK
 */
static int AmpUT_NetAudioDec_Init(void)
{
    AMP_AUDIODEC_INIT_CFG_s CodecInitCfg;

    AMBA_AUDIO_TASK_CREATE_INFO_s DecInfo;
    AMBA_ABU_CREATE_INFO_s AbuInfo;
    UINT32 DecSize, AbuSize;
    int Er = 0;
    char *ErrMsg = "";

    AmbaPrint("%s", __FUNCTION__);

    /// init system output
    if (AudioDec_Init == 1) {
        AmbaPrint("%s: NetAudioDec already inited", __FUNCTION__);
        return 0;
    }

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

    AudCodecCfg.PureAudio = VIDEO_AUDIO;

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

        AudCodecCfg.Spec.AACCfg.BitstreamType = AAC_BS_RAW;
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
    AmbaPrint("AudDecHdlr = 0x%x ", AudDecHdlr);

    AudioDec_Init = 1;
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
static int AmpUT_NetAudioDec_Start(char *audFmt, int sampleRate)
{
    AMP_AVDEC_TRICKPLAY_s Trick = {
            0 };
    AMP_AUDIODEC_DECODER_CFG_s Cfg;
    int Er = 0;
    char *ErrMsg = "";

    memset(&Cfg, 0, sizeof(Cfg));

    AmbaPrint("%s", __FUNCTION__);

    AmbaKAL_TaskSleep(100);

    Cfg.SrcSampleRate = sampleRate;
    if (Cfg.SrcSampleRate == 0) {
        Cfg.SrcSampleRate = 48000;
    }

    if (strcmp(audFmt, "pcm") == 0) {
        Cfg.DecType = AMBA_AUDIO_PCM;
        Cfg.Spec.PCMCfg.BitsPerSample = 16;
        Cfg.Spec.PCMCfg.DataFormat = 0;
        Cfg.Spec.PCMCfg.FrameSize = 1024;
    } else if (strcmp(audFmt, "aac") == 0) {
        Cfg.DecType = AMBA_AUDIO_AAC;
        Cfg.Spec.AACCfg.BitstreamType = AAC_BS_RAW;
    } else if (strcmp(audFmt, "opus") == 0) {
        AmbaPrint("format opus");
        Cfg.Spec.OPUSCfg.BitstreamType = OPUS_BS_RTP;
        Cfg.Spec.OPUSCfg.FrameSize = Cfg.SrcSampleRate * 20 / 1000;
        Cfg.DecType = AMBA_AUDIO_OPUS;
    } else {
        //AmbaPrint("%s: Format (%s) not supported", __FUNCTION__, argv[3]);
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

    if(AmpUT_enVideo != 0x1){//don't call start twice
        Trick.Speed = 0x100;
        AmbaPrint("AmpDec_Start");
        Er = AmpDec_Start(DecPipeHdlr, &Trick, NULL);
        if (Er != 0) {
            ErrMsg = "AmpDec_Start failed";
            goto ReturnError;
        }
    }

    AmbaPrint("%s OK", __FUNCTION__);
    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}



/**
 * Create display handlers and windows on Vout.
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_NetVideoDec_ConfigDisplay(void)
{
    AMP_DISP_WINDOW_CFG_s DispWindow;
    int Er = NG;

    if (AmpUT_Display_Init() == NG) {
        return Er;
    }

    memset(&DispWindow, 0, sizeof(AMP_DISP_WINDOW_CFG_s));

    DispWindow.CropArea.Width = 0;
    DispWindow.CropArea.Height = 0;
    DispWindow.CropArea.X = 0;
    DispWindow.CropArea.Y = 0;
    DispWindow.TargetAreaOnPlane.Width = 960;
    DispWindow.TargetAreaOnPlane.Height = 360;
    DispWindow.TargetAreaOnPlane.X = 0;
    DispWindow.TargetAreaOnPlane.Y = 60;
    DispWindow.Source = AMP_DISP_DEC;
    if (AmpUT_Display_Window_Create(0, &DispWindow) == NG) {
        return Er;
    }

#ifdef  DEC_1080
    DispWindow.TargetAreaOnPlane.Width = 1920;
    DispWindow.TargetAreaOnPlane.Height = 1080;
#else
    DispWindow.TargetAreaOnPlane.Width = 3840;
    DispWindow.TargetAreaOnPlane.Height = 2160;
#endif
    if (AmpUT_Display_Window_Create(1, &DispWindow) == NG) {
        return Er;
    }
    return 0;
}

static int AmpUT_NetVideoDec_CodecCB(void* hdlr, UINT32 event, void* info)
{
    return 0;
}

/**
 * Initialize video decode.
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_NetVideoDec_Init(void)
{
    AMP_VIDEODEC_CFG_s CodecCfg;
    AMP_VIDEODEC_INIT_CFG_s CodecInitCfg;
    int Er = 0;
    char *ErrMsg = "";
    AMP_UT_DUMMY_DEMUX_HDLR_s* DmxHdlr = NULL;

    AmbaPrint("%s", __FUNCTION__);

    if (VideoDec_Init) {
        AmbaPrint("%s: Already initialized", __FUNCTION__);
        goto ReturnSuccess;
    }

    {
        extern int AmbaLoadDSPuCode(void);
        AmbaLoadDSPuCode(); /* Load uCodes from NAND */
    }


    // Get from Isaac for VideoHISO's implementation
    {
        AMBA_OPMODE_CLK_PARAM_s ClkParam = {0};
        ClkParam.CoreClkFreq = 350000000;
        ClkParam.IdspClkFreq = 432000000/*400000000*/;
        ClkParam.CortexClkFreq = 504000000;
        Er = AmbaPLL_SetOpMode(&ClkParam);
        if (Er != 0) {
            ErrMsg = "AmbaPLL_SetOpMode failed";
            goto ReturnError;
        }
    }

    /** Initialize codec module */
    if (AvcCodecBufferOri == NULL) {
        // Get the default codec module settings
        AmpVideoDec_GetInitDefaultCfg(&CodecInitCfg);
        // Customize the module settings
            // You can add some codes here ...
        // Allocate memory for codec module
        if (AmpUtil_GetAlignedPool(&G_MMPL, (void**) &(CodecInitCfg.Buf), &AvcCodecBufferOri, CodecInitCfg.BufSize, 1 << 5) != AMP_OK) {
            AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
            ErrMsg = "AmpUtil_GetAlignedPool failed";
            goto ReturnError;
        }
        // Configure the initial settings
        Er = AmpVideoDec_Init(&CodecInitCfg);
        if (Er != 0) {
            ErrMsg = "AmpVideoDec_Init failed";
            goto ReturnError;
        }
    }

    /** Create codec handler */
    // Get the default codec handler settings
    AmpVideoDec_GetDefaultCfg(&CodecCfg);
    if (AvcRawBufferOri == NULL) {
        // Allocate memory for codec raw buffer
        if (AmpUtil_GetAlignedPool(&G_MMPL, &AvcRawBuffer, &AvcRawBufferOri, VIDEODEC_RAW_SIZE, 1 << 6) != AMP_OK) {
            AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
            ErrMsg = "AmpUtil_GetAlignedPool failed";
            goto ReturnError;
        }
    }
    CodecCfg.RawBuffer = (char*) AvcRawBuffer;
    AmbaPrint("%s:%u RawBuffer:%x -> %x", __FUNCTION__, __LINE__, AvcRawBufferOri, CodecCfg.RawBuffer);
    CodecCfg.RawBufferSize = VIDEODEC_RAW_SIZE;
    if (AvcDescBufferOri == NULL) {
        if (AmpUtil_GetAlignedPool(&G_MMPL, &AvcDescBuffer, &AvcDescBufferOri, VIDEODEC_RAW_DESC_NUM * sizeof(AMP_BITS_DESC_s), 1 << 5) != AMP_OK) {
            AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
            ErrMsg = "AmpUtil_GetAlignedPool failed";
            goto ReturnError;
        }
    }
    CodecCfg.DescBuffer = (char*) AvcDescBuffer;
    CodecCfg.NumDescBuffer = VIDEODEC_RAW_DESC_NUM;
    // Customize the handler settings
    CodecCfg.CbCodecEvent = AmpUT_NetVideoDec_CodecCB;
    // TDB feature config
    CodecCfg.Feature.MaxVoutWidth = 3840;
    CodecCfg.Feature.MaxVoutHeight = 2160;
    // Create a codec handler, and configure the initial settings
    if (AmpVideoDec_Create(&CodecCfg, &AvcDecHdlr) != AMP_OK) {
        AmbaPrint("%s:%u Cannot create video codec.", __FUNCTION__, __LINE__);
        ErrMsg = "AmpVideoDec_Create failed";
        goto ReturnError;
    }
    AmbaPrint("AvcDecHdlr = 0x%x ", AvcDecHdlr);

    AmbaKAL_TaskSleep(17);

    strcpy(netdec_info.fn, "c:\\out_0000");
    netdec_info.DmxHdlr = DmxHdlr;
    netdec_info.VideoWidth = 1280;
    netdec_info.VideoHeight = 720;
    netdec_info.StartTime = 0;
    netdec_info.Speed = 0x100;
    netdec_info.PreFeedNum = 32;
    netdec_info.Direction = 0;
    netdec_info.FadeEnable = 0;

    VideoDec_Init = 1;

ReturnSuccess:
    AmbaPrint("%s OK", __FUNCTION__);
    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}


/**
 * Set display channel configuration.
 *
 * @param [in] argv         Command line arguments
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_NetVideoDec_ConfigDisplayCh(int ch)
{
    int Er = 0;
    char *ErrMsg = "";

    AmbaPrint("%s", __FUNCTION__);

    if (ch == 1) {
        // Start up TV handler
        Er = AmpUT_Display_Start(1);
        if (Er != 0) {
            ErrMsg = "AmpUT_Display_Start failed";
            goto ReturnError;
        }
        Er = AmpUT_Display_Act_Window(1);
        if (Er != 0) {
            ErrMsg = "AmpUT_Display_Act_Window failed";
            goto ReturnError;
        }
    } else {
        // Start up LCD handler
        Er = AmpUT_Display_Start(0);
        if (Er != 0) {
            ErrMsg = "AmpUT_Display_Start failed";
            goto ReturnError;
        }
        Er = AmpUT_Display_Act_Window(0);
        if (Er != 0) {
            ErrMsg = "AmpUT_Display_Act_Window failed";
            goto ReturnError;
        }
    }

    AmbaPrint("%s OK", __FUNCTION__);
    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}

static int AmpUT_NetVideoDec_EnableDisp(AMP_VIDEODEC_DISPLAY_s *Display)
{
    AMP_DISP_WINDOW_CFG_s WinCfg;
    AMP_DISP_INFO_s DispInfo;
    int Er = 0;
    char *ErrMsg = "";

    /** Re-config window */
    // Get display info
    Er = AmpUT_Display_GetInfo(0, &DispInfo);
    if (Er != 0) {
        ErrMsg = "AmpUT_Display_GetInfo failed";
        goto ReturnError;
    }
    // Get window settings
    AmpDisplay_GetDefaultWindowCfg(&WinCfg);

    {   // Center align the image
        float WidthPxlRatioOnDev, HeightPxlRatioOnDev;
        WidthPxlRatioOnDev = Display->SrcWidth / DispInfo.DeviceInfo.DevPixelAr;
        HeightPxlRatioOnDev = Display->SrcHeight;
        if (WidthPxlRatioOnDev/HeightPxlRatioOnDev >
        (float)DispInfo.DeviceInfo.VoutWidth/((float)DispInfo.DeviceInfo.VoutHeight)) {
            // black bar at top and bottom
            WinCfg.TargetAreaOnPlane.Width = DispInfo.DeviceInfo.VoutWidth;
            WinCfg.TargetAreaOnPlane.Height = (UINT32)((WinCfg.TargetAreaOnPlane.Width*HeightPxlRatioOnDev)/WidthPxlRatioOnDev);
            WinCfg.TargetAreaOnPlane.X = 0;
            WinCfg.TargetAreaOnPlane.Y = (DispInfo.DeviceInfo.VoutHeight - WinCfg.TargetAreaOnPlane.Height)>>1;
        } else {
            // black bar at left and right
            WinCfg.TargetAreaOnPlane.Height = DispInfo.DeviceInfo.VoutHeight;
            WinCfg.TargetAreaOnPlane.Width = (UINT32)((WinCfg.TargetAreaOnPlane.Height*WidthPxlRatioOnDev)/HeightPxlRatioOnDev);
            WinCfg.TargetAreaOnPlane.Y = 0;
            WinCfg.TargetAreaOnPlane.X = (DispInfo.DeviceInfo.VoutWidth - WinCfg.TargetAreaOnPlane.Width)>>1;
        }
        // LCD Align to 6
        {
            UINT32 Align = 6;
            WinCfg.TargetAreaOnPlane.X = WinCfg.TargetAreaOnPlane.X / Align * Align;
            WinCfg.TargetAreaOnPlane.Y = WinCfg.TargetAreaOnPlane.Y / Align * Align;
            WinCfg.TargetAreaOnPlane.Width = (WinCfg.TargetAreaOnPlane.Width + Align - 1) / Align * Align;
            WinCfg.TargetAreaOnPlane.Height = (WinCfg.TargetAreaOnPlane.Height + Align - 1) / Align * Align;
        }
    }
    // Configure & update window settings
    WinCfg.Source = AMP_DISP_DEC;
    Er = AmpUT_Display_Window_Create(0, &WinCfg);
    if (Er != 0) {
        ErrMsg = "AmpUT_Display_SetWindowCfg failed";
        goto ReturnError;
    }
    // active and Update window
    Er = AmpUT_Display_Act_Window(0);
    if (Er != 0) {
        ErrMsg = "AmpUT_Display_Update failed";
        goto ReturnError;
    }

    // Get display info
    Er = AmpUT_Display_GetInfo(1, &DispInfo);
    if (Er != 0) {
        ErrMsg = "AmpUT_Display_GetInfo failed";
        goto ReturnError;
    }
    // Get window settings
    AmpDisplay_GetDefaultWindowCfg(&WinCfg);

    {   // Center align the image
        float WidthPxlRatioOnDev, HeightPxlRatioOnDev;
        WidthPxlRatioOnDev = Display->SrcWidth / DispInfo.DeviceInfo.DevPixelAr;
        HeightPxlRatioOnDev = Display->SrcHeight;
        if (WidthPxlRatioOnDev/HeightPxlRatioOnDev >
        (float)DispInfo.DeviceInfo.VoutWidth/((float)DispInfo.DeviceInfo.VoutHeight)) {
            // black bar at top and bottom
            WinCfg.TargetAreaOnPlane.Width = DispInfo.DeviceInfo.VoutWidth;
            WinCfg.TargetAreaOnPlane.Height = (UINT32)((WinCfg.TargetAreaOnPlane.Width*HeightPxlRatioOnDev)/WidthPxlRatioOnDev);
            WinCfg.TargetAreaOnPlane.X = 0;
            WinCfg.TargetAreaOnPlane.Y = (DispInfo.DeviceInfo.VoutHeight - WinCfg.TargetAreaOnPlane.Height)>>1;
        } else {
            // black bar at left and right
            WinCfg.TargetAreaOnPlane.Height = DispInfo.DeviceInfo.VoutHeight;
            WinCfg.TargetAreaOnPlane.Width = (UINT32)((WinCfg.TargetAreaOnPlane.Height*WidthPxlRatioOnDev)/HeightPxlRatioOnDev);
            WinCfg.TargetAreaOnPlane.Y = 0;
            WinCfg.TargetAreaOnPlane.X = (DispInfo.DeviceInfo.VoutWidth - WinCfg.TargetAreaOnPlane.Width)>>1;
        }
        // TV Align to 2
        {
            UINT32 Align = 2;
            WinCfg.TargetAreaOnPlane.X = WinCfg.TargetAreaOnPlane.X / Align * Align;
            WinCfg.TargetAreaOnPlane.Y = WinCfg.TargetAreaOnPlane.Y / Align * Align;
            WinCfg.TargetAreaOnPlane.Width = (WinCfg.TargetAreaOnPlane.Width + Align - 1) / Align * Align;
            WinCfg.TargetAreaOnPlane.Height = (WinCfg.TargetAreaOnPlane.Height + Align - 1) / Align * Align;
        }
    }
    // Configure window settings
    WinCfg.Source = AMP_DISP_DEC;
    Er = AmpUT_Display_Window_Create(1, &WinCfg);
    if (Er != 0) {
        ErrMsg = "AmpUT_Display_SetWindowCfg failed";
        goto ReturnError;
    }
    // active and Update window
    Er = AmpUT_Display_Act_Window(1);
    if (Er != 0) {
        ErrMsg = "AmpUT_Display_Update failed";
        goto ReturnError;
    }

    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}

/**
 * Start decoding video.
 *
 * @param [in] argv         Command line arguments
 *
 * @return 0 - OK, others - Error
 */
static int AmpUT_NetVideoDec_Start(void)
{
    AMP_VIDEODEC_DISPLAY_s Display = { 0 };
    AMP_AVDEC_TRICKPLAY_s Trick = { 0 };
    char AvcFn[80];         // Path of .h264 file in ASCII format
    char IdxFn[80];         // Path of .nhnt file in ASCII format
    char UdtFn[80];         // Path of .udta file in ASCII format
    int Er = 0;
    char *ErrMsg = "";

    AmbaPrint("%s", __FUNCTION__);

    // Set direction before feeding frames
    if (netdec_info.Direction == 0) {
        PlayDirection = AMP_VIDEO_PLAY_FW;
    } else {
        PlayDirection = AMP_VIDEO_PLAY_BW;
    }

    AmbaPrint("settings:");
    AmbaPrint("fn=%s",netdec_info.fn);
    AmbaPrint("width=%u, height=%u, stime=%u, speed=%u, dir=%u",
        netdec_info.VideoWidth,netdec_info.VideoHeight,netdec_info.StartTime,
        netdec_info.Speed,netdec_info.Direction);

    /** Get target file name (.h264, .nhnt) */
    AmbaPrint("%s: target file - %s", __FUNCTION__, netdec_info.fn);
    sprintf(AvcFn, "%s.h264", netdec_info.fn);
    sprintf(IdxFn, "%s.nhnt", netdec_info.fn);
    sprintf(UdtFn, "%s.udta", netdec_info.fn);
    // Convert file name to Unicode

    /** Configure Display settings */
    Display.SrcWidth = netdec_info.VideoWidth;
    Display.SrcHeight = netdec_info.VideoHeight;
    Display.AOI.X = 0;
    Display.AOI.Y = 0;
    Display.AOI.Width = Display.SrcWidth;
    Display.AOI.Height = Display.SrcHeight;
    /** enable display **/
    Er = AmpUT_NetVideoDec_EnableDisp(&Display);
    if (Er != 0) {
        ErrMsg = "AmpUT_NetVideoDec_EnableDisp failed";
        goto ReturnError;
    }

    /** Configure play settings */
    Trick.Speed = netdec_info.Speed;
    Trick.TimeOffsetOfFirstFrame = netdec_info.StartTime;
    Trick.Direction = PlayDirection;

    /** Play video */
    AmbaPrint("%s: AmpDec_Start start", __FUNCTION__);
    Er = AmpDec_Start(DecPipeHdlr, &Trick, &Display);
    if (Er != 0) {
        ErrMsg = "AmpDec_Start failed";
        goto ReturnError;
    }
    AmbaPrint("%s: AmpDec_Start end", __FUNCTION__);

    AmbaPrint("%s OK", __FUNCTION__);
    return 0;

ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}

/**
 * video encoder event callback function.
 */
static int DummyVideoEnc_Net_EventCB(void *pHdlr, UINT32 nEvent, void *pInfo)
{
    if (nEvent == AMP_FIFO_EVENT_DATA_CONSUMED) {
        if (RingBuf_DeleteFrame(&g_Encoder.RingBuf) == -1)
             AmbaPrint("RingBuf_DeleteFrame Fail");
    }

    return 0;
}

/**
 * Dummy encoder reset function.
 */
static int AmpUT_NetFifo_rec_Reset(void)
{
    int rval = -1;

    if(g_EncFifoHdlr != NULL) {
        rval = DummyVideoEnc_Net_Reset(&g_Encoder, g_EncFifoHdlr);
    }

    return rval;
}

/**
 * Dummy encoder start function.
 */
static int AmpUT_NetFifo_rec_start(void)
{
    int rval = -1;

    rval = DummyVideoEnc_Net_QueryState(&g_Encoder);
    if(rval < 0) {
        AmbaPrint("%s: Fail to do DummyVideoEnc_Net_QueryState. rval=%d",__FUNCTION__,rval);
        return rval;
    }

    if(rval == ENCODER_STATE_RUNNING) {
        AmbaPrint("Already Encoding...");
        return 0;
    }

    if(g_EncStatus == 1) { //previous stop due to EOS
        AmbaPrint("previous stop due to EOS...");
        rval = AmpNetFifo_SendNotify(AMP_NETFIFO_NOTIFY_STOPENC, 0, 0);
        if(rval != 0){
            AmbaPrint("%s: Fail to AmpNetFifo_SendNotify().%d", __FUNCTION__,rval);
        }
    }

    //Reset DummyVideoEnc_Net
    rval = AmpUT_NetFifo_rec_Reset();
    if(rval < 0){
        return rval;
    }

    rval = DummyVideoEnc_Net_Start(&g_Encoder);
    if(rval == 0){
        rval = AmpNetFifo_SendNotify(AMP_NETFIFO_NOTIFY_STARTENC, 0, 0);
        if(rval != 0){
            AmbaPrint("%s: Fail to AmpNetFifo_SendNotify().%d", __FUNCTION__,rval);
        }
        g_EncStatus = 1; //encoding
    }

    return rval;
}

/**
 * Dummy encoder stop function.
 */
static int AmpUT_NetFifo_rec_stop(void)
{
    int rval = -1;

    if (g_EncStatus == 0) {
        AmbaPrint("Already Stopped...");
        return 0;
    }

    rval = DummyVideoEnc_Net_Stop(&g_Encoder);
    if (rval == 0) {
        rval = AmpNetFifo_SendNotify(AMP_NETFIFO_NOTIFY_STOPENC, 0, 0);
        if(rval != 0){
            AmbaPrint("%s: Fail to AmpNetFifo_SendNotify().%d", __FUNCTION__,rval);
        }
        g_EncStatus = 0; //idle
    }

    return rval;
}

/**
 * callback function to handle NetFifo event.
 */
static int AmpUT_NetDec_EventCB(void *hdlr, UINT32 event, void* info)
{
    int rval = 0;

    AmbaPrint("%s: Got NetFifo Event 0x%08x.", __FUNCTION__,event);

    return rval;
}

/**
 * Return Stream ID list.
 */
static int AmpUT_NetDec_GetStreamIDList(AMP_NETFIFO_MEDIA_STREAMID_LIST_s *slist)
{
    int rval = 0;

    if (slist == NULL) {
        AmbaPrint("%s: invalid StreamID List output buffer.(NULL)", __FUNCTION__);
        return -1;
    }

    if (g_EncStatus == 1) { //encoding
        slist->Amount = 1; //only 1 stream
        slist->StreamID_List[0] = AMPUT_NETFIFO_MEDIA_STREAMID;
    } else { //idle
        slist->Amount = 1;
        slist->StreamID_List[0] = AMPUT_NETFIFO_MEDIA_STREAMID;
    }

    return rval;
}

/**
 * Return Media Ifno for dedicated Stream ID.
 */
static int AmpUT_NetDec_GetMediaInfo(int StreamID, AMP_NETFIFO_MOVIE_INFO_CFG_s *media_info)
{
    int rval = -1;

    AmbaPrint("%s: request MediaInfo for Stream 0x%08x.", __FUNCTION__,StreamID);

    //Only CH0_Primary stream is supported for this test
    if(StreamID != AMPUT_NETFIFO_MEDIA_STREAMID
        && StreamID != AmpUT_NetVideoDec_MEDIA_STREAMID) {
        AmbaPrint("%s: Only first StreamID is supported", __FUNCTION__);
        return -1;
    }

    // prepare media info output buffer
    if(media_info == NULL){
        AmbaPrint("%s: invalid MediaInfo output buffer.(NULL)", __FUNCTION__);
        return -1;
    }
    rval = AmpNetFifo_GetDefaultMovieInfoCfg(media_info);

    // fill media info
    if(rval == 0 && StreamID == AMPUT_NETFIFO_MEDIA_STREAMID) {
#if 1
        media_info->nTrack = 1;
        media_info->Track[0].hCodec = AMPUT_NETFIFO_CODEC_HDLR;
        media_info->Track[0].nTrackType = AMP_NETFIFO_MEDIA_TRACK_TYPE_VIDEO;
        media_info->Track[0].pBufferBase = (UINT8 *)g_EncBuffer;
        media_info->Track[0].pBufferLimit = (UINT8 *)g_EncBuffer + AMPUT_NETFIFO_ENC_RAW_SIZE;
        media_info->Track[0].Info.Video.bDefault = TRUE;
        media_info->Track[0].nMediaId = AMP_FORMAT_MID_AVC;
        media_info->Track[0].nTimeScale = g_Udta.nTimeScale;//atoi(Rate);
        media_info->Track[0].nTimePerFrame = g_Udta.nTickPerPicture;//atoi(Scale);
        media_info->Track[0].Info.Video.nMode = AMP_VIDEO_MODE_P;
        media_info->Track[0].Info.Video.nM = g_Udta.nM;//atoi(M);
        media_info->Track[0].Info.Video.nN = g_Udta.nN;//atoi(N);
        media_info->Track[0].Info.Video.nGOPSize = (UINT32)media_info->Track[0].Info.Video.nN * g_Udta.nIdrInterval;//atoi(idrItvl);
        media_info->Track[0].Info.Video.nCodecTimeScale = g_Udta.nTimeScale;    // TODO: should remove nCodecTimeScale, but the sample clip uses 90K
        media_info->Track[0].Info.Video.nWidth = g_Udta.nVideoWidth;//atoi(Width);
        media_info->Track[0].Info.Video.nHeight = g_Udta.nVideoHeight;//atoi(Height);
        media_info->Track[0].Info.Video.nTrickRecDen = media_info->Track[0].Info.Video.nTrickRecNum = 1;
#else
        media_info->nTrack = 1;
        media_info->Track[0].hCodec = AMPUT_NETFIFO_CODEC_HDLR;
        media_info->Track[0].nTrackType = AMP_NETFIFO_MEDIA_TRACK_TYPE_AUDIO;
        media_info->Track[0].pBufferBase = (UINT8 *)g_EncBuffer;
        media_info->Track[0].pBufferLimit = (UINT8 *)g_EncBuffer + AMPUT_NETFIFO_ENC_RAW_SIZE;
        media_info->Track[0].Info.Audio.nSampleRate = 48000;
        media_info->Track[0].Info.Audio.nChannels = 2;
        media_info->Track[0].nMediaId = AMP_FORMAT_MID_AAC;
#endif
    }else if(rval == 0 && StreamID == AmpUT_NETDEC_MEDIA_STREAMID){
        int i= 0;
        media_info->nTrack = (UINT8) AmpUT_nTrack;
        if (AmpUT_enVideo == 0x01) {
            AmbaPrint("Video enable");
            media_info->Track[i].hCodec = (void*) AvcDecHdlr;
            media_info->Track[i].nTrackType = AMP_NETFIFO_MEDIA_TRACK_TYPE_VIDEO;
            media_info->Track[i].pBufferBase = (UINT8 *)AvcRawBuffer;
            media_info->Track[i].pBufferLimit = (UINT8*) ((UINT32) AvcRawBuffer + VIDEODEC_RAW_SIZE - 1);
            i++;
        }
        if (AmpUT_enAudio == 0x01 && i<media_info->nTrack) {
            AmbaPrint("Audio enable");
            media_info->Track[i].hCodec = (void*) AudDecHdlr;
            media_info->Track[i].nTrackType = AMP_NETFIFO_MEDIA_TRACK_TYPE_AUDIO;
            media_info->Track[i].pBufferBase = (UINT8 *)AudRawBuf;
            media_info->Track[i].pBufferLimit = (UINT8*) ((UINT32) AudRawBuf + AUDIODEC_RAW_SIZE - 1);
        }
    }

    return rval;
}

static int AmpUT_NetDec_exit(void)
{
    int Er;
    char *ErrMsg = "";

    if(AmpUT_NetDecInited == 0){
        AmbaPrint("No need to exit");
        return 0;
    }

    // Deinit decoder manager
    if (DecPipeHdlr != NULL) {
        if (AmpDec_Stop(DecPipeHdlr) != AMP_OK) {
            AmbaPrint("%s:%u Failed to stop the video decoder manager.", __FUNCTION__, __LINE__);
        }
        if (AmpDec_Remove(DecPipeHdlr) != AMP_OK) {
            AmbaPrint("%s:%u Failed to remove the video decoder manager.", __FUNCTION__, __LINE__);
        }
        if (AmpDec_Delete(DecPipeHdlr) != AMP_OK) {
            AmbaPrint("%s:%u Failed to delete the video decoder manager.", __FUNCTION__, __LINE__);
        }
        DecPipeHdlr = NULL;
    }

    // Deinit video decoder
    if (AvcDecHdlr != NULL) {
        if (AmpVideoDec_Delete(AvcDecHdlr) != AMP_OK) {
            AmbaPrint("%s:%u Failed to deinit the video decoder.", __FUNCTION__, __LINE__);
        }
        AvcDecHdlr = NULL;
    }

    // Delete LCD Window
    if (AmpUT_Display_Window_Delete(0) != AMP_OK) {
        AmbaPrint("%s:%u Failed to delete LCD window handler.", __FUNCTION__, __LINE__);
        ErrMsg = "AmpUT_Display_Window_Delete failed";
        goto ReturnError;
    }

    // Delete TV Window
    if (AmpUT_Display_Window_Delete(1) != AMP_OK) {
        AmbaPrint("%s:%u Failed to delete TV window handler.", __FUNCTION__, __LINE__);
        ErrMsg = "AmpUT_Display_Window_Delete failed";
        goto ReturnError;
    }

    // Release video descriptor buffer
    if (AvcDescBufferOri != NULL) {
        if (AmbaKAL_BytePoolFree(AvcDescBufferOri) != AMP_OK) {
            AmbaPrint("%s:%u Failed to release the descriptor buffer.", __FUNCTION__, __LINE__);
        }
        AvcDescBufferOri = NULL;
    }

    // Release video raw buffer
    if (AvcRawBufferOri != NULL) {
        if (AmbaKAL_BytePoolFree(AvcRawBufferOri) != AMP_OK) {
            AmbaPrint("%s:%u Failed to release the raw buffer.", __FUNCTION__, __LINE__);
        }
        AvcRawBuffer = NULL;
        AvcRawBufferOri = NULL;
    }
    AmbaKAL_TaskTerminate(&Task);
    AmbaKAL_TaskDelete(&Task);
    AmbaKAL_MsgQueueDelete(&MsgQueue);

    VideoDec_Init = 0;

    // no release flow for Audio, we only remove fifo and wait next Decode start

    AmpUT_NetDecInited = 0;
    return 0;
ReturnError:
    AmbaPrintColor(RED, "%s NG %s", __FUNCTION__, ErrMsg);
    return -1;
}


/**
 * callback function to handle get MediaInfo request.
 */
static int AmpUT_NetDec_MediaInfoCB(void *hdlr, UINT32 event, void* info)
{
    int rval = 0;
    int sid;
    unsigned int cmd;

    cmd = AMP_NETFIFO_GET_MEDIA_CMD(event);
    switch(cmd) {
    case AMP_NETFIFO_MEDIA_CMD_GET_STREAM_LIST:
        rval = AmpUT_NetDec_GetStreamIDList((AMP_NETFIFO_MEDIA_STREAMID_LIST_s *)info);
        break;
    case AMP_NETFIFO_MEDIA_CMD_GET_INFO:
        sid = AMP_NETFIFO_GET_MEDIA_PARAM(event);
        rval = AmpUT_NetDec_GetMediaInfo(sid,(AMP_NETFIFO_MOVIE_INFO_CFG_s *)info);
        break;
    default:
        AmbaPrint("%s: unsupported cmd 0x04%x, event:0x%08x", __FUNCTION__,
                cmd, event);
        break;
    }

    return rval;
}

/**
 * callback function to handle playback request.
 */
static int AmpUT_NetDec_PlaybackCB(void *hdlr, UINT32 event, void* info)
{
    AmbaPrint("%s: Got NetFifo Playback request 0x%08x.", __FUNCTION__,event);
    return 0;
}
static int AmpUT_NetDec_StreamInMsgCB(void *hdlr, UINT32 event, void* param)
{
    int rval = 0;
    //AMP_NETFIFO_STREAMIN_MSG_INFO_s* msg = (AMP_NETFIFO_STREAMIN_MSG_INFO_s*)param;

    switch(event){
    case AMP_NETFIFO_STREAMIN_START:
        AmbaPrint("%s: Got AMP_NETFIFO_STREAMIN_START", __FUNCTION__);
        AmbaKAL_MsgQueueSend(&MsgQueue, &event, AMBA_KAL_NO_WAIT);
        break;
    case AMP_NETFIFO_STREAMIN_STOP:
        AmbaPrint("%s: Got AMP_NETFIFO_STREAMIN_STOP", __FUNCTION__);
        AmpUT_NetDec_exit();
        //release flow in RTOS side
        break;
    case AMP_NETFIFO_STREAMIN_ERROR:
        AmbaPrint("%s: Got AMP_NETFIFO_STREAMIN_ERROR", __FUNCTION__);
        AmpUT_NetDec_exit();
        //release flow in RTOS side
        break;
    default:
        AmbaPrint("%s: Unsuoported event 0x%08x",__FUNCTION__,event);
        rval = -1;
        break;
    }

    return rval;
}
static int AmpUT_Decoder_create(int dual_vout)
{
    int i=0, Er = 0;
    AMP_DEC_PIPE_CFG_s PipeCfg;

    AmpDec_GetDefaultCfg(&PipeCfg);
    // Customize the manager settings
    //PipeCfg.cbEvent

    if (AmpUT_enVideo == 0x01) {
        PipeCfg.Decoder[i] = AvcDecHdlr;
        AmbaPrint("set AvcDec to decoder[%d]", i);
        i++;
    }
    if (AmpUT_enAudio == 0x01) {
        PipeCfg.Decoder[i] = AudDecHdlr;
        AmbaPrint("set AudDec to decoder[%d]", i);
    }

    PipeCfg.NumDecoder = AmpUT_nTrack;
    AmbaPrint("total track = %d\n", AmpUT_nTrack);
    PipeCfg.Type = AMP_DEC_VID_PIPE;
    // Create a decoder manager, and insert the codec handler into the manager
    Er = AmpDec_Create(&PipeCfg, &DecPipeHdlr);
    if (Er != 0) {
        AmbaPrint("AmpDec_Create failed");
        return Er;
    }

    if (AmpUT_enVideo == 0x01) {
        Er = AmpUT_NetVideoDec_ConfigDisplay();
        if (Er != 0) {
            AmbaPrint("AmpUT_NetVideoDec_ConfigDisplay failed");
            return Er;
        }
    }
        /** Activate decoder manager */
        // Activate the decoder manager and all the codec handlers in the manager
    Er = AmpDec_Add(DecPipeHdlr);
    if (Er != 0) {
        AmbaPrint("AmpDec_Add failed");
    }
    if (AmpUT_enVideo == 0x01) {
        Er = AmpUT_NetVideoDec_ConfigDisplayCh(0);
        if (dual_vout == 1) {
            AmbaPrint("Init HDMI output\n");
            Er = AmpUT_NetVideoDec_ConfigDisplayCh(1);
        }
    }
    return Er;
}

static int AmpUT_Dummy_init(void)
{
    AMP_CFS_STAT stat = {0};
    AMP_CFS_FILE_s *hUdtaFile = NULL;
    AMP_CFS_FILE_PARAM_s cfsParam = {0};
    UINT32 nBufferSize = 0;
    void *encRawBuf = NULL;
    int rval;

    //create encode buffer
    nBufferSize = DummyVideoEnc_Net_GetRequiredBufSize(AMPUT_NETFIFO_ENC_RAW_SIZE, AMPUT_NETFIFO_MAX_FIFO_NUM);
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **)&g_EncBuffer, &encRawBuf, nBufferSize, AMBA_CACHE_LINE_SIZE) != OK) {
        AmbaPrint("%s:%u", __FUNCTION__, __LINE__);
    }
    DummyVideoEnc_Net_Init(&g_Encoder, BS_FN, HDR_FN, g_EncBuffer, nBufferSize, AMPUT_NETFIFO_MAX_FIFO_NUM);

    // Read UDTA file
    rval = AmpCFS_Stat(UDTA_FN, &stat);
    if (rval == AMP_OK) {
        AmpCFS_GetFileParam(&cfsParam);
        cfsParam.Mode = AMP_CFS_FILE_MODE_READ_ONLY;
        strcpy(cfsParam.Filename, UDTA_FN);
        hUdtaFile = AmpCFS_fopen(&cfsParam);
        if (hUdtaFile == NULL) {
            AmbaPrint("Udta Open fail");
            rval = -1;
            goto done;
        } else {
            AmpCFS_fread(&g_Udta, 1, sizeof(g_Udta), hUdtaFile);
            AmpCFS_fclose(hUdtaFile);
        }
    } else {
        AmbaPrint("%s: fail to read UDTA file!(%d)", __FUNCTION__,rval);
        rval = -1;
        goto done;
    }

    //create Physical FIFO for encoder
    do {
        AMP_FIFO_CFG_s fifoDefCfg;

        // create codec fifo
        AmpFifo_GetDefaultCfg(&fifoDefCfg);
        fifoDefCfg.hCodec = AMPUT_NETFIFO_CODEC_HDLR;
        fifoDefCfg.IsVirtual = 0;
        fifoDefCfg.NumEntries = AMPUT_NETFIFO_MAX_FIFO_NUM;
        fifoDefCfg.cbEvent = DummyVideoEnc_Net_EventCB;
        AmpFifo_Create(&fifoDefCfg, &g_EncFifoHdlr);

        // hook default callback
        AmpFifo_GetDefaultCfg(&fifoDefCfg);
        fifoDefCfg.hCodec = AMPUT_NETFIFO_CODEC_HDLR;
        fifoDefCfg.IsVirtual = 1;
        fifoDefCfg.NumEntries = AMPUT_NETFIFO_MAX_FIFO_NUM;
        fifoDefCfg.cbEvent = AmpNetFifo_DefaultCB;
        AmpFifo_Create(&fifoDefCfg, &g_DefFifoHdlr);
    } while (0);

done:
    if (rval < 0) {
        if (AmbaKAL_BytePoolFree(encRawBuf) != AMP_OK) {
            AmbaPrint("%s:%u Failed to release the encRawBuf.", __FUNCTION__, __LINE__);
        }
    }
    return rval;
}

void PlaycommandTask(UINT32 info)
{
    int msg;
    int rval;
    while (1) {
        rval = AmbaKAL_MsgQueueReceive(&MsgQueue, (void *)&msg, AMBA_KAL_WAIT_FOREVER);
        if (rval != 0) {
            return ;
        }
        // check open status before playing
        switch (msg) {
        case AMP_NETFIFO_STREAMIN_START:
            AmbaPrint("%s: playStart = %d", __FUNCTION__, playStart);
            if(AmpUT_enAudio == 1 && playStart == 0){
                AmpUT_NetAudioDec_Start("aac", 48000);
            }
            if(AmpUT_enVideo == 1 && playStart == 0){
                AmpUT_NetVideoDec_Start();
            }
            playStart = 1;
            break;
        default:
            AmbaPrintColor(YELLOW, "<%s> L%d unknown msg id %d ",__FUNCTION__,__LINE__,msg);
            break;
        }
    }
}

/*
 * Unitest initiate function.
 */
static int AmpUT_NetDec_init(int en_video, int en_audio, int dual_vout)
{

    int rval = 0;
    if(AmpUT_NetDecInited){
        AmbaPrint("Already inited!!");
        return 0;
    }

    AmpUT_nTrack = 0;
    playStart = 0;

    if(en_video == 1){
        rval = AmpUT_NetVideoDec_Init();
        if( rval < 0) goto done;
        AmpUT_enVideo = 0x01;
        AmpUT_nTrack ++;
    }
    if(en_audio == 1){
        rval = AmpUT_NetAudioDec_Init();
        if( rval < 0) goto done;
        AmpUT_enAudio = 0x01;
        AmpUT_nTrack ++;
    }
    AmpUT_Decoder_create(dual_vout);
    //init AmpNetFifo
    do {
        AMP_NETFIFO_INIT_CFG_s init_cfg;

        AmpNetFifo_GetInitDefaultCfg(&init_cfg);
        init_cfg.cbEvent = AmpUT_NetDec_EventCB;
        init_cfg.cbMediaInfo = AmpUT_NetDec_MediaInfoCB;
        init_cfg.cbPlayback = AmpUT_NetDec_PlaybackCB;
        init_cfg.cbStreamInMsg = AmpUT_NetDec_StreamInMsgCB;
        rval = AmpNetFifo_init(&init_cfg);
    } while (0);

    /* Create net fifo playback message queue */
    rval = AmbaKAL_MsgQueueCreate(&MsgQueue, MsgPool, sizeof(int), UT_NETDEC_MSGQUEUE_SIZE);
    if (rval != OK) {
        AmbaPrint("[AppLib - NetFifo] <PlaybackInit> Create Queue fail. (error code: %d)",rval);
        return rval;
    }

    /* Create net fifo playback task */
    rval = AmbaKAL_TaskCreate(&Task, /* pTask */
        "NetDec_playcommander", /* pTaskName */
        102,/* Priority */
        PlaycommandTask, /* void (*EntryFunction)(UINT32) */
        0x0, /* EntryArg */
        (void *) Stack, /* pStackBase */
        UT_NETDEC_STACK_SIZE, /* StackByteSize */
        AMBA_KAL_AUTO_START); /* Do NOT Start */

done:
    if (rval < 0) {
        AmbaPrint("NetDec Init fail");
        return rval;
    } else {
        AmpUT_NetDecInited = 1;
    }

    return 0;
}
extern int Linux_resource_init(void);
extern int Linux_Playback_OP(unsigned int OP, void *param, int param_size);

static void AmpUT_NetDec_usage(char *pName)
{
    AmbaPrint("usage: %s <CMD>",pName);
    AmbaPrint("\t init [en_video] [en_audio]");
    AmbaPrint("\t init_dual [en_video] [en_audio]");
    AmbaPrint("\t rtsp_start rtsp://192.168.42.1/live");
    AmbaPrint("\t rtsp_stop");
    AmbaPrint("\t dummy");

    AmbaPrint("\t\n\n");
}

/**
 * NetFifo test function.
 */
static int AmpUT_NetFifo_Test(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    int rval = -1;

    if (strcmp(argv[1], "init") == 0 && argc == 4) { //init resource for video codec and audio codec
        rval = AmpUT_NetDec_init(atoi(argv[2]), atoi(argv[3]), 0);
    } else if (strcmp(argv[1], "init_dual") == 0 && argc == 4) {
        rval = AmpUT_NetDec_init(atoi(argv[2]), atoi(argv[3]), 1);
    } else if (strcmp(argv[1], "dummy") == 0) {      //loop-back streaming and display video
        AmpUT_Dummy_init();
        AmpUT_NetDec_init(1, 0, 0);
        AmpUT_NetFifo_rec_start();
        AmpNetUtility_luExecNoResponse("/usr/bin/AmbaRTSPServer");
        AmbaKAL_TaskSleep(1000);
        AmpNetUtility_luExecNoResponse("/tmp/SD0/DCIM/AmbaRTSPClient rtsp://127.0.0.1/live");
    } else if (strcmp(argv[1], "dummy_stop") == 0) {
        AmpUT_NetFifo_rec_stop();
        AmpNetUtility_luExecNoResponse("killall AmbaRTSPServer");
    } else if (strcmp(argv[1], "rtsp_start") == 0) {
        char cmd[128] = { 0 };
        /* fixing RX error in wifi */
        AmpNetUtility_luExecNoResponse("echo 43636363 > /sys/kernel/debug/mmc0/clock");
        snprintf(cmd, 128, "/usr/bin/AmbaRTSPClient %s &", argv[2]);
        AmpNetUtility_luExecNoResponse(cmd);
    } else if (strcmp(argv[1], "rtsp_stop") == 0) {
        AmpNetUtility_luExecNoResponse("killall AmbaRTSPClient");
        AmpNetUtility_luExecNoResponse("echo 48000000 > /sys/kernel/debug/mmc0/clock");
    } else if (strcmp(argv[1], "stop") == 0){
        AmpDec_Stop(DecPipeHdlr);
    } else if (strcmp(argv[1], "dummy_audio") == 0) {//loop-back streaming and play sound
        //TODO
    } else if (strcmp(argv[1], "exit") == 0){
        //AmpUT_NetDec_exit();
    } else { //show usage
        AmpUT_NetDec_usage(argv[0]);
    }

    return rval;
}

/**
 * NetFifo test command hooking function.
 */
int AmpUT_NetDec_TestAdd(void)
{
    // hook command
    AmbaTest_RegisterCommand("netdec", AmpUT_NetFifo_Test);

    return AMP_OK;
}

