/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaUserSysCtrl.c
 *
 *  @Copyright      :: Copyright (C) 2014 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: User defined System Control APIs (board dependent)
 *
 *  @History        ::
 *      Date        Name        Comments
 *      02/20/2014  W.Shi       Created
 *
\*-------------------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>

#include "AmbaDataType.h"
#include "AmbaRTSL_GPIO.h"

#include "AmbaSysCtrl.h"
#include "AmbaBuffers.h"

#include "AmbaPrint.h"
#include "AmbaShell.h"

#include "AmbaI2C.h"
#include "AmbaGPIO.h"
#include "AmbaUART.h"
#include "AmbaSD.h"
#include "AmbaNAND.h"
#include "AmbaNOR.h"
#include "AmbaFIO.h"
#include "AmbaROM.h"
#include "AmbaCardManager.h"
#include "AmbaNFTL.h"

#include "AmbaPLL_Def.h"
#include "AmbaPLL.h"
#include "AmbaVer.h"
#include "AmbaMonitor_SD.h"

#include "AmbaSSPSystemInfo.h"

#define AMBA_UART_RX_RING_BUF_SIZE  1024
static UINT8 AmbaUartRxRingBuf[AMBA_UART_RX_RING_BUF_SIZE] __attribute__((section (".bss.noinit"))) ;
static AMBA_KAL_TASK_t _AmbaSysInitTask;
static UINT8 _AmbaSysInitTaskStack[8192] __attribute__((section (".bss.noinit")));

extern int AmbaUserConsole_Read(int StringSize, char *StringBuf, UINT32 TimeOut);
extern int AmbaUserConsole_Write(int StringSize, char *StringBuf, UINT32 TimeOut);
extern void AmbaUserExpGPIO_Init(void);
extern int AmbaFwUpdaterMain(void);
extern void AmbaEMMC_Init(void);
extern void AmbaUserSD_Adjust(void);
void IdleFunction(void) {}

static void CallFwUpdaterMain(int slot, int eid)
{
    AmbaFwUpdaterMain();
}

