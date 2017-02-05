 /**
  * @file bsp.c
  *
  * BSP
  *
  * History:
  *    2014/04/09 - [Peter Weng] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#include <stdio.h>
#include <string.h>

#include "AmbaDataType.h"
#include "AmbaKAL.h"
#include "bsp.h"
#include "AmbaSD_Def.h"
#include "AmbaNAND_Def.h"
#include "AmbaGPIO.h"
#include "AmbaUART.h"
#include "AmbaAudio_CODEC.h"
#include "AmbaADC.h"


/** GPIO config */
AMBA_GPIO_PIN_GROUP_CONFIG_s GpioPinGrpConfig = {
    .PinGroup[AMBA_GPIO_GROUP0] = {
        .Function = {
            0x7800001f,
            0x00078780,
            0x00000000
        },
        .Direction = 0x81e05800,
        .Mask = 0xffffffff,
        .State = 0x01c01800,
        .PullEnable = 0x00000000,
        .PullSelect = 0x00000000,
        .DriveStrength = {0xffffffff, 0x0}
    },
    .PinGroup[AMBA_GPIO_GROUP1] = {
        .Function = {
            0x00003ffc,
            0xffc00000,
            0x00000000
       },
        .Direction = 0x00000000,
        .Mask = 0xffffffff,
        .State = 0x00000000,
        .PullEnable = 0x00000000,
        .PullSelect = 0x00000000,
        .DriveStrength = {0xffffffff, 0x0}
    },
#ifdef CONFIG_ENABLE_EMMC_BOOT
    .PinGroup[AMBA_GPIO_GROUP2] = {
        .Function = {
            0xfe000000,
            0x01ffffff,
            0x00000000
        },
        .Direction = 0x00000000,
        .Mask = 0xffffffff,
        .State = 0x00000000,
        .PullEnable = 0x00000000,
        .PullSelect = 0x00000000,
        .DriveStrength = {0xffffffff, 0x0}
    },
#else
    .PinGroup[AMBA_GPIO_GROUP2] = {
        .Function = {
            0xfe000000,
            0x01fe1fff,
            0x00000000
        },
        .Direction = 0x00000000,
        .Mask = 0xffffffff,
        .State = 0x00000000,
        .PullEnable = 0x00000000,
        .PullSelect = 0x00000000,
        .DriveStrength = {0xffffffff, 0x0}
    },
#endif
    .PinGroup[AMBA_GPIO_GROUP3] = {
        .Function = {
            0x0003ffff,
            0x00000000,
            0x00000000
        },
        .Direction = 0x00000000,
        .Mask = 0xffffffff,
        .State = 0x00000000,
        .DriveStrength = {0xffffffff, 0x0}
    },
};


const int AmbaFW_PartitionSize[AMBA_NUM_FW_PARTITION] = {
    /* 2KB fixed size. Can not be changed. */
    [AMBA_PARTITION_BOOTSTRAP]                    = AMBA_BOOTSTRAPE_SIZE,

    /* Firmware (System code) partitions */
    [AMBA_PARTITION_TABLE]                        = AMBA_PARTITION_TABLE_SIZE,
    [AMBA_PARTITION_BOOTLOADER]                   = AMBA_BOOTLOADER_SIZE,
    [AMBA_PARTITION_SD_FIRMWARE_UPDATE]           = AMBA_SD_FWUPDATE_SIZE,
    [AMBA_PARTITION_SYS_SOFTWARE]                 = AMBA_SYSTEM_SOFTWARE_SIZE,

    /* Firmware (Read-Only, uCode and ROM) partitions */
    [AMBA_PARTITION_DSP_uCODE]                    = AMBA_DSP_SIZE,
    [AMBA_PARTITION_SYS_ROM_DATA]                 = AMBA_ROM_SIZE,

    [AMBA_PARTITION_LINUX_KERNEL]                 = AMBA_LINUX_SIZE,
    [AMBA_PARTITION_LINUX_ROOT_FS]                = AMBA_ROOTFS_SIZE,
    [AMBA_PARTITION_LINUX_HIBERNATION_IMAGE]      = AMBA_HIBERNATION_SIZE,

    /* Media partitions */
    [AMBA_PARTITION_STORAGE0]                     = MP_STORAGE1_SIZE,
    [AMBA_PARTITION_STORAGE1]                     = MP_STORAGE2_SIZE,
    [AMBA_PARTITION_VIDEO_REC_INDEX]              = MP_INDEX_SIZE,
    [AMBA_PARTITION_USER_SETTING]                 = MP_PREFERENCE_SIZE,
    [AMBA_PARTITION_CALIBRATION_DATA]             = MP_CALIBRATION_SIZE,
};

