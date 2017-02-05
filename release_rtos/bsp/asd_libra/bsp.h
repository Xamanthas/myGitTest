 /**
  * @file bsp.h
  *
  * BSP header
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

#ifndef _BSP_H_
#define _BSP_H_

#define BSPNAME "ASD LIBRA"

#ifdef CONFIG_ENABLE_SPINOR_BOOT
/**
 * SPINOR flash partition map
 */

/* Firmware (System code) partitions. */
#define AMBA_BOOTSTRAPE_SIZE             2048               /* 256KB */
#define AMBA_PARTITION_TABLE_SIZE       (256 * 1024)        /* 256KB */
#define AMBA_BOOTLOADER_SIZE            (256 * 1024)        /* 256KB */
#define AMBA_SD_FWUPDATE_SIZE           (2 * 1024 * 1024)  /* 2MB */
#define AMBA_SYSTEM_SOFTWARE_SIZE       (5 * 512 * 1024)  /* 2.5MB */

/* Firmware (Read-Only, uCode and ROM) partitions. */
#define AMBA_DSP_SIZE                   (2 * 1024 * 1024)    /* 2MB */
#define AMBA_ROM_SIZE                   (1 * 1024 * 1024)   /* 1MB */

/* Firmware (Linux) partitions. */
#define AMBA_LINUX_SIZE                  0   		     /* 0MB */
#define AMBA_ROOTFS_SIZE                 0                   /* 0MB */
#define AMBA_HIBERNATION_SIZE            0                   /* Not used */

/* Media partitions. Created after firmware programming. */
#define MP_STORAGE1_SIZE                 0                   /* Remain blocks are allocated to this partition. */
#define MP_STORAGE2_SIZE                 0                   /* Not used */
#define MP_INDEX_SIZE                    (0 * 1024 * 1024)   /* 0MB */
#define MP_PREFERENCE_SIZE               (256 * 1024)        /* 256KB */
#define MP_CALIBRATION_SIZE              (1024 * 1024)        /* 512KB */
#else
/**
 * NAND flash partition map
 */

/* Firmware (System code) partitions. */
#define AMBA_BOOTSTRAPE_SIZE             2048               /* 2KB */
#define AMBA_PARTITION_TABLE_SIZE       (256 * 1024)        /* 256KB */
#define AMBA_BOOTLOADER_SIZE            (256 * 1024)        /* 256KB */
#define AMBA_SD_FWUPDATE_SIZE           ( 2 * 1024 * 1024)  /* 2MB */
#define AMBA_SYSTEM_SOFTWARE_SIZE       (32 * 1024 * 1024)  /* 32MB */

/* Firmware (Read-Only, uCode and ROM) partitions. */
#define AMBA_DSP_SIZE                   (5 * 1024 * 1024)    /* 5MB */
#ifdef CONFIG_BUILD_AMBA_MW_UNITTEST
#define AMBA_ROM_SIZE                   (24 * 1024 * 1024)   /* 24MB */
#else
#define AMBA_ROM_SIZE                   (14 * 1024 * 1024)   /* 14MB */
#endif

/* Firmware (Linux) partitions. */
#define AMBA_LINUX_SIZE                  (10 * 1024 * 1024)   /* 10MB */
#define AMBA_ROOTFS_SIZE                 (60 * 1024 *  1024)  /* 120MB */
#define AMBA_HIBERNATION_SIZE            (30 * 1024 * 1024)                   /* Not used */

/* Media partitions. Created after firmware programming. */
#define MP_STORAGE1_SIZE                 0                   /* Remain blocks are allocated to this partition. */
#define MP_STORAGE2_SIZE                 0                   /* Not used */
#define MP_INDEX_SIZE                    (12 * 1024 * 1024)  /* 12MB */
#define MP_PREFERENCE_SIZE               (256 * 1024)        /* 256KB */
#define MP_CALIBRATION_SIZE              (1024 * 1024)        /* 512KB */
#endif

//#define GPIO_COMPOSITE  GPIO_PIN_10

/* SD dealy ctrl settings*/
#define ENABLE_SD_DLY_CTRL  0
#define SD_DLY_CTRL_RND_DLY 2240     /* 2240/1024 ns */
#define SD_DLY_CTRL_CLK_SPEED 100000 /* 100000 Khz */

/* Spi Slave id */
#define AMBA_SENSOR_SPI_SLAVE_ID    1

/* Parallel H/V sync pin select */
#define AMBA_SENSOR_PARALLEL_VSYNC_PIN_SEL  15
#define AMBA_SENSOR_PARALLEL_HSYNC_PIN_SEL  14

/* APIs */
void AmbaUserExpGPIO_Init(void);
void AmbaUserGPIO_SdPowerCtrl(UINT32 EnableFlag);
void AmbaUserGPIO_SdioPowerCtrl(UINT32 EnableFlag);
void AmbaUserGPIO_PhyResetCtrl(UINT32 ResetFlag);
void AmbaUserGPIO_AudioResetCtrl(UINT32 ResetFlag);
void AmbaUserGPIO_FlashCtrl(UINT32 EnableFlag);
void AmbaUserGPIO_LedCtrl(UINT32 LedFlag);
void AmbaUserGPIO_SensorResetCtrl(UINT32 ResetFlag);
void AmbaUserGPIO_UsbHubResetCtrl(UINT32 ResetFlag);
void AmbaUserGPIO_UsbModeSelect(UINT32 Flag);
void AmbaUserGPIO_LcdReset(void);
void AmbaUserGPIO_LcdCtrl(UINT32 LcdFlag);
void AmbaUserGPIO_FlashLightCtrl(UINT32 StrobeFlag);
void AmbaUserAudioCodec_Init(void);
void AmbaUserGPIO_FlashLightCtrl(UINT32 StrobeFlag);
void AmbaUserSD_VoltageCtrl(UINT32 Level);
void AmbaUserSD_PowerCtrl(UINT32 Enable);
void AmbaUserSD_Adjust(void);
void AmbaUserSDIO_VoltageCtrl(UINT32 Level);
void AmbaUserSDIO_PowerCtrl(UINT32 Enable);
int AmbaUserConsole_Read(int StringSize, char *StringBuf, UINT32 TimeOut);
int AmbaUserConsole_Write(int StringSize, char *StringBuf, UINT32 TimeOut);
void AmbaUserSetCdcAcmRunning(UINT8 value);
int AmbaUser_CheckIsUsbSlave(void);

#endif
