/**
 * @file src/app/sample/unittest/AmpUnitTest.c
 *
 * Unit test entry
 *
 * History:
 *    2013/02/25 - [Peter Weng] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include "AmpUnitTest.h"

#include "AmbaLCD.h"
//#include "AmbaGlib.h"
#include "bsp.h"
#include "AmbaPLL.h"
#include "AmbaFIO.h"
#include "AmbaSD.h"
#include "AmbaNAND.h"
#include "AmbaROM.h"
#include "AmbaNOR.h"
#include "AmbaCardManager.h"
#include "AmbaRTC.h"
#include "AmbaGDMA.h"
#include "AmbaWDT.h"
#include "dcf/AmpDcf.h"
#include "cfs/AmpCfs.h"
#include "scheduler.h"
#include "AmbaAudio_CODEC.h"
#ifdef CONFIG_AUDIO_TITLV320AIC3256
#include "AmbaAudio_TLV320AIC3256.h"
#endif
#ifdef CONFIG_AUDIO_WM8974
#include "AmbaAudio_WM8974.h"
#endif
#include "AmbaVer.h"
#include "AmbaMonitor_SD.h"

//#include "mw_unittest_info.h"
#include <util.h>
#include <AmbaCache_Def.h>
#include "AmbaPLL.h"
#include "UT_Pref.h"
#ifdef CONFIG_SOC_A12
#include "AmbaLink.h"
#include "AmbaSysCtrl.h"
#include <net/NetEventNotifier.h>
#else
#ifdef CONFIG_ENABLE_AMBALINK
#include "AmbaLink.h"
#include "AmbaSysCtrl.h"
#endif /* CONFIG_ENABLE_AMBALINK */
#endif

#ifdef CONFIG_CC_CXX_SUPPORT
#include "AmbaCtorDtor.hpp"
#endif

#define MAX_LCD_WIDTH (960) ///< Width of LCD. For configuring seamless.
#define MAX_LCD_HEIGHT (480) ///< Height of LCD. For configuring seamless.

AMBA_KAL_BYTE_POOL_t G_MMPL = {0};
AMBA_KAL_BYTE_POOL_t G_NC_MMPL = {0};

UINT8 *DspWorkAreaResvStart;
UINT8 *DspWorkAreaResvLimit;
UINT32 DspWorkAreaResvSize;

// Display handler
AMP_DISP_HDLR_s *LCDHdlr = NULL;
AMP_DISP_HDLR_s *TVHdlr = NULL;
void* dispModuleMemPool = NULL;

// CFS event handler
AMP_CFS_EVENT_HDLR_FP CfsEventHandler = NULL;

static AMBA_KAL_TASK_t SystemInitTask = { 0 };
#define SystemInitTaskStackSize (16384)
static UINT8 SystemInitTaskStack[SystemInitTaskStackSize] = { 0 };

#ifdef CONFIG_ENABLE_AMBALINK
static AMBA_KAL_TASK_t HookDefaultUserRPCTask = { 0 };
#define HookDefaultUserRPCTaskStackSize (2048)
static UINT8 HookDefaultUserRPCTaskStack[HookDefaultUserRPCTaskStackSize] = { 0 };
#endif

#ifdef CONFIG_ENABLE_EMMC_BOOT
extern void AmbaEMMC_Init(void);
#endif
#define AUTO_FORMAT_NAND_STG (0)

UINT32 AmpUT_GetProjectDspMaxSize(void)
{
    #define DSP_MAX_USAGE (CONFIG_MW_UT_DSP_WORKBUFFER_SIZE<<20)
    return DSP_MAX_USAGE;
}

/**
 * init fifo
 */
int UT_FifoInit(void)
{
    static void* FifoBuf = NULL;
#define FIFO_BUF_SIZE   (3<<20)
#define FIFO_STACK_SIZE   (0x8000)
    AMP_FIFO_INIT_CFG_s FifoCfg;

    AmpFifo_GetInitDefaultCfg(&FifoCfg);
    AmbaPrint("AmpFifo_GetInitDefaultCfg %d", FifoCfg.NumMaxCodec);
    // alloc memory
    AmbaKAL_BytePoolAllocate(&G_MMPL, &FifoBuf, FIFO_BUF_SIZE, 100);
    FifoCfg.MemoryPoolAddr = (UINT8*) FifoBuf;
    FifoCfg.MemoryPoolSize = FIFO_BUF_SIZE;
    FifoCfg.TaskInfo.StackSize = FIFO_STACK_SIZE;
    AmbaKAL_TaskSleep(10);
    AmpFifo_Init(&FifoCfg);

    return 0;

}

static int CfsFileOperation(int opCode, UINT32 param)
{
    return (CfsEventHandler != NULL) ? CfsEventHandler(opCode, param) : AMP_OK;
}

static void InitCFS(void){
    AMP_CFS_CFG_s cfsCfg;
    void *cfshdlrAddr = NULL;

    AmbaPrint("==%s==", __FUNCTION__);
    /** Cfs Init */
    if (AmpCFS_GetDefaultCfg(&cfsCfg) != AMP_OK) {
        AmbaPrint("%s: Can't get default config.", __FUNCTION__);
        return;
    }
    /* we have 3 file to write in every stream,
     * and every file shall have 2 bank to use
     */
#define AMP_UT_CFS_BANK_AMOUNT  ((AMP_UT_CFS_MAX_STREAM_NUM) * (AMP_UT_CFS_FILE_PER_STREAM) * (AMP_UT_CFS_BANK_PER_FILE))
    if (cfsCfg.SchBankAmount <= AMP_UT_CFS_BANK_AMOUNT) {
        cfsCfg.SchBankAmount = AMP_UT_CFS_BANK_AMOUNT;
    }
#undef AMP_UT_CFS_BANK_AMOUNT

    cfsCfg.BufferSize = AmpCFS_GetRequiredBufferSize(cfsCfg.SchBankSize, \
                                               cfsCfg.SchBankAmount, cfsCfg.TaskInfo.StackSize, cfsCfg.SchTaskAmount, \
                                               cfsCfg.CacheEnable, cfsCfg.CacheMaxFileNum);
    /** Allocate memory */
    if (AmpUtil_GetAlignedPool(&G_MMPL, (void **) &cfsCfg.Buffer, &cfshdlrAddr, cfsCfg.BufferSize, AMBA_CACHE_LINE_SIZE) != AMP_OK) {
        AmbaPrint("%s: Can't allocate memory.", __FUNCTION__);
        return;
    }
    /** Set call back function*/
    cfsCfg.FileOperation = CfsFileOperation;
    /** Initialize CFS */
    if (AmpCFS_Init(&cfsCfg) != AMP_OK) {
        AmbaPrint("%s: Can't initialize.", __FUNCTION__);
        AmbaKAL_BytePoolFree(cfshdlrAddr);
        return;
    }

    return;
}

#define MAX_AUD_EVENT_HANDLER (3)
// audio event hdlr

#ifdef CONFIG_AUDIO_TITLV320AIC3256
extern AMBA_AUDIO_CODEC_OBJ_s AmbaAudio_Tlv320Aic3256Obj;
#endif

#ifdef CONFIG_AUDIO_WM8974
extern AMBA_AUDIO_CODEC_OBJ_s AmbaAudio_WM8974Obj;
#endif

