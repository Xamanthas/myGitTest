/**
 * @file mw/unittest/extSrc/sample_calib/AmbaUTCalibMgr.h
 *
 * Header file for Calibration Manager control
 *
 * History:
 *    07/10/2013  Allen Chiu Created
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __CALIB_MGR_H__
#define __CALIB_MGR_H__

#include "AmbaDataType.h"
#include "AmbaCalib_VIG.h"
#include "AmbaImg_AeAwb.h"
#include "AmbaImg_AaaDef.h"
#include "AmbaImg_Impl_Cmd.h"
#include "AmbaFS.h"
#include <recorder/VideoEnc.h>
#include <recorder/StillEnc.h>
#include "AmbaImg_Impl_Cmd.h"
#include "AmbaFS.h"
#include "AmbaCardManager.h"


#define APP_CAL_DBG
#ifdef APP_CAL_DBG
#define CAL_PRINT        AmbaPrint
#else
#define CAL_PRINT(...)
#endif

//projectID
#ifdef CHIP_REV_A9
#define CALIB_PROJECT_NAME "A9_SDK"
#endif
#ifdef CONFIG_SOC_A12
#define CALIB_PROJECT_NAME "A12_SDK"
#endif

#define CALIB_CH_ALL 0xFF

#define MAX_VIGNETTE_ROM_TABLE_COUNT 0
#define MAX_VIGNETTE_NAND_TABLE_COUNT 10
#define MAX_VIGNETTE_GAIN_TABLE_COUNT (MAX_VIGNETTE_NAND_TABLE_COUNT + MAX_VIGNETTE_ROM_TABLE_COUNT)
#define VIGNETTE_STORAGE_BLEND_NUM_MAX (MAX_VIGNETTE_GAIN_TABLE_COUNT)

#define CALIB_PARTIAL_LOAD 0
#define CALIB_FULL_LOAD 0xFF

//Total xx bytes
typedef struct Vignette_Global_Map_Config_Info_s_ {
    UINT8  NumVignetteTable;
    //UINT16 BlendRatio[VIGNETTE_STORAGE_BLEND_NUM_MAX]; //0~256
    UINT16 BlendRatio;
    UINT8 Id[2];
    UINT8 Padding[3];
    UINT32 Strength;
    Vignette_Pack_Storage_s *VignetteStorageAddress[VIGNETTE_STORAGE_BLEND_NUM_MAX];
} Vignette_Global_Map_Config_Info_s;


//#define CALIB_CH_NO 5 //for B5 config
#define CALIB_CH_NO 1 //for single chan
#define SITE_NUM (15)



/* Define Calibration source */
#define CALIB_SOURCE_NAND (0)
#define CALIB_SOURCE_ROMFS (1)
#define CALIB_SOURCE_BOTH (2)
#define CALIB_SOURCE_SDCard (3)

/*Calibration load data format*/

#define CAL_STATUS_LOAD_FORMAT         (CALIB_SOURCE_NAND)
#define CAL_AF_LOAD_FORMAT          (CALIB_SOURCE_NAND)
#define CAL_GYRO_LOAD_FORMAT        (CALIB_SOURCE_NAND)
#define CAL_MSHUTTER_LOAD_FORMAT    (CALIB_SOURCE_NAND)
#define CAL_IRIS_LOAD_FORMAT          (CALIB_SOURCE_NAND)
#define CAL_VIGNETTE_LOAD_FORMAT    (CALIB_SOURCE_NAND)
#define CAL_FPN_LOAD_FORMAT          (CALIB_SOURCE_NAND)
#define CAL_WB_LOAD_FORMAT          (CALIB_SOURCE_NAND)
#define CAL_ISO_LOAD_FORMAT          (CALIB_SOURCE_NAND)
#define CAL_BLC_LOAD_FORMAT          (CALIB_SOURCE_NAND)
#define CAL_FLASH_LOAD_FORMAT          (CALIB_SOURCE_NAND)
#define CAL_AUDIO_LOAD_FORMAT          (CALIB_SOURCE_NAND)
#define CAL_WARP_LOAD_FORMAT          (CALIB_SOURCE_ROMFS)
#define CAL_CA_LOAD_FORMAT             (CALIB_SOURCE_NAND)
#define CAL_LENSSHIFT_LOAD_FORMAT    (CALIB_SOURCE_NAND)

/* ---------- */
// ID, SIZE, VER of each calibration item

#define CAL_STATUS_ID                   (0)
#define CAL_STATUS_SIZE                 (4096)
#define CAL_STATUS_VER                  (0)

#define CAL_AF_ID                       (1)
#define CAL_AF_SIZE                     (2048)
#define CAL_AF_VER                      (0x20130730)