/**
 * User defined idle function
 *
 * @return none
 */
void AmbaUserIdleFunction(void)
{
#if 0
    AmbaUserGPIO_LedCtrl(1);
    AmbaSysTimer_Wait(1000, 1000);   /* wait for a while */
    AmbaUserGPIO_LedCtrl(0);
#else
    static UINT32 LedFlag = 0;

    LedFlag ^= 1;
    AmbaUserGPIO_LedCtrl(LedFlag);
#endif
}

/**
 * User defined external GPIO initialization
 *
 * @return none
 */
void AmbaUserExpGPIO_Init(void)
{
}

/**
 * LED control
 *
 * @param [in] LedFlag Led Flag
 *
 * @return none
 */
void AmbaUserGPIO_LedCtrl(UINT32 LedFlag)
{
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaUserGPIO_AudioResetCtrl
 *
 *  @Description:: Audio Codec reset control
 *
 *  @Input      ::
 *          ResetFlag: 1 = Audio Codec reset, 0 = Active
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
void AmbaUserGPIO_AudioResetCtrl(UINT32 ResetFlag)
{
}
/**
 * Sensor Reset Control
 *
 * @param [in] ResetFlag Sensor reset
 *
 * @return none
 */
void AmbaUserGPIO_SensorResetCtrl(UINT32 ResetFlag)
{
    if(ResetFlag==0)
        AmbaGPIO_ConfigOutput(GPIO_PIN_21, AMBA_GPIO_LEVEL_LOW);
    else if(ResetFlag==1)
        AmbaGPIO_ConfigOutput(GPIO_PIN_21, AMBA_GPIO_LEVEL_HIGH);
}

/**
 * SD power control
 *
 * @param [in] EnableFlag Enable SD power
 *
 * @return none
 */
void AmbaUserGPIO_SDPowerCtrl(UINT32 EnableFlag)
{
}

/**
 * SD voltage
 *
 * @param [in] Level Level
 *
 * @return none
 */
void AmbaUserSD_VoltageCtrl(UINT32 Level)
{
    AMBA_GPIO_PIN_LEVEL_e GpioLevel = (Level ==  SD_POWER_CTRL_33V) ?  AMBA_GPIO_LEVEL_LOW :
                                      (Level ==  SD_POWER_CTRL_18V) ?  AMBA_GPIO_LEVEL_HIGH :
                                      AMBA_GPIO_LEVEL_LOW;
    AmbaRTSL_GpioConfigOutput(GPIO_PIN_0, GpioLevel);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaUserSD_PowerCtrl
 *
 *  @Description::
 *
 *  @Input      ::
 *      Enable:
 *      Level :
 *
 *  @Output     :: none
 *
 *  @Return     :: none
\*-----------------------------------------------------------------------------------------------*/
void AmbaUserSD_PowerCtrl(UINT32 Enable)
{
#ifndef CONFIG_ENABLE_EMMC_BOOT
    if(Enable==0)
        AmbaGPIO_ConfigOutput(GPIO_PIN_23, AMBA_GPIO_LEVEL_LOW);
    else if(Enable==1)
        AmbaGPIO_ConfigOutput(GPIO_PIN_23, AMBA_GPIO_LEVEL_HIGH);
#endif
}

/**
 * LCD backlight control
 *
 * @param [in] LcdFlag LCD flag
 *
 * @return none
 */
void AmbaUserGPIO_LcdCtrl(UINT32 LcdFlag)
{
    if (LcdFlag == 0)
        AmbaGPIO_ConfigOutput(GPIO_PIN_113, AMBA_GPIO_LEVEL_LOW);
    else
        AmbaGPIO_ConfigOutput(GPIO_PIN_113, AMBA_GPIO_LEVEL_HIGH);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaUserConsole_Write
 *
 *  @Description:: Write string to console
 *
 *  @Input      ::
 *      StringSize: Number of characters
 *      StringBuf:  Character buffer
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int :   Number of written characters
\*-----------------------------------------------------------------------------------------------*/
int AmbaUserConsole_Write(int StringSize, char *StringBuf, UINT32 TimeOut)
{
    AmbaUART_Write(AMBA_UART_CHANNEL0, StringSize, (UINT8 *)StringBuf, TimeOut);
    return StringSize;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaUserConsole_Read
 *
 *  @Description:: Read string from console
 *
 *  @Input      ::
 *      StringSize: Number of characters
 *      StringBuf:  Character buffer
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *          int :   Number of read characters
\*-----------------------------------------------------------------------------------------------*/
int AmbaUserConsole_Read(int StringSize, char *StringBuf, UINT32 TimeOut)
{
    AMBA_UART_CHANNEL_e UartChanNo = AMBA_UART_CHANNEL0;
    return AmbaUART_Read(UartChanNo, StringSize, (UINT8 *)StringBuf, TimeOut);
}

/**
 * Audio codec initialization
 *
 * @return none
 */
void AmbaUserAudioCodec_Init(void)
{
    AmbaPrint("AmbaUserAudioCodec_Init");
    AmbaPrint("AmbaAudio_CodecInit start");
    AmbaAudio_CodecInit(AMBA_AUDIO_CODEC_0);
    AmbaPrint("AmbaAudio_CodecInit end");
    AmbaAudio_CodecModeConfig(AMBA_AUDIO_CODEC_0, I2S);
    AmbaAudio_CodecFreqConfig(AMBA_AUDIO_CODEC_0, 48000);
    AmbaAudio_CodecSetInput(AMBA_AUDIO_CODEC_0, AMBA_AUDIO_CODEC_MIC_IN);
    AmbaAudio_CodecSetOutput(AMBA_AUDIO_CODEC_0, AMBA_AUDIO_CODEC_SPEAKER_OUT);
    AmbaAudio_CodecSetMute(AMBA_AUDIO_CODEC_0, 0);
}

/**
 *  Get version ID from adc
 *
 *  @return Bsp version ID
 */
int AmbaUser_GetVersion(void)
{
    static int BspVersionID = -1;

    if (BspVersionID < 0) {
        UINT32 AdcValues = 0;

        /*Get the ADC data.*/
        AmbaADC_DataCollection();
        AdcValues = AmbaAdcData[3];

        if ((AdcValues >= 2971) && (AdcValues <= 3171)) {
            BspVersionID = AMBA_BSP_VERSION_V11A_10V;
        } else if ((AdcValues >= 239) && (AdcValues <= 439)) {
            BspVersionID = AMBA_BSP_VERSION_V11A_09V;
        } else if ((AdcValues >= 2693) && (AdcValues <= 2893)) {
            BspVersionID = AMBA_BSP_VERSION_V20A_10V;
        } else if ((AdcValues >= 1091) && (AdcValues <= 1291)) {
            BspVersionID = AMBA_BSP_VERSION_V20A_09V;
        } else if ((AdcValues >= 2460) && (AdcValues <= 2660)) {
            BspVersionID = AMBA_BSP_VERSION_V10B_10V;
        } else if ((AdcValues >= 1496) && (AdcValues <= 1696)) {
            BspVersionID = AMBA_BSP_VERSION_V10B_09V;
        } else {
            BspVersionID = AMBA_BSP_VERSION_V11A_10V;// default value
            AmbaPrint("Use default BSP version");
        }
    }

    AmbaPrint("BSP version: %d", BspVersionID);
    return BspVersionID;
}

/**
 * Check usb is slave or host
 *
 * @return 1 slave, 0 host
 */
int AmbaUser_CheckIsUsbSlave(void)
{
    return 1;
}

/**
 * set SD delay control
 */
void AmbaUserSD_Adjust(void)
{
#if (ENABLE_SD_DLY_CTRL == 1)
    AmbaSD_DelayCtrlReset();
    AmbaSD_DelayCtrlAdjustPhy(SD_DLY_CTRL_RND_DLY, 1024000000 / SD_DLY_CTRL_CLK_SPEED, 1, 0);
#endif
}