void AmbaSysInitTaskEntry(UINT32 EntryArg)
{
    AMBA_UART_CONFIG_s  UartConfig = {0};
    AMBA_SHELL_CONFIG_s ShellConfig = {0};
    static char *AutoExecScripts[] = {
        "c:\\autoexec.ash",
        "d:\\autoexec.ash"
    };

    AmbaUserExpGPIO_Init();     /* Initialize all Expanded GPIOs (after OS running) */

    UartConfig.BaudRate = 115200;                               /* Baud Rate */
    UartConfig.DataBit  = AMBA_UART_DATA_8_BIT;                  /* number of data bits */
    UartConfig.Parity   = AMBA_UART_PARITY_NONE;                  /* parity */
    UartConfig.StopBit  = AMBA_UART_STOP_1_BIT;                  /* number of stop bits */
    UartConfig.FlowCtrl = AMBA_UART_FLOW_CTRL_NONE;             /* flow control */

    UartConfig.MaxRxRingBufSize = sizeof(AmbaUartRxRingBuf);    /* maximum receive ring-buffer size in Byte */
    UartConfig.pRxRingBuf = (UINT8 *) & (AmbaUartRxRingBuf[0]); /* pointer to the receive ring-buffer */
    AmbaUART_Config(AMBA_UART_CHANNEL0, &UartConfig);

    AmbaBufferInit();

    if (AmbaSSPSystemInfo.DspLogAddr == 0x0) {
        void *DspLogRawBuf = 0;
        AmbaKAL_BytePoolAllocate(&AmbaBytePool_NonCached, (void**) &DspLogRawBuf, AmbaSSPSystemInfo.DspLogSize + 63, 100);
        AmbaSSPSystemInfo.DspLogAddr = (UINT8*) ALIGN_64((UINT32)DspLogRawBuf);
    }

    {
        /* The buffer is used to queue characters sent through AmbaPrint */
        static UINT8 AmbaPrintRingBuf[128 * 1024] __attribute__((section(".bss.noinit")));

        static AMBA_PRINT_CONFIG_s AmbaPrintConfig = {
            .Priority             = AMBA_KAL_TASK_LOWEST_PRIORITY - 1,
            .PrintBufSize         = sizeof(AmbaPrintRingBuf),
            .pPrintBuf            = (UINT8 *) & (AmbaPrintRingBuf[0]),
            .DspDebugLogAreaSize  = 0,
            .pDspDebugLogDataArea = NULL,
            .PutCharCallBack      = AmbaUserConsole_Write
        };
        AmbaPrintConfig.Priority = AMBA_KAL_TASK_LOWEST_PRIORITY - 1;
        AmbaPrintConfig.PrintBufSize = sizeof(AmbaPrintRingBuf);
        AmbaPrintConfig.pPrintBuf = (UINT8 *) & (AmbaPrintRingBuf[0]);
        AmbaPrintConfig.DspDebugLogAreaSize = AmbaSSPSystemInfo.DspLogSize;
        AmbaPrintConfig.pDspDebugLogDataArea = AmbaSSPSystemInfo.DspLogAddr;
        AmbaPrintConfig.PutCharCallBack = AmbaUserConsole_Write;

        AmbaPrint_Init(&AmbaPrintConfig);
    }

    AmbaVer_Init();
    AmbaVer_ShowLinkVerInfo();

    AmbaFIO_Init();                     /* FIO Initializations */
    AmbaFS_Init(AMBA_FS_ASCII);         /* FS  Initializations */
    AmbaSCM_Init();
#ifdef CONFIG_ENABLE_EMMC_BOOT
    AmbaEMMC_Init();
    AmbaFwUpdaterMain();
#else
    AmbaSD_Init(2, SD_UHS_MAX_FREQ);    /* SD  Initializations */
    AmbaUserSD_Adjust();
    AmbaSCM_Register(&CallFwUpdaterMain);

#if defined(CONFIG_ENABLE_SPINOR_BOOT)
    {
        extern AMBA_NOR_DEV_INFO_s AmbaNOR_DevInfo;
        AmbaNOR_Init(&AmbaNOR_DevInfo);   /* NOR Initializations */
    }
#else
    {
        int Id;
        extern AMBA_NAND_DEV_INFO_s AmbaNAND_DevInfo;
        AmbaNAND_Init(&AmbaNAND_DevInfo); /* NAND Initializations */

        for (Id = 0; Id < NFTL_MAX_INSTANCE; Id ++) {
            AmbaNFTL_InitLock(Id);
            AmbaNFTL_Init(Id, NFTL_MODE_NO_SAVE_TRL_TBL);
        }
    }
#endif
#endif

    AmbaMonitor_SDInit(150, 0);

    /* Initialize HEAP for Shell */
    ShellConfig.Priority         = 160;
    ShellConfig.pCachedHeap      = &AmbaBytePool_Cached;
    ShellConfig.ConsolePutCharCb = AmbaUserConsole_Write;
    ShellConfig.ConsoleGetCharCb = AmbaUserConsole_Read;
    AmbaShell_Init(&ShellConfig);
    AmbaShell_Start(GetArraySize(AutoExecScripts), AutoExecScripts);

    /* register User Idle function */
    AmbaIdleTaskCallbackFunctionRegister(IdleFunction);

    AmbaPrint("----- Version info --------------------------");
    AmbaVer_ShowAllVerCompactInfo();
    AmbaVer_ShowLinkVerInfo();
    AmbaPrint("---------------------------------------------");
}
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

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaUserSysInitPostOS
 *
 *  @Description:: User defined System Initializations (after OS running)
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
void AmbaUserSysInitPostOS(void)
{
    if (AmbaKAL_TaskCreate(&_AmbaSysInitTask,                      /* pTask */
                           "AmbaSysInit",                          /* pTaskName */
                           AMBA_KAL_TASK_LOWEST_PRIORITY - 5,      /* Priority */
                           AmbaSysInitTaskEntry,                   /* void (*EntryFunction)(UINT32) */
                           0x0,                                    /* EntryArg */
                           (void *) & (_AmbaSysInitTaskStack[0]),  /* pStackBase */
                           sizeof(_AmbaSysInitTaskStack),          /* StackByteSize */
                           AMBA_KAL_AUTO_START) != 0)              /* AutoStart */

    {
        return;     /* should never happen ! */
    }
}