#define CAL_GYRO_ID                     (2)
#define CAL_GYRO_SIZE                   (512)
#define CAL_GYRO_VER                    (0x20130730)

#define CAL_MSHUTTER_ID                 (3)
#define CAL_MSHUTTER_SIZE               (512)
#define CAL_MSHUTTER_VER                (0x20130730)

#define CAL_IRIS_ID                     (4)
#define CAL_IRIS_SIZE                   (512)
#define CAL_IRIS_VER                    (0x20130730)

#define CAL_VIGNETTE_ID                 (5)
#define CAL_VIGNETTE_SIZE               (17*1024*MAX_VIGNETTE_GAIN_TABLE_COUNT)
#define CAL_VIGNETTE_VER                AMBA_DSP_IMG_VIG_VER_1_0

#define CAL_WARP_ID                     (6)
#define CAL_WARP_SIZE                   (128*1024)
#define CAL_WARP_VER                    AMBA_DSP_IMG_WARP_VER_1_0

#define CAL_BPC_ID                      (7)
#define CAL_BPC_SIZE                    (512*1024)
#define CAL_BPC_VER                     AMBA_DSP_IMG_SBP_VER_1_0

#define CAL_WB_ID                       (8)
#define CAL_WB_SIZE                     (512)
#define CAL_WB_VER                      (0x20130730)

#define CAL_ISO_ID                      (9)
#define CAL_ISO_SIZE                    (512)
#define CAL_ISO_VER                     (0x20130730)

#define CAL_BLC_ID                      (10)
#define CAL_BLC_SIZE                    (512)
#define CAL_BLC_VER                     (0x20130730)

#define CAL_FLASH_ID                    (11)
#define CAL_FLASH_SIZE                  (1024)
#define CAL_FLASH_VER                   (0x20130730)

#define CAL_AUDIO_ID                    (12)
#define CAL_AUDIO_SIZE                  (8*1024+512)
#define CAL_AUDIO_VER                   (0x20130730)

#define CAL_CA_ID                       (13)
#define CAL_CA_SIZE                     (64*1024)
#define CAL_CA_VER                      AMBA_DSP_IMG_CAWARP_VER_1_0

#define CAL_LENSSHIFT_ID                (14)
#define CAL_LENSSHIFT_SIZE              (512)
#define CAL_LENSSHIFT_VER               (0x20130730)



#define CAL_VIGNETTE_PARTIAL_LOAD_START_ID   (20)
#define CAL_VIGNETTE_PARTIAL_ID_(x)                 (CAL_VIGNETTE_PARTIAL_LOAD_START_ID+(x))

//define the storage interfance for calibration
#define CALIB_FROM_NAND 0
#define CALIB_FROM_SD_CARD 1
#define CALIB_FROM_ROM 2
#define CALIB_STORAGE  CALIB_FROM_NAND


#define CALIB_LOAD          0
#define CALIB_INIT       1
#define CALIB_POST_LOAD 100
#define CALIB_LOAD_ROM_AFT_NAND 200
// Define calibration partial load parameters
#define CALIB_TABLE_IDX_INIT    0
#define CALIB_TABLE_IDX_POST    1
#define CALIB_TABLE_COUNT_INIT    1
// Just a flag, it's not actually load 20 tables
#define CALIB_TABLE_COUNT_POST    0

/* ---------- */
// Calibration status definition

#define CAL_MAX_SUBSITE      (8)
#define NVD_CALIB_MAX_OBJS    32
typedef struct Cal_Site_Status_s_ {
    UINT32 Version;
    UINT32 Status; // two state: reset/ done
#define CAL_SITE_DONE_VIGNETTE_LOAD_ALL          (0x55FF55FF)
#define CAL_SITE_DONE_VIGNETTE_LOAD_PARTIAL      (0xAAFFAAFF)
#define CAL_VIGNETTE_SUB_SITE_DONE                (0x01)
#define CAL_SITE_DONE                   (0x55FF55FF)
#define CAL_SITE_RESET                  (0x00000000)
    UINT32 SubSiteStatus[CAL_MAX_SUBSITE]; // two state: reset/done
    UINT8  Reserved[88]; //for vignette partial save/load [0]:vignette enable [1]:tablecount [2...]:table0... dis/enable:0x00/0xFF
} Cal_Stie_Status_s; // 128byte per entry

typedef  struct Cal_Status_s_ {
    Cal_Stie_Status_s    Site[NVD_CALIB_MAX_OBJS];
}  Cal_Status_s; // 4096 byte