int UT_AudioInit(void)
{
#ifdef CONFIG_AUDIO_TITLV320AIC3256
    AmbaAduio_CodecHook(AMBA_AUDIO_CODEC_0, &AmbaAudio_Tlv320Aic3256Obj);
#endif

#ifdef CONFIG_AUDIO_WM8974
    AmbaAduio_CodecHook(AMBA_AUDIO_CODEC_0, &AmbaAudio_WM8974Obj);
#endif

    AmbaUserAudioCodec_Init();
    return 0;
}
#if (defined(CONFIG_ENABLE_AMBALINK) || defined(CONFIG_SOC_A12))
/*----------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaLink_SuspendDoneCallBackImpl
 *
 *  @Description:: Call back function for Linux suspend done.
 *
 *  @Input      ::
 *      SuspendMode:  The mode to suspend. See AMBA_LINK_SUSPEND_MODE_e.
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*----------------------------------------------------------------------------*/
void AmbaLink_SuspendDoneCallBackImpl(UINT32 SuspendMode)
{
    switch (SuspendMode) {
    case AMBA_LINK_HIBER_TO_DISK:
        AmbaPrint("Linux suspend AMBA_LINK_HIBER_TO_DISK is done");
        AmbaPrint("====> auto reboot...\r\n\r\n\r\n");
        AmbaKAL_TaskSleep(1000);
        AmbaSysSoftReset();
        break;
    case AMBA_LINK_HIBER_TO_RAM:
        AmbaPrint("Linux suspend AMBA_LINK_HIBER_TO_RAM is done");
        break;
    case AMBA_LINK_STANDBY_TO_RAM:
        AmbaPrint("Linux suspend AMBA_LINK_STANDBY_TO_RAM is done");
        break;
    case AMBA_LINK_SLEEP_TO_RAM:
        AmbaPrint("Linux suspend AMBA_LINK_SLEEP_TO_RAM is done");
        break;
    default:
        break;
    }
}

/*----------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaLink_ResumeDoneCallBackImpl
 *
 *  @Description:: Call back function for Linux Resume done
 *
 *  @Input      ::
 *      SuspendMode:  The mode resume from. See AMBA_LINK_SUSPEND_MODE_e.
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*----------------------------------------------------------------------------*/
void AmbaLink_ResumeDoneCallBackImpl(UINT32 SuspendMode)
{
    int BootType;

    BootType = AmbaLink_BootType(3000);

    AmbaPrint("Linux is %s and IPC is ready!",
        (BootType == AMBALINK_COLD_BOOT) ? "ColdBoot" :
        (BootType == AMBALINK_WARM_BOOT) ? "WarmBoot" : "Hibernation Boot");

    if (BootType == AMBALINK_COLD_BOOT) {
        return;
    }

    switch (SuspendMode) {
    case AMBA_LINK_HIBER_TO_DISK:
        AmbaPrint("Linux resume AMBA_LINK_HIBER_TO_DISK is done");
        break;
    case AMBA_LINK_HIBER_TO_RAM:
        AmbaPrint("Linux resume AMBA_LINK_HIBER_TO_RAM is done");
        break;
    case AMBA_LINK_STANDBY_TO_RAM:
        AmbaPrint("Linux  resume AMBA_LINK_STANDBY_TO_RAM is done");
        break;
    case AMBA_LINK_SLEEP_TO_RAM:
        AmbaPrint("Linux resume AMBA_LINK_SLEEP_TO_RAM is done");
        break;
    default:
        break;
    }
}

/*----------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaLink_HookDefaultUserRPC
 *
 *  @Description:: Task to wait Linux RPC module ready and hook default User RPC services
 *
 *  @Input      ::
 *      info:  Specify Hibernation is enabled or not.
 *             1:hibernation is enabled, 0:hibernation is disable
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*----------------------------------------------------------------------------*/
void AmbaLink_HookDefaultUserRPC(UINT32 info)
{
    int BootType;

    BootType = AmbaLink_BootType(30000); //blocked until Linux ready
    AmbaPrint("AmbaLink_HookDefaultUserRPC: info = %u",info);

    if (BootType == AMBALINK_COLD_BOOT) {
        if(info == 1){ //hibernation enabled
            /* system should not do anything for cold boot, which will do hibernation later. */
            /* If your project does not enable hibernation, you have to remove this protection. */
            return;
        }
    }

    /* Init Event Notifier to receive Linux boot_done event */
    do {
        extern int AmpUT_NetEventNotifier_init(void) __attribute__((weak));

        if (AmpUT_NetEventNotifier_init) {
            AmpUT_NetEventNotifier_init();
            AmbaPrint("Doing AmpUT_NetEventNotifier_init...");
        }
    } while(0);
}

static void AmbaLink_KillHookDefaultUserRPCTask(AMBA_KAL_TASK_t *pTask, UINT32 Condition)
{
    if (Condition == TX_THREAD_EXIT) {
        if (AmbaKAL_TaskDelete(pTask) != OK) {
            AmbaPrint("%s: failed!", __func__);
        }
    }
}
#endif

/**
 * User defined System Initializations (before OS running)
 *
 * @return none
 */
void AmbaUserSysInitPreOS(void)
{
    extern void AmbaRTSL_GpioInit(AMBA_GPIO_PIN_GROUP_CONFIG_s *);
    extern AMBA_GPIO_PIN_GROUP_CONFIG_s GpioPinGrpConfig;

    AmbaRTSL_GpioInit(&GpioPinGrpConfig);   /* GPIO Initializations before OS running */
}

static void UT_PrintkInit(int dspLogSz, UINT8* dspLogAddr)
{
    /* The buffer is used to queue characters sent through AmbaPrint */
    static UINT8 AmbaPrintRingBuf[128 * 1024] __attribute__((section(".bss.noinit")));

    static AMBA_PRINT_CONFIG_s AmbaPrintConfig = {
        .Priority = AMBA_KAL_TASK_LOWEST_PRIORITY - 1,
        .PrintBufSize = sizeof(AmbaPrintRingBuf),
        .pPrintBuf = (UINT8 *) & (AmbaPrintRingBuf[0]),
        .DspDebugLogAreaSize = 0,
        .pDspDebugLogDataArea = NULL,
        .PutCharCallBack = AmbaUserConsole_Write
    };
    AmbaPrintConfig.DspDebugLogAreaSize = dspLogSz;
    AmbaPrintConfig.pDspDebugLogDataArea = dspLogAddr;

    AmbaPrint_Init(&AmbaPrintConfig);
}


static void UT_UARTInit(void)
{
    static UINT8 UartWorkingBuf[1024] __attribute__((section(".bss.noinit")));
    AMBA_UART_CONFIG_s UartConfig = {115200, AMBA_UART_DATA_8_BIT, AMBA_UART_PARITY_NONE, AMBA_UART_STOP_1_BIT, AMBA_UART_FLOW_CTRL_NONE, 0, 0};

    UartConfig.MaxRxRingBufSize = 1024;
    UartConfig.pRxRingBuf = UartWorkingBuf;

    AmbaUART_Config(AMBA_UART_CHANNEL0, &UartConfig);
}


static void UT_ShellInit(void)
{
    static char *AutoExecScripts[] = {
        "c:\\autoexec.ash",
        "d:\\autoexec.ash"
    };
    AMBA_SHELL_CONFIG_s ShellConfig = {0};

    // enable shell
    ShellConfig.Priority = 160;
    ShellConfig.pCachedHeap = &G_MMPL;
    ShellConfig.ConsolePutCharCb = AmbaUserConsole_Write;
    ShellConfig.ConsoleGetCharCb = AmbaUserConsole_Read;
    AmbaShell_Init(&ShellConfig);
    AmbaShell_Start(GetArraySize(AutoExecScripts), AutoExecScripts);
    AmbaPrint("AmbaShell Started");
}

static void UT_PLLInit(void)
{
#ifdef BSP_B5_MULTICHAN
    AMBA_OPMODE_CLK_PARAM_s AmbaInitClk = {
        .IdspClkFreq = 304000000,
        .CoreClkFreq = 384000000,
        .CortexClkFreq = 504000000,
    };

    AmbaInitClk.IdspClkFreq = 328000000;
    // Set initial clocks
    AmbaPLL_SetOpMode(&AmbaInitClk);
#else
    //A12 already put MAX freq in bootloader
#endif
}

