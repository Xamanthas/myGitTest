/**
 *  @file AmpUT_Beep.c
 *  
 *  **History**
 *      |Date       |Name        |Comments       |
 *      |-----------|------------|---------------|
 *      |2013/8/14 |cyweng       |Created        |
 *
 *  @copyright 2013 Ambarella Corporation. All rights reserved.
 *                 No part of this file may be reproduced, stored in a retrieval system,
 *                 or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                 recording, or otherwise, without the prior consent of Ambarella Corporation.
 */
#include <player/Decode.h>
#include <player/Beep.h>
#include <player/AudioDec.h>
#include "AmpUnitTest.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <AmbaUtility.h>
#include "DecCFSWrapper.h"

static void *BeepCodecBuf = NULL;
static UINT8 *BeepPcmRaw = NULL;

static AMBA_MEM_CTRL_s AudCtrlCacheBuf = {
        0 };       ///< Cache buffer for audio output control
static AMBA_MEM_CTRL_s AudCtrlNonCacheBuf = {
        0 };    ///< Non-cache buffer for audio output control


#define AUDIO_OUTPUT_TASK_PRIORITY (59)
extern UINT32* CreateAudioOutputCtrl(void);

/**
 * create audio output control\n
 * output ctrl is shared by all audio codec.
 *
 * @return 0: ok
 */
UINT32* CreateAudioOutputCtrl(void)
{
    static UINT32* AudOutCtrl = NULL;

    AMBA_AUDIO_IO_CREATE_INFO_s IOInfo = {
            0 };
    UINT32 SizeOutCacheCtrl, SizeOutNCCtrl;
    AMBA_AUDIO_BUF_INFO_s CachedInfo;
    AMBA_AUDIO_BUF_INFO_s NonCachedInfo;

    if (AudOutCtrl != NULL ) {
        return AudOutCtrl;
    }

    IOInfo.I2sIndex = 0;
    IOInfo.MaxChNum = 2;
    IOInfo.MaxDmaDescNum = 16;
    IOInfo.MaxDmaSize = 256;
    IOInfo.MaxSampleFreq = 48000;
    SizeOutCacheCtrl = AmbaAudio_OutputCachedSizeQuery(&IOInfo);
    SizeOutNCCtrl = AmbaAudio_OutputNonCachedSizeQuery(&IOInfo);

    if (0) { // TBD somehow it wont work FIX IT!!!!!!!
        if (AmpUtil_GetAlignedPool(&G_MMPL, &(AudCtrlCacheBuf.pMemAlignedBase), &(AudCtrlCacheBuf.pMemBase),
                ALIGN_4(SizeOutCacheCtrl), 1 << 6) != AMP_OK) {
            AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
            // TODO: ASSERT
            return AudOutCtrl;
        }

        if (AmpUtil_GetAlignedPool(&G_NC_MMPL, &(AudCtrlNonCacheBuf.pMemAlignedBase), &(AudCtrlNonCacheBuf.pMemBase),
                ALIGN_4(SizeOutNCCtrl), 1 << 6) != AMP_OK) {
            AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
            // TODO: ASSERT
            return AudOutCtrl;
        }
        //AmbaPrint("==>> audCtrlNCacheBufOri 0x%x", audCtrlNCacheBufOri);
    } else {
        AmbaKAL_MemAllocate(&G_MMPL, &AudCtrlCacheBuf, SizeOutCacheCtrl, 1 << 5);
        AmbaKAL_MemAllocate(&G_NC_MMPL, &AudCtrlNonCacheBuf, SizeOutNCCtrl, 1 << 5);
    }

    CachedInfo.MaxSize = (SizeOutCacheCtrl);
    CachedInfo.pHead = AudCtrlCacheBuf.pMemAlignedBase;
    NonCachedInfo.MaxSize = (SizeOutNCCtrl);
    NonCachedInfo.pHead = AudCtrlNonCacheBuf.pMemAlignedBase;

    AmbaPrint("Out Ctrl Mem: 0x%x 0x%x", (UINT32) CachedInfo.pHead, (UINT32) NonCachedInfo.pHead);
    AudOutCtrl = AmbaAudio_OutputCreate(&IOInfo, &CachedInfo, &NonCachedInfo);

    return AudOutCtrl;
}