typedef struct App_Cal_Sensor_Status_s_ {
    UINT32     SensorReadoutMode;    // Related to DSP's phase correction, TBD
    UINT16     MainFps;
    UINT16     CurFps;
    UINT16     CurEshutterIndex;
    UINT8      SensorMode;
    UINT8      ActiveStillConfig;
    UINT8      BinningStatus;
#define    NON_BINNING     0
#define BINNING_2X      1
#define    BINNING_4X      3
#define    BINNING_8X      7
    UINT8      BinningMode;           // 0: All sensor mode is non-binning; 1: All sensor mode is binning, 2:Dont care binning_status
    UINT8      FpsSlowdownRatio;
    UINT8      DataBits;              // # of sensor data bits, 10 for Sony CCD
    UINT8      BayerPattern;          // RG, or BG, or GR, or GB
    UINT8      Reserved[3];
} App_Cal_Sensor_Status_s; // The structure's size is 32-byte


/* ---------- */
// Calibration manager table

typedef struct Cal_Obj_s_ {
    UINT32   Enable;
    UINT32   Id;
    UINT32   Size;
    UINT32   Version;
    char       Name[16];
    UINT8    *DramShadow;
    int (*InitFunc)(struct Cal_Obj_s_*);
    int (*UpgradeFunc)(struct Cal_Obj_s_*, struct Cal_Site_Status_s_*); // The function pointer to handle version mismatch
    int (*CalibFunc)(int Argc, char *Argv[], char *OutputStr, Cal_Stie_Status_s *CalSite, struct Cal_Obj_s_ *CalObj);
    UINT8    Reserved[3];
} Cal_Obj_s;



typedef struct Calib_Raw_Cap_Ctrl_s_ {
    AMBA_KAL_EVENT_FLAG_t   Flag;
} Calib_Raw_Cap_Ctrl_s;


extern Calib_Raw_Cap_Ctrl_s AmbaMonitorCalibRawCap;


extern Cal_Obj_s CalObjTable[NVD_CALIB_MAX_OBJS];



/* ---------- */
// Function prototype

// Calibration manager
extern Cal_Stie_Status_s* AmpUT_CalibGetSiteStatus(UINT32 CalId);
extern int sprintf(char *, const char *Fmt, ...);
extern int AmpUT_CalibInit(UINT32 CalId,UINT8 JobId);
extern Cal_Obj_s* AmpUT_CalibGetObj(UINT32 CalId);
extern int AmpUT_CalibSetExposureValue(float Shutter, float Agc, UINT32 Dgain);
extern int AmpUT_CalibMemReset(UINT32 CalId);
extern int AmpUT_MultiGetline(AMBA_FS_FILE *Fp, char Buf[]);
extern int AmpUT_CalibTableMapping(UINT8 Channel, UINT8 Id);
extern Cal_Obj_s* AmpUT_CalibGetNandStatus(UINT32 CalId,UINT32 *Offset, UINT32 *SectorNum);
extern int AmpUT_CalibNandReset(UINT32 CalId);
extern int AmpUT_CalibNandInit(void);
extern int AmpUT_CalibNandSaveTest(UINT32 Offset, UINT32 SecNum,UINT8 Value);
extern int AmpUT_CalibNandSaveTest1(UINT32 Offset, UINT32 SecNum,UINT8 Value);
int AmpUT_CalibCheckFlag(UINT8 CalId, UINT8 TableIdx);
int AmpUT_CalibCheckHeadFlag(UINT8 CalId);
int AmpUT_CalibMarkFlag(UINT8 CalId, UINT8 TableIdx, UINT8 Flag);
int AmpUT_CalibMarkHeadFlag(UINT8 CalId, UINT8 Flag);
void AmpUT_CalibRestCheckAllFlag(void);
int AmpUT_CalibMgrRawCapture(UINT16 SensorMode, UINT16 Flip);
extern int AmpUtil_GetAlignedPool(AMBA_KAL_BYTE_POOL_t * BytePool, void ** AlignedPool, void ** Pool, UINT32 Size, UINT32 Alignment);
extern int AmpUT_ItunerRawCapture(UINT8 OutputFlag, AMP_STILLENC_ITUNER_RAWCAPTURE_CTRL_s ItunerRawCapCtrl)  __attribute__((weak));;
extern int AmbaUT_CalibRotateMap( UINT16 *Map, UINT16 MapWidth, UINT16 MapHeight, UINT8 Rotate);
extern char AmpUT_CalibGetDriverLetter(void);
extern int AmpUT_DummyProcCmd(UINT32 cmd, UINT32 param1, UINT32 param2, UINT32 param3);
extern void AmpCalib_SetOpticalBlackFlag(UINT8 *OpticalBlackEnable);
extern UINT8 AmpCalib_GetOpticalBlackFlag(void);
extern void AmpUT_CalibRestCheckFlag(UINT8 CalId);
extern int AmpUT_CalibInitFunc(UINT32 CalId,UINT8 JobId, UINT8 SubId);
#endif //__CALIB_MGR_H__