static void UT_MWInit(AMP_MW_INIT_CFG_s *mwInitCfg)
{
    int Er;
    char* tempPtr;
    UINT8* ImgKernelWorkArea = NULL;
    UINT8* ImgKernelWorkAreaAligned = NULL;

    AmpMW_GetDefaultInitCfg(mwInitCfg);

    // Memory allocation
    mwInitCfg->SizeDspWorkArea = AmpUT_GetProjectDspMaxSize();
    mwInitCfg->DspWorkingArea = (UINT8*) DspWorkAreaResvStart;
    mwInitCfg->SizeDspDbgLogDataArea = 0x20000;

    Er = AmbaKAL_BytePoolAllocate(&G_NC_MMPL, (void **) &tempPtr, mwInitCfg->SizeDspDbgLogDataArea + 64, 100);

    tempPtr = (char*) ALIGN_64((UINT32)tempPtr);
    mwInitCfg->DspDbgLogDataArea = (UINT8*) tempPtr;

    // enable print
    UT_PrintkInit(mwInitCfg->SizeDspDbgLogDataArea, mwInitCfg->DspDbgLogDataArea);

    AmbaVer_Init();
    // Query and set IK's working buffer
    {
        int MemSize = 0;
        AMBA_DSP_IMG_ARCH_INFO_s ArchInfo = {0};
        static AMBA_DSP_IMG_PIPE_INFO_s PipeInfo[2] = {
                                    [0] = {
                                        .Pipe = AMBA_DSP_IMG_PIPE_VIDEO,
                                        .CtxBufNum = 2,
                                        .CfgBufNum = 20/*1*/,
                                    },
                                    [1] = {
                                        .Pipe = AMBA_DSP_IMG_PIPE_STILL,
                                        .CtxBufNum = 1,
                                        .CfgBufNum = 1,
                                    },
                                };
        memset(&ArchInfo, 0x0, sizeof(AMBA_DSP_IMG_ARCH_INFO_s));
//DualVin need double ctx
#ifdef CONFIG_SENSOR_B5_OV4689
        PipeInfo[0].CtxBufNum = 4;
        PipeInfo[0].CfgBufNum = 20;
#endif

#ifdef CONFIG_SOC_A9
        {
            static AMBA_DSP_IMG_DEF_TBL_s ImgDefaultTable = {0};

            Er = AmbaROM_LoadByName(AMBA_ROM_SYS_DATA, "UniformInputForSpecialCc.bin",
                                    (UINT8 *) &ImgDefaultTable.UniformInputForSpecialCc, AMBA_DSP_IMG_UNIFORM_INPUT_NUM, 0);
            if(Er != AMBA_DSP_IMG_UNIFORM_INPUT_NUM) {
                AmbaPrint("Load UniformInputForSpecialCc.bin failed!");
            }

            Er = AmbaROM_LoadByName(AMBA_ROM_SYS_DATA, "MctfTable.bin",
                                    (UINT8 *) &ImgDefaultTable.DefVideoMctfBin, AMBA_DSP_IMG_MCTF_CFG_SIZE, 0);
            if(Er != AMBA_DSP_IMG_MCTF_CFG_SIZE) {
                AmbaPrint("Load MctfTable.bin failed!");
            } else {
                memcpy(&ImgDefaultTable.DefIsoMctfBin, &ImgDefaultTable.DefVideoMctfBin, AMBA_DSP_IMG_MCTF_CFG_SIZE);
                memcpy(&ImgDefaultTable.DefHighIsoMctfBin, &ImgDefaultTable.DefVideoMctfBin, AMBA_DSP_IMG_MCTF_CFG_SIZE);
            }

            Er = AmbaROM_LoadByName(AMBA_ROM_SYS_DATA, "MctfCmpr.bin",
                                    (UINT8 *) &ImgDefaultTable.DefMctfCmprBin, AMBA_DSP_IMG_CMPR_CFG_SIZE, 0);
            if(Er != AMBA_DSP_IMG_CMPR_CFG_SIZE) {
                AmbaPrint("Load MctfCmpr.bin failed!");
            }

            Er = AmbaROM_LoadByName(AMBA_ROM_SYS_DATA, "MctfSecCc.bin",
                                    (UINT8 *) &ImgDefaultTable.DefMctfSecCcBin, AMBA_DSP_IMG_CC_CFG_SIZE, 0);
            if(Er != AMBA_DSP_IMG_CC_CFG_SIZE) {
                AmbaPrint("Load MctfSecCc.bin failed!");
            }

            Er = AmbaROM_LoadByName(AMBA_ROM_SYS_DATA, "HisoL4CEdgeMctfTable.bin",
                                    (UINT8 *) &ImgDefaultTable.DefHighIsoL4CEdgeMctfBin, AMBA_DSP_IMG_MCTF_CFG_SIZE, 0);
            if(Er != AMBA_DSP_IMG_MCTF_CFG_SIZE) {
                AmbaPrint("Load HisoL4CEdgeMctfTable.bin failed!");
            }

            Er = AmbaROM_LoadByName(AMBA_ROM_SYS_DATA, "HisoMix124XMctfTable.bin",
                                    (UINT8 *) &ImgDefaultTable.DefHighIsoMix124XMctfBin, AMBA_DSP_IMG_MCTF_CFG_SIZE, 0);
            if(Er != AMBA_DSP_IMG_MCTF_CFG_SIZE) {
                AmbaPrint("Load AMBA_DSP_IMG_MCTF_CFG_SIZE.bin failed!");
            }
            mwInitCfg->ImgKernelDefaultTable = &ImgDefaultTable;
        }
#endif
#ifdef BSP_B5_MULTICHAN
        PipeInfo[0].CtxBufNum = 10;
        PipeInfo[0].CfgBufNum = 1;
#endif
        mwInitCfg->NumImgKernelPipe = ArchInfo.PipeNum = 2;
        mwInitCfg->ImgKernelPipeInfo = &PipeInfo[0];
        for (UINT8 t = 0; t < ArchInfo.PipeNum; t++) {
            ArchInfo.pPipeInfo[t] = &PipeInfo[t];
        }
#ifdef CONFIG_SOC_A12 //FIXME
        mwInitCfg->NumImgKernelBufNum = ArchInfo.BufNum = 2; //TBD at least 2 in singel channel
#else
        mwInitCfg->NumImgKernelBufNum = 0; //allocate by SSP
#endif

#ifdef BSP_B5_MULTICHAN
        //TBD
        MemSize = mwInitCfg->SizeImgKernelWorkArea = 16200000;
#else
        MemSize = AmbaDSP_ImgQueryArchMemSize(&ArchInfo)*12/10;
#endif
        if (MemSize >=0) {
            Er = AmpUtil_GetAlignedPool(&G_MMPL, (void **) &ImgKernelWorkAreaAligned, (void **)&ImgKernelWorkArea, MemSize, AMBA_CACHE_LINE_SIZE);
            if (Er != OK) {
                AmbaPrint("CacheDDR alloc failed!!!!!!!!!");
                K_ASSERT(0);
            } else {
                AmbaPrint("IK working area start %x size %d",ImgKernelWorkAreaAligned, MemSize);
            }
            mwInitCfg->ImgKernelWorkArea = (UINT8*) ALIGN_32((UINT32)ImgKernelWorkAreaAligned);
            mwInitCfg->SizeImgKernelWorkArea = MemSize;
        }
    }

    // To enable seamless, set EnSeamless to 1 and set LCD size by AmpDisplay_SetMaxVout0Size().
    // To disable seamless, set EnSeamless to 0 or set LCD size to 0.
    // EnSeamless has to be configured here.
    // AmpDisplay_SetMaxVout0Size() can be called before DSP boot (not necessary here).
    switch (UT_Pref_Check(UT_PREF_SEAMLESS)) {
        case UT_PREF_SEAMLESS_ENABLE:
            mwInitCfg->EnSeamless = 1; // Enable
            break;
        case UT_PREF_SEAMLESS_DISABLE:
            mwInitCfg->EnSeamless = 0; // Disable
            break;
        default:
            mwInitCfg->EnSeamless = 1; // Enable by default in main branch
            break;
    }
    AmbaPrint("%s: Set EnSeamless = %u", __FUNCTION__, mwInitCfg->EnSeamless);
    AmpDisplay_SetMaxVout0Size(MAX_LCD_WIDTH, MAX_LCD_HEIGHT); // Set Vout0 size to make seamless work
    AmpMW_Init(mwInitCfg);
}