/**
 * init beep unit test
 */
void AmpUT_Beep_Init(void)
{
    AMP_BEEP_INIT_CFG_t Cfg;
    AMBA_AUDIO_TASK_CREATE_INFO_s DecInfo;
    AMBA_ABU_CREATE_INFO_s AbuInfo;
    UINT32 DecSize, AbuSize;

    AmpAudio_OutputInit(CreateAudioOutputCtrl(), AUDIO_OUTPUT_TASK_PRIORITY);

    Cfg.SrcChannelMode = 2;
    Cfg.MaxChannelNum = 2;
    Cfg.MaxFrameSize = 4096;
    Cfg.MaxSampleRate = 48000;
    Cfg.MaxChunkNum = 16;
    Cfg.I2SIndex = 0;

    DecInfo.MaxSampleFreq = Cfg.MaxSampleRate;
    DecInfo.MaxChNum = Cfg.MaxChannelNum;
    DecInfo.MaxFrameSize = Cfg.MaxFrameSize;
    DecSize = AmbaAudio_DecSizeQuery(&DecInfo);

    AbuInfo.MaxSampleFreq = Cfg.MaxSampleRate;
    AbuInfo.MaxChNum = Cfg.MaxChannelNum;
    AbuInfo.MaxChunkNum = Cfg.MaxChunkNum;
    AbuSize = AmbaAudio_BufferSizeQuery(&AbuInfo);

    AmbaKAL_BytePoolAllocate(&G_MMPL, &BeepCodecBuf, DecSize + AbuSize + 64,
            100);
    Cfg.CodecCacheWorkBuff = BeepCodecBuf;
    Cfg.CodecCacheWorkSize = DecSize + AbuSize + 64;
    Cfg.PcmCfg.BitsPerSample = 16;
    Cfg.PcmCfg.DataFormat = 0;
    Cfg.PcmCfg.FrameSize = 1024;
    AmpBeep_Init(&Cfg);

    AmbaKAL_BytePoolAllocate(&G_MMPL, (void**)&BeepPcmRaw, 5 << 20, 100);
    AmbaPrint("beepPcmRaw 0x%x", BeepPcmRaw);
}

/**
 * send beep sound
 *
 * @param argv - argv[2]: file name
 */
void AmpUT_Beep_Beep(char **argv)
{
    AMP_BEEP_t Beep = { 0 };
    AMP_CFS_FILE_s *BeepFile;
    char Mode[3] = "rb";

    Beep.Fn = NULL;
    Beep.RawPcm = BeepPcmRaw;

    BeepFile = AmpDecCFS_fopen(argv[2], Mode);
    if (BeepFile == NULL ) {
        AmbaPrint("AmpUT_Beep_Beep: open failed");
        return;
    }
    AmpCFS_fseek(BeepFile, 0, AMBA_FS_SEEK_END);
    Beep.RawSize = AmpCFS_ftell(BeepFile);
    AmpCFS_fseek(BeepFile, 0, AMBA_FS_SEEK_START);
    AmpCFS_fread(Beep.RawPcm, Beep.RawSize, 1, BeepFile);
    AmpCFS_fclose(BeepFile);

    // TBD
    Beep.SrcChannelMode = 2;
    Beep.SrcSampleRate = 48000;


    AmpBeep_Beep(&Beep);

}

int AmpUT_BeepTest(struct _AMBA_SHELL_ENV_s_ *env,
                       int argc,
                       char **argv)
{
    AmbaPrint("AmpUT_AudioDecTest cmd: %s", argv[1]);
    if (strcmp(argv[1], "init") == 0) {
        AmpUT_Beep_Init();
    } else if (strcmp(argv[1], "b") == 0) {
        AmpUT_Beep_Beep(argv);
    } else {
        //AmpUT_AudioDec_Usage();
    }
    return 0;
}

int AmpUT_BeepTestAdd(void)
{
    AmbaPrint("Adding AmpUT_Beep");

    // hook command
    AmbaTest_RegisterCommand("be", AmpUT_BeepTest);

    return AMP_OK;
}