static void UT_AmbaLinkInit(void)
{

#ifdef CONFIG_ENABLE_AMBALINK
        do {
            extern int AmpUT_IPCTestAdd(void)  __attribute__((weak));
            extern  int AmpUT_IPCTestInit(AMBA_KAL_BYTE_POOL_t *pCachedHeap)  __attribute__((weak));
            extern  int AmpUT_HiberTestAdd(void)  __attribute__((weak));
            extern  int AmpUT_NetFifo_TestAdd(void)  __attribute__((weak));
            extern  int AmpUT_NetPbFifo_TestAdd(void) __attribute__((weak));
            extern  int AmpUT_NetEventNotifier_TestAdd(void)  __attribute__((weak));
            extern int AmpUT_NetDec_TestAdd(void)  __attribute__((weak));
            extern AMBA_LINK_CTRL_s AmbaLinkCtrl;
            extern AMBA_LINK_TASK_CTRL_s AmbaLinkTaskCtrl;

            if (AmpUT_IPCTestInit) {
                AmbaPrint("register AmbaLink_BootDoneCallBack");
                AmpUT_IPCTestInit(&G_MMPL);
                AmbaLink_UserIpcInitCallBack = AmpUT_IPCTestAdd;
            } else {
                AmbaPrint("Cannot register AmbaLink_BootDoneCallBack");
            }

            if (AmpUT_HiberTestAdd) {
                AmbaPrint("register AmbaHiber_InitCallBack");
                AmbaHiber_InitCallBack = AmpUT_HiberTestAdd;
            } else {
                AmbaPrint("Cannot register AmbaHiber_InitCallBack");
            }

            if (AmpUT_NetFifo_TestAdd) {
                AmpUT_NetFifo_TestAdd();
            }

            if (AmpUT_NetPbFifo_TestAdd) {
                AmpUT_NetPbFifo_TestAdd();
            }

            if (AmpUT_NetEventNotifier_TestAdd) {
                AmpUT_NetEventNotifier_TestAdd();
            }

            if (AmpUT_NetDec_TestAdd) {
                AmpUT_NetDec_TestAdd();
            }

            AmbaIPC_LinkCtrlSuspendLinuxDoneCallBack = (VOID_UINT32_IN_FUNCTION) AmbaLink_SuspendDoneCallBackImpl;
            AmbaIPC_LinkCtrlResumeLinuxDoneCallBack = (VOID_UINT32_IN_FUNCTION) AmbaLink_ResumeDoneCallBackImpl;

            AmbaLinkCtrl.pBytePool = &G_MMPL;

#if 1
            AmbaPrint("AmbaLinkMachineID=%x",AmbaLinkCtrl.AmbaLinkMachineID);
            AmbaPrint("AmbaLinkMachineRev=%x",AmbaLinkCtrl.AmbaLinkMachineRev);
            AmbaPrint("AmbaLinkSharedMemAddr=%x",AmbaLinkCtrl.AmbaLinkSharedMemAddr);
            AmbaPrint("AmbaLinkMemSize=%x",AmbaLinkCtrl.AmbaLinkMemSize);

            AmbaPrint("AmbarellaPPMSize=%x",AmbaLinkCtrl.AmbarellaPPMSize);
            AmbaPrint("AmbarellaZRealAddr=%x",AmbaLinkCtrl.AmbarellaZRealAddr);
            AmbaPrint("AmbarellaTextOfs=%x",AmbaLinkCtrl.AmbarellaTextOfs);
            AmbaPrint("AmbarellaParamsPhys=%x",AmbaLinkCtrl.AmbarellaParamsPhys);

            AmbaPrint("VringC0AndC1BufAddr=%x",AmbaLinkCtrl.VringC0AndC1BufAddr);
            AmbaPrint("VringC0ToC1Addr=%x",AmbaLinkCtrl.VringC0ToC1Addr);
            AmbaPrint("VringC1ToC0Addr=%x",AmbaLinkCtrl.VringC1ToC0Addr);
            AmbaPrint("RpmsgNumBuf=%x",AmbaLinkCtrl.RpmsgNumBuf);
            AmbaPrint("RpmsgBufSize=%x",AmbaLinkCtrl.RpmsgBufSize);

            AmbaPrint("AmbaIpcSpinLockBufAddr=%x",AmbaLinkCtrl.AmbaIpcSpinLockBufAddr);
            AmbaPrint("AmbaIpcSpinLockBufSize=%x",AmbaLinkCtrl.AmbaIpcSpinLockBufSize);
            AmbaPrint("AmbaIpcMutexBufAddr=%x",AmbaLinkCtrl.AmbaIpcMutexBufAddr);
            AmbaPrint("AmbaIpcMutexBufSize=%x",AmbaLinkCtrl.AmbaIpcMutexBufSize);

            AmbaPrint("AmbaRpcProfileAddr=%x",AmbaLinkCtrl.AmbaRpcProfileAddr);
            AmbaPrint("AmbaRpmsgProfileAddr=%x",AmbaLinkCtrl.AmbaRpmsgProfileAddr);

            AmbaPrint("RpmsgSuspBackupAddr=%x",AmbaLinkCtrl.RpmsgSuspBackupAddr);
            AmbaPrint("RpmsgSuspBackupSize=%x",AmbaLinkCtrl.RpmsgSuspBackupSize);
            AmbaPrint("BossSuspBackupAddr=%x",AmbaLinkCtrl.BossSuspBackupAddr);
            AmbaPrint("BossSuspBackupSize=%x",AmbaLinkCtrl.BossSuspBackupSize);
            AmbaPrint("BossDataAddr=%x",AmbaLinkCtrl.BossDataAddr);
            AmbaPrint("BossDataSize=%x",AmbaLinkCtrl.BossDataSize);

            AmbaPrint("AmbaLinkAossAddr=%x",AmbaLinkCtrl.AmbaLinkAossAddr);
            AmbaPrint("AmbaHiberImgCheck=%x",AmbaLinkCtrl.AmbaHiberImgCheck);

            AmbaPrint("SvcPriority=%x",AmbaLinkTaskCtrl.SvcPriority);
            AmbaPrint("SvcStackSize=%x",AmbaLinkTaskCtrl.SvcStackSize);
            AmbaPrint("SvcThreadNum=%x",AmbaLinkTaskCtrl.SvcThreadNum);

            AmbaPrint("RvqPriority=%x",AmbaLinkTaskCtrl.RvqPriority);
            AmbaPrint("RvqStackSize=%x",AmbaLinkTaskCtrl.RvqStackSize);
            AmbaPrint("SvqPriority=%x",AmbaLinkTaskCtrl.SvqPriority);
            AmbaPrint("SvqStackSize=%x",AmbaLinkTaskCtrl.SvqStackSize);

            AmbaPrint("VfsOpsPriority=%x",AmbaLinkTaskCtrl.VfsOpsPriority);
            AmbaPrint("VfsOpsStackSize=%x",AmbaLinkTaskCtrl.VfsOpsStackSize);
            AmbaPrint("VfsSgPriority=%x",AmbaLinkTaskCtrl.VfsSgPriority);
            AmbaPrint("VfsSgStackSize=%x",AmbaLinkTaskCtrl.VfsSgStackSize);

            AmbaPrint("pKernelCmdLine=%s",AmbaLinkCtrl.pKernelCmdLine);

            AmbaPrint("pBytePool=%p",AmbaLinkCtrl.pBytePool);

            AmbaPrint("AmbaLinkRunTarget=%x",AmbaLinkCtrl.AmbaLinkRunTarget);
            AmbaPrint("AmbaLinkUsbOwner=%x",AmbaLinkCtrl.AmbaLinkUsbOwner);
            AmbaPrint("pPrivate=%p",AmbaLinkCtrl.pPrivate);
#endif
            AmbaLink_Init();
            AmbaLink_Load();
            AmbaLink_Boot(10000);
            /* Create task to hook default service right after RPC ready */
            AmbaKAL_TaskCreate(&HookDefaultUserRPCTask, /* pTask */
                            "HookDefaultUserRPCTask", /* pTaskName */
                            50, /* Priority */
                            AmbaLink_HookDefaultUserRPC, /* void (*EntryFunction)(UINT32) */
                            0x1, /* EntryArg, 1:hibernation enabled */
                            (void *) HookDefaultUserRPCTaskStack, /* pStackBase */
                            HookDefaultUserRPCTaskStackSize, /* StackByteSize */
                            AMBA_KAL_AUTO_START); /* AutoStart */

            if (tx_thread_entry_exit_notify(&HookDefaultUserRPCTask, AmbaLink_KillHookDefaultUserRPCTask) != OK) {
                AmbaPrint("Fail to do thread_entry_exit_notify(AmbaLink_KillHookDefaultUserRPCTask).\n");
            }
        } while (0);
#else
        {
//            extern  int AmpUT_NetCtrl_TestAdd(void) ;
//            if (AmpUT_NetCtrl_TestAdd) {
//                AmpUT_NetCtrl_TestAdd();
//            }
        }
#endif /* CONFIG_ENABLE_AMBALINK */

}

static void UT_HookTestFunc(void)
{
    {   /**< Hook dummy encoder for FIFO test */
        extern  int AmpUT_DummyEncInit(void)  __attribute__((weak));

        if (AmpUT_DummyEncInit) {
            AmpUT_DummyEncInit();
        }
    }
    {   /**< Hook dummy decoder for FIFO test */
        extern  int AmpUT_DummyDecInit(void)  __attribute__((weak));

        if (AmpUT_DummyDecInit) {
            AmpUT_DummyDecInit();
        }
    }
    {   /**< Hook video decode unit test */
        extern  int AmpUT_VideoDecInit(void)  __attribute__((weak));

        if (AmpUT_VideoDecInit) {
            AmpUT_VideoDecInit();
        }
    }
    {   /**< Hook multiple video decode unit test */
        extern  int AmpUT_VideoMultiDecInit(void)  __attribute__((weak));
        if (AmpUT_VideoMultiDecInit) {
            AmpUT_VideoMultiDecInit();
        }
    }
    {   /**< Hook still decode unit test */
        extern  int AmpUT_StlDecInit(void)  __attribute__((weak));

        if (AmpUT_StlDecInit) {
            AmpUT_StlDecInit();
        }
    }
    {   /**< Hook display unit test */
        extern  int AmpUT_DisplayInit(void)  __attribute__((weak));
        if (AmpUT_DisplayInit) {
            AmpUT_DisplayInit();
        }
    }
    {   /**< Hook OSD unit test */
        extern  int AmpUT_OsdInit(void)  __attribute__((weak));

        if (AmpUT_OsdInit) {
            AmpUT_OsdInit();
        }
    }
    {   /**< Hook Video Encode unit test */
        extern  int AmpUT_VideoEncTestAdd(void) __attribute__((weak)) ;

        if (AmpUT_VideoEncTestAdd) {
            AmpUT_VideoEncTestAdd();
        }
    }
    {   /**< Hook sensor unit test */
        extern  int SensorUT_TestAdd(void) __attribute__((weak)) ;

        if (SensorUT_TestAdd) {
            SensorUT_TestAdd();
        }
    }
    {   /**< Hook "t dsp" command for ssp debug message */
        extern void AmbaTest_DspAddCommands(void) __attribute__((weak)) ;
        AmbaTest_DspAddCommands();
    }
    {   /**< Hook Still Encode unit test */
        extern  int AmpUT_StillEncTestAdd(void)  __attribute__((weak));

        if (AmpUT_StillEncTestAdd) {
            AmpUT_StillEncTestAdd();
        }
    }
    {   /**< Hook CFS unit test */
        extern int AmpUT_CfsTestAdd(void)  __attribute__((weak));

        if (AmpUT_CfsTestAdd) {
            AmpUT_CfsTestAdd();
        }
    }
    {   /**< Hook FCF unit test */
        extern int AmpUT_DcfTestAdd(void)  __attribute__((weak));

        if (AmpUT_DcfTestAdd) {
            AmpUT_DcfTestAdd();
        }
    }
    {   /**< Hook Mp4 muxer single stream unit test */
        extern int AmpUT_Mp4MuxTestAdd(void)  __attribute__((weak));

        if (AmpUT_Mp4MuxTestAdd) {
            AmpUT_Mp4MuxTestAdd();
        }
    }
    {   /**< Hook External muxer unit test */
        extern int AmpUT_ExtMuxTestAdd(void)  __attribute__((weak));

        if (AmpUT_ExtMuxTestAdd) {
            AmpUT_ExtMuxTestAdd();
        }
    }
    {   /**< Hook ExternalAV muxer unit test */
        extern int AmpUT_ExtMuxAVTestAdd(void)  __attribute__((weak));
        if (AmpUT_ExtMuxAVTestAdd) {
            AmpUT_ExtMuxAVTestAdd();
        }
    }
    {   /**< Hook External demuxer unit test */
        extern int AmpUT_ExtDmxTestAdd(void)  __attribute__((weak));

        if (AmpUT_ExtDmxTestAdd) {
            AmpUT_ExtDmxTestAdd();
        }
    }
    {   /**< Hook MP4 dualstream muxer unit test */
        extern int AmpUT_Mp4MuxDualTestAdd(void)  __attribute__((weak));

        if (AmpUT_Mp4MuxDualTestAdd) {
            AmpUT_Mp4MuxDualTestAdd();
        }
    }
    {   /**< Hook MP4 AV muxer unit test */
        extern int AmpUT_Mp4MuxAVTestAdd(void)  __attribute__((weak));
        if (AmpUT_Mp4MuxAVTestAdd) {
            AmpUT_Mp4MuxAVTestAdd();
        }
    }
    {   /**< Hook External muxer unit test */
        extern int AmpUT_Mp4Mux2sTestAdd(void)  __attribute__((weak));
        if (AmpUT_Mp4Mux2sTestAdd) {
            AmpUT_Mp4Mux2sTestAdd();
        }
    }
    {
        extern int AmpUT_Mp4Mux4sTestAdd(void)  __attribute__((weak));
        if (AmpUT_Mp4Mux4sTestAdd) {
            AmpUT_Mp4Mux4sTestAdd();
        }
    }
    {
        extern int AmpUT_Mp4DmxTestAdd(void)  __attribute__((weak));
        if (AmpUT_Mp4DmxTestAdd) {
            AmpUT_Mp4DmxTestAdd();
        }
    }
    {
        extern int AmpUT_Mp4DmxAVTestAdd(void)  __attribute__((weak));
        if (AmpUT_Mp4DmxAVTestAdd) {
            AmpUT_Mp4DmxAVTestAdd();
        }
    }
    {
        extern int AmpUT_Mp4EdtTestAdd(void)  __attribute__((weak));
        if (AmpUT_Mp4EdtTestAdd) {
            AmpUT_Mp4EdtTestAdd();
        }
    }
    {
        extern int AmpUT_MovMuxTestAdd(void)  __attribute__((weak));
        if (AmpUT_MovMuxTestAdd) {
            AmpUT_MovMuxTestAdd();
        }
    }
    {
        extern int AmpUT_MovDmxTestAdd(void)  __attribute__((weak));
        if (AmpUT_MovDmxTestAdd) {
            AmpUT_MovDmxTestAdd();
        }
    }
    {
        extern int AmpUT_MovEdtTestAdd(void)  __attribute__((weak));
        if (AmpUT_MovEdtTestAdd) {
            AmpUT_MovEdtTestAdd();
        }
    }
    {
        extern int AmpUT_MkvMuxTestAdd(void)  __attribute__((weak));
        if (AmpUT_MkvMuxTestAdd) {
            AmpUT_MkvMuxTestAdd();
        }
    }
    {
        extern int AmpUT_MkvMuxAVTestAdd(void)  __attribute__((weak));
        if (AmpUT_MkvMuxAVTestAdd) {
            AmpUT_MkvMuxAVTestAdd();
        }
    }
    {
        extern int AmpUT_MkvMuxDualTestAdd(void) __attribute__((weak));
        if (AmpUT_MkvMuxDualTestAdd) {
            AmpUT_MkvMuxDualTestAdd();
        }
    }
    {
        extern int AmpUT_MkvDmxTestAdd(void) __attribute__((weak));
        if (AmpUT_MkvDmxTestAdd) {
            AmpUT_MkvDmxTestAdd();
        }
    }
    {
        extern int AmpUT_MkvDmxAVTestAdd(void) __attribute__((weak));
        if (AmpUT_MkvDmxAVTestAdd) {
            AmpUT_MkvDmxAVTestAdd();
        }
    }
    {
        extern int AmpUT_MkvEdtTestAdd(void) __attribute__((weak));
        if (AmpUT_MkvEdtTestAdd) {
            AmpUT_MkvEdtTestAdd();
        }
    }
    {
        extern int AmpUT_EditorTestAdd(void) __attribute__((weak));
        if (AmpUT_EditorTestAdd) {
            AmpUT_EditorTestAdd();
        }
    }
    {
        extern int AmpUT_ExifMuxTestAdd(void)  __attribute__((weak));
        if (AmpUT_ExifMuxTestAdd) {
            AmpUT_ExifMuxTestAdd();
        }
    }
    {
        extern int AmpUT_ExifDmxTestAdd(void)  __attribute__((weak));
        if (AmpUT_ExifDmxTestAdd) {
            AmpUT_ExifDmxTestAdd();
        }
    }
    {
        extern int AmpUT_EventRecordTestAdd(void) __attribute__((weak));
        if (AmpUT_EventRecordTestAdd) {
            AmpUT_EventRecordTestAdd();
        }
    }
    {
        extern int AmpUT_EventRecordTestAdd_O(void)  __attribute__((weak));
        if (AmpUT_EventRecordTestAdd_O) {
            AmpUT_EventRecordTestAdd_O();
        }
    }
    {
        extern int AmpUT_EventRecordTestAdd_N(void)  __attribute__((weak));
        if (AmpUT_EventRecordTestAdd_N) {
            AmpUT_EventRecordTestAdd_N();
        }
    }
    {
        extern int AmpUT_EmTestAdd(void)  __attribute__((weak));
        if (AmpUT_EmTestAdd) {
            AmpUT_EmTestAdd();
        }
    }
    {
        extern int AmpUT_FioTestAdd(void)  __attribute__((weak));
        if (AmpUT_FioTestAdd) {
            AmpUT_FioTestAdd();
        }
    }

    {
        extern  int AmpUT_CalibModuleInit(void)  __attribute__((weak));
        if (AmpUT_CalibModuleInit) {
            AmpUT_CalibModuleInit();
        }
    }

    {   /**< Hook Audio Encode unit test */
        extern  int AmpUT_AudioEncTestAdd(void)  __attribute__((weak));
        if (AmpUT_AudioEncTestAdd) {
            AmpUT_AudioEncTestAdd();
        }
    }
    {
        extern  int AmpUT_ImgProcTestAdd(void)  __attribute__((weak));
        if (AmpUT_ImgProcTestAdd) {
            AmpUT_ImgProcTestAdd();
        }
    }
    {
        extern  int AmpUT_AudioDecTestAdd(void)  __attribute__((weak));
        if (AmpUT_AudioDecTestAdd) {
            AmpUT_AudioDecTestAdd();
        }
    }
#if 0
    {
        extern  int AmpUT_ImgkTestAdd(void);
        if (AmpUT_ImgkTestAdd) {
            AmpUT_ImgkTestAdd();
        }
    }
#endif
    {
        extern  int AmpUT_BeepTestAdd(void)  __attribute__((weak));
        if (AmpUT_BeepTestAdd) {
            AmpUT_BeepTestAdd();
        }
    }
    {
        extern  int AmpUT_TuneTestAdd(void)  __attribute__((weak));
        if (AmpUT_TuneTestAdd) {
            AmpUT_TuneTestAdd();
        }
    }
    {
        extern  int AmpUT_XcodeInit(void)  __attribute__((weak));
        if (AmpUT_XcodeInit) {
            AmpUT_XcodeInit();
        }
    }
    {   /**< Hook Video Tuning unit test */
        extern  int AmpUT_VideoTuningTestAdd(void)  __attribute__((weak));
        if (AmpUT_VideoTuningTestAdd) {
            AmpUT_VideoTuningTestAdd();
        }
    }
    {   /**< Hook Still Tuning unit test */
        extern  int AmpUT_StillTuningTestAdd(void)  __attribute__((weak));
        if (AmpUT_StillTuningTestAdd) {
            AmpUT_StillTuningTestAdd();
        }
    }
    {
        extern  int AmpUT_MapTestAdd(void)  __attribute__((weak));
        if (AmpUT_MapTestAdd) {
            AmpUT_MapTestAdd();
        }
    }
    {   /**< Hook External Encode unit test */
        extern  int AmpUT_ExtEncTestAdd(void)  __attribute__((weak));
        if (AmpUT_ExtEncTestAdd) {
            AmpUT_ExtEncTestAdd();
        }
    }
    {   /**< Hook Arm-Based transcode Encode unit test */
        extern  int AmpUT_TranscoderEncTestAdd(void)  __attribute__((weak));
        if (AmpUT_TranscoderEncTestAdd) {
            AmpUT_TranscoderEncTestAdd();
        }
    }

    {   /**< Hook SystemIO unit test */
        extern  int AmpUT_SystemIOTestAdd(void) __attribute__((weak)) ;
        if (AmpUT_SystemIOTestAdd) {
            AmpUT_SystemIOTestAdd();
        }
    }

    {
        extern  int AmpUT_NetCtrl_TestAdd(void)  __attribute__((weak));
        if (AmpUT_NetCtrl_TestAdd) {
            AmpUT_NetCtrl_TestAdd();
        }
    }

    {
        extern int AmpUT_CscInit(void) __attribute__((weak));
        if (AmpUT_CscInit) {
            AmpUT_CscInit();
        }
    }

    {   /**< Hook Audio Encode unit test */
        extern  int AmpUT_AVEncTestAdd(void)  __attribute__((weak));
        if (AmpUT_AVEncTestAdd) {
            AmpUT_AVEncTestAdd();
        }
    }
    {   /**< Hook "t schldr" command for image scheduler debug message */
        extern void AmbaTest_ImgSchdlrAddCommands(void) __attribute__((weak)) ;
        if (AmbaTest_ImgSchdlrAddCommands) {
            AmbaTest_ImgSchdlrAddCommands();
        }
    }
    {   /**< Hook "t encmon" command for image encode monitor debug message */
        extern void AmbaTest_ImgEncMonitorAddCommands(void) __attribute__((weak)) ;
        if (AmbaTest_ImgEncMonitorAddCommands) {
            AmbaTest_ImgEncMonitorAddCommands();
        }
    }
    {   /**< VA unit test */
        extern int AmpUT_VATestAdd(void) __attribute__((weak));
        if (AmpUT_VATestAdd) {
            AmpUT_VATestAdd();
        }
    }

    {   /**< Yuv input Video Encode unit test */
        extern int AmpUT_YUVEncTestAdd(void) __attribute__((weak));

        if (AmpUT_YUVEncTestAdd) {
            AmpUT_YUVEncTestAdd();
        }
    }

    {   /**< USB unit test */
        extern int AmbaTest_UsbAddCommands(void) __attribute__((weak));
        extern UINT32 AmbaSspUt_USB_SetMemoryPool(TX_BYTE_POOL *cached_pool, TX_BYTE_POOL *noncached_pool) __attribute((weak));

        if(AmbaSspUt_USB_SetMemoryPool) {
            AmbaSspUt_USB_SetMemoryPool(&G_MMPL, &G_NC_MMPL);
        }

        if (AmbaTest_UsbAddCommands) {
            AmbaTest_UsbAddCommands();
        }
    }

    {   /**< MSPUSB unit test */
        extern int AmbaTest_MspUsbAddCommands(void) __attribute__((weak));

        if (AmbaTest_MspUsbAddCommands) {
            AmbaTest_MspUsbAddCommands();
        }

    }
    {
        extern int AmpUT_TranscoderDecInit(void) __attribute__((weak));
        if (AmpUT_TranscoderDecInit) {
            AmpUT_TranscoderDecInit();
        }
    }
    {
        extern int AmpUT_YUVReadSDAdd(void) __attribute__((weak));
        if (AmpUT_YUVReadSDAdd) {
            AmpUT_YUVReadSDAdd();
        }
    }
    {
        extern int AmpUT_FrameHandlerTestAdd(void) __attribute__((weak));
        if (AmpUT_FrameHandlerTestAdd) {
            AmpUT_FrameHandlerTestAdd();
        }
    }

#ifdef CONFIG_CC_CXX_SUPPORT
    {   /* C++ test case */
        extern  int AmbaTest_cppAddCommands(void)  __attribute__((weak));

        if (AmbaTest_cppAddCommands) {
            AmbaTest_cppAddCommands();
        }
    }
#endif
#ifdef CONFIG_BT_AMBA
    {
        extern void AmbaTest_BT_AMBA_Add(void);

        AmbaTest_BT_AMBA_Add();
    }
#endif /* CONFIG_BT_AMBA */
    {
        extern int AmpUT_DualVinEncTestAdd(void) __attribute__((weak));
        if (AmpUT_DualVinEncTestAdd) {
            AmpUT_DualVinEncTestAdd();
        }
    }
    //AmbaTest_RegisterCommand("gpio", test_gpio);

}

static void UT_StorageInit(void)
{
    //FIXIT should be the same!!!!

    // Init FIO
    AmbaFIO_Init();
    AmbaFS_Init(AMBA_FS_ASCII);
    InitCFS();
    AmbaSCM_Init();

#ifdef CONFIG_SOC_A9
    AmbaSD_Init(2, SD_UHS_MAX_FREQ);    /* SD  Initializations */
    {
        extern AMBA_NAND_DEV_INFO_s AmbaNAND_DevInfo;
        AmbaNAND_Init(&AmbaNAND_DevInfo); /* NAND Initializations */
    }
    AmbaROM_Init(&G_MMPL); /* ROM Initializations */
    AmbaNFTL_InitLock(NFTL_ID_IDX);
    AmbaNFTL_InitLock(NFTL_ID_PRF);
    AmbaNFTL_InitLock(NFTL_ID_CAL);
    if (AmbaNFTL_Init(NFTL_ID_IDX, NFTL_MODE_NO_SAVE_TRL_TBL) != OK) {
        AmbaPrint("NFTL init NFTL_ID_IDX fail %d",AmbaNFTL_IsInit(NFTL_ID_IDX));
    }
    AmbaNFTL_Init(NFTL_ID_PRF, NFTL_MODE_NO_SAVE_TRL_TBL);
    AmbaNFTL_Init(NFTL_ID_CAL, NFTL_MODE_NO_SAVE_TRL_TBL);
#else //CONFIG_SOC_A12
#ifdef CONFIG_ENABLE_EMMC_BOOT
    AmbaEMMC_Init();
#else
    AmbaSD_Init(1, SD_UHS_MAX_FREQ);    /* SD  Initializations */
#if defined(CONFIG_ENABLE_SPINOR_BOOT)
    {
      extern AMBA_NOR_DEV_INFO_s AmbaNOR_DevInfo;
      AmbaNOR_Init(&AmbaNOR_DevInfo); /* NAND Initializations */
    }
#else
    {
      extern AMBA_NAND_DEV_INFO_s AmbaNAND_DevInfo;
      AmbaNAND_Init(&AmbaNAND_DevInfo); /* NAND Initializations */
    }
#endif //defined(CONFIG_ENABLE_SPINOR_BOOT)
#endif //CONFIG_ENABLE_EMMC_BOOT

#ifdef CONFIG_ENABLE_EMMC_BOOT
    AmbaROM_Init(&G_MMPL, AMBA_ROM_DEVICE_SM); /* ROM Initializations */
#elif defined(CONFIG_ENABLE_SPINOR_BOOT)
    AmbaROM_Init(&G_MMPL, AMBA_ROM_DEVICE_NOR); /* ROM Initializations */
#else
    AmbaROM_Init(&G_MMPL, AMBA_ROM_DEVICE_NAND); /* ROM Initializations */
#endif //CONFIG_ENABLE_EMMC_BOOT

#ifndef CONFIG_ENABLE_EMMC_BOOT
    AmbaNFTL_InitLock(NFTL_ID_IDX);
    AmbaNFTL_InitLock(NFTL_ID_PRF);
    AmbaNFTL_InitLock(NFTL_ID_CAL);
    if (AmbaNFTL_Init(NFTL_ID_IDX, NFTL_MODE_NO_SAVE_TRL_TBL) != OK) {
        AmbaPrint("NFTL init NFTL_ID_IDX fail %d",AmbaNFTL_IsInit(NFTL_ID_IDX));
    }
    AmbaNFTL_Init(NFTL_ID_PRF, NFTL_MODE_NO_SAVE_TRL_TBL);
    AmbaNFTL_Init(NFTL_ID_CAL, NFTL_MODE_NO_SAVE_TRL_TBL);
#endif
#endif //CONFIG_SOC_A12
}

static void UT_InitADrive(void)
{
#ifndef BSP_B5_MULTICHAN
#ifndef CONFIG_ENABLE_EMMC_BOOT
        AmbaNFTL_InitLock(NFTL_ID_STORAGE);
        AmbaNFTL_Init(NFTL_ID_STORAGE, NFTL_MODE_NO_SAVE_TRL_TBL);
        {
            extern void AmbaFS_Nand1CheckMedia(void);
            AmbaFS_Nand1CheckMedia();
        }
        if (AUTO_FORMAT_NAND_STG) {
            int Slot = SCM_SLOT_FL0, local_rval = 0;
            AMBA_SCM_STATUS_s CardStatus;

            local_rval = AmbaSCM_GetSlotStatus(Slot, &CardStatus);
            if (local_rval < 0) {
                AmbaPrint("cannot get Nand slot Status!\n");
            } else {
                /* If NAND storage is not Formatted, Format it */
                if ((CardStatus.CardPresent) && (CardStatus.Format == FS_FAT_UNKNOWN)) {    // not Formated
                  /* For erase all, boot after burning code for the first time*/
                  const char ParamMem[13] = {'F','A','T','1','6',',','s','p','c','=','6','4','\0'};

                  AmbaPrint("Auto format A drive!\n");
                  AmbaFS_Format('A', ParamMem);
                }
            }
        }
        AmbaPrint("Drive A inserted");
#endif
#endif
}

void SystemInitTaskEntry(UINT32 info)
{
    UINT8* CacheDdrStart;
    UINT8* CacheDdrLimit;
    UINT8* NonCacheDdrStart;
    UINT8* NonCacheDdrLimit;
    extern void *__cache_heap_start;
    extern void *__cache_heap_end;
    extern void *__non_cache_heap_start;
    extern void *__non_cache_heap_end;
    AMP_MW_INIT_CFG_s mwInitCfg = {0};

    UT_PLLInit();

    memset (&G_MMPL, 0, sizeof(G_MMPL));
    memset (&G_NC_MMPL, 0, sizeof(G_MMPL));

    CacheDdrStart = (UINT8*) &__cache_heap_start;
    CacheDdrLimit = (UINT8*) &__cache_heap_end-1;

    NonCacheDdrStart = (UINT8*) &__non_cache_heap_start;
    NonCacheDdrLimit = (UINT8*) &__non_cache_heap_end-1;


    // DSP Working Buffer is cached
    DspWorkAreaResvStart = (UINT8*) ALIGN_64((UINT32)CacheDdrStart);
    DspWorkAreaResvLimit = DspWorkAreaResvStart + AmpUT_GetProjectDspMaxSize() - 1;
    DspWorkAreaResvSize = AmpUT_GetProjectDspMaxSize();
    AmbaKAL_BytePoolCreate(&G_MMPL, DspWorkAreaResvLimit + 1, (UINT32)CacheDdrLimit - (UINT32)DspWorkAreaResvLimit);
    AmbaKAL_BytePoolCreate(&G_NC_MMPL, NonCacheDdrStart, (UINT32)NonCacheDdrLimit - (UINT32)NonCacheDdrStart + 1);

#ifdef CONFIG_CC_CXX_SUPPORT
    AmbaCtorDtor_Init(&G_MMPL);
#endif

#ifndef BSP_B5_MULTICHAN
    AmbaUserExpGPIO_Init(); /* Initialize all Expanded GPIOs (after OS running) */
#endif

    // enable uart
    UT_UARTInit();

    UT_StorageInit();

    UT_MWInit(&mwInitCfg);

    // Init test module
    AmbaTest_Init();
    //AmbaTest_FioAddCommands();
    //AmbaTest_PioAddCommands();

    /* Set SD_DRIVING_STRENGTH to 12MA. */
#ifdef CONFIG_SOC_A9
    for (UINT8 Id = 0; Id < MAX_SD_HOST; Id++)
        AmbaSD_SetDrivingStrength(Id, AMBA_SD_DRIVING_STRENGTH_ALL, AMBA_SD_DRIVING_STRENGTH_12MA);
    AmbaSD_SetDelayControl(SD_HOST_0 , AMBA_SD_DATA_INPUT_DELAY, 4);
    AmbaSD_SetDelayControl(SD_HOST_0 , AMBA_SD_CLK_INPUT_DELAY , 5);
#endif

    UT_AmbaLinkInit();

    // Init SD card insertion monitor
    AmbaMonitor_SDInit(150, 0);

    AmbaRTC_Init();   /**< Init RTC */
    AmbaGDMA_Init();  /**< Init GDMA */
#ifdef BSP_B5_MULTICHAN
   //AmbaB5_PllInit(24000000, 0, 0, 0, 0);  /**< Init B5 PLL */
   //AmbaB5_PllInit(24000000, 24000000, 24000000, 24000000, 24000000);  // For ov10640
#endif

    UT_FifoInit();   /**< Init FIFO */
    UT_AudioInit();  /**< Init Audio */

    UT_HookTestFunc();

    AmbaPrint("DspLog @ 0x%X", mwInitCfg.DspDbgLogDataArea);
    AmbaPrint("CACHE    : 0x%08x - 0x%08x, %dMB", (UINT32)CacheDdrStart, (UINT32)CacheDdrLimit, ((UINT32)CacheDdrLimit - (UINT32)CacheDdrStart + 1)>>20);
    AmbaPrint("NON-CACHE: 0x%08x - 0x%08x, %dMB ", (UINT32)NonCacheDdrStart, (UINT32)NonCacheDdrLimit, ((UINT32)NonCacheDdrLimit - (UINT32)NonCacheDdrStart+1)>>20);
    AmbaPrint("G_MMPL.id: %x, Start %x Size %dMB", G_MMPL.tx_byte_pool_id,DspWorkAreaResvLimit + 1,((UINT32)CacheDdrLimit - (UINT32)DspWorkAreaResvLimit)>>20);
    AmbaPrint("G_NC_MMPL.id: %x Start %x Size %dMB", G_NC_MMPL.tx_byte_pool_id,NonCacheDdrStart, ((UINT32)NonCacheDdrLimit - (UINT32)NonCacheDdrStart + 1)>>20);

    AmbaPrint("---------------------------------------------");
    AmbaPrint("BSP: %s",BSPNAME);
    AmbaPrint("---------------------------------------------");
    AmbaPrint("Cortex freq:\t\t%d", AmbaPLL_GetCortexClk());
#ifdef CONFIG_SOC_A9
    AmbaPrint("ARM freq:\t\t%d", AmbaPLL_GetArmClk());
#endif
    AmbaPrint("DDR freq:\t\t%d", AmbaPLL_GetDdrClk());
    AmbaPrint("IDSP freq:\t\t%d", AmbaPLL_GetIdspClk());
    AmbaPrint("Core freq:\t\t%d", AmbaPLL_GetCoreClk());
    AmbaPrint("AXI freq:\t\t%d", AmbaPLL_GetAxiClk());
    AmbaPrint("AHB freq:\t\t%d", AmbaPLL_GetAhbClk());
    AmbaPrint("APB freq:\t\t%d", AmbaPLL_GetApbClk());
    AmbaPrint("---------------------------------------------");

    AmbaPrint("----- Version info --------------------------");
    AmbaVer_ShowAllVerCompactInfo();
    AmbaVer_ShowLinkVerInfo();
    AmbaPrint("---------------------------------------------");

    UT_ShellInit();

    /* Init storage 'a' drive */
    UT_InitADrive();
    {
        extern int AmbaLoadDSPuCode(void);
        AmbaLoadDSPuCode();
    }
    {
	    extern UINT32 AmbaSspUt_Soc_SetMemoryPool(TX_BYTE_POOL *cached_pool, TX_BYTE_POOL *noncached_pool) __attribute__((weak));
		if(AmbaSspUt_Soc_SetMemoryPool)
		    AmbaSspUt_Soc_SetMemoryPool(&G_MMPL, &G_NC_MMPL);
	}
}

void AmpSystemInit(void)
{
    /* Create task */
    AmbaKAL_TaskCreate(&SystemInitTask, /* pTask */
    "AmpSystemInit", /* pTaskName */
    200, /* Priority */
    SystemInitTaskEntry, /* void (*EntryFunction)(UINT32) */
    0x0, /* EntryArg */
    (void *) SystemInitTaskStack, /* pStackBase */
    SystemInitTaskStackSize, /* StackByteSize */
    AMBA_KAL_AUTO_START); /* AutoStart */
}

/**
 * User defined System Initializations (after OS running)
 */
void AmbaUserSysInitPostOS(void)
{
    AmpSystemInit();
}
