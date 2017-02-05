/**
 * @file mw/unittest/extSrc/sample_calib/bpc/AmbaUTCalibBpc.h
 *
 * Header file for bad pixel calibration
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
#include "AmbaCalib_BPC.h"
#include "../AmbaUTCalibMgr.h"

// FPN_OP_UPDATE_MAP related parameter
#define BPC_UPDATE_MAP_NORMAL                   (0)
#define BPC_UPDATE_MAP_FAST                     (1)

#define BPC_CH_ALL                              0xFF
#define BPC_CH_NO                               1
#define BPC_MAP_INIT        0
#define BPC_MAP_UPDATE 1
#define BPC_TASK_PRIORITY          (75)         // Low priority task
#define BPC_TASK_LOW_PRIORITY      (90)         // Low priority task
#define BPC_MAP_PITCH           (32)    // DMA's 2D mode needs row size to be multiple of 32-byte
#define BPC_2CH_MIXED_MODE  (0xEFEF0000)

#define BPC_RECORD_BADPIXEL             (0x00000001)

#define BPC_FLIP_ON        1

#define BPC_MAX_MODES       (16)
#define BPC_MAX_BUFFER      (BPC_MAX_MODES*CALIB_CH_NO)
#define BPC_MAX_PAGES       (BPC_MAX_MODES<<1)

// NAND Flash storage structure
typedef struct BPC_Page_s_ {
    UINT32    PageValid;    // 1 when the record existed in NAND flash
    UINT32    PageSize;
    UINT32    Offset;
    UINT32    Type;
    UINT32  CheckSum;
#define BPC_RECORD_BADPIXEL             (0x00000001)
} BPC_Page_s;

typedef struct BPC_Nand_s_ {
#define BPC_NAND_RESERVED (256-(5*BPC_MAX_PAGES))
    BPC_Page_s        BPCPage[BPC_MAX_PAGES];
    UINT32          Reserved[BPC_NAND_RESERVED];
    void            *Data;
} BPC_Nand_s;

// BPC set (combination of pages)
typedef struct BPC_Set_s {
    INT16    Valid;
    INT16    BadPixelPage;
} BPC_Set_t;

// BPC buffer management structure
typedef struct BPC_Buffer_s_ {
    UINT32    MemoryAllocated;
    UINT8    *BufferAddr;
    UINT8   Channel;
    UINT8   BPCMode;
    UINT8   Resvered;
    UINT32    Size;
} BPC_Buffer_s;

typedef struct BPC_Buffer_Setup_s {
    UINT32    Enable;
    UINT32    BPCMode;
    UINT32    BPCPage;
    UINT32     ActiveBuffer;
    UINT32    Option;
} BPC_Buffer_Setup_t;


/* ---------- */
// BPC buffer content management (the content of each buffer (content=set), lock & valid status)
typedef struct BPC_Buffer_Status_s_ {
    // Following information is updated by BPC_buffer_setup_t
    int                BPCSet[BPC_MAX_BUFFER*CALIB_CH_NO];    // The buffer's current BPC set

    // Following infomation is BPC library internal holding information
    UINT32            Lock[BPC_MAX_BUFFER*CALIB_CH_NO];
    int                Valid[BPC_MAX_BUFFER*CALIB_CH_NO];        // =1 when buffer contain map
} BPC_Buffer_Status_s;

/* ---------- */
// Optical black Object
typedef struct BPC_OB_Offset_s_ {
    INT16   X;
    INT16   Y;
}BPC_OB_Offset_s;

/* ---------- */
// BPC Object

typedef struct BPC_Obj_s {
    // Mutex
    //	ID				BPCMutexId;
    // BPC related flag
    UINT32                BPCMemAllocFlag;
    UINT32                BPCInitialDecompressFlag;
    UINT32                BPCDebugEn;
    UINT32                BPCDynamicPageEn;
    UINT32                BPCS3dMode;
    // NAND Flash storage structure
    BPC_Nand_s            *BPCNand;

    // BPC buffer management structure
    BPC_Buffer_s        BPCBuffer[BPC_MAX_BUFFER];

    // BPC config (correlation between BPC buffer & BPC pages)
    BPC_Buffer_Status_s        BufferStatus;

    // Active buffer information of 1x/2x/4x map
    UINT32                ActiveBuffer[BPC_MAX_MODES*CALIB_CH_NO];

    // Geometry information of 1x/2x/4x map
    int                    GeometryValid[2][BPC_MAX_MODES*CALIB_CH_NO];
    AMBA_DSP_IMG_VIN_SENSOR_GEOMETRY_s EffectiveGeometry[2][BPC_MAX_MODES*CALIB_CH_NO];    //effective sensor geometry information by BPC_MODE.
    AMBA_DSP_IMG_VIN_SENSOR_GEOMETRY_s OBGeometry[2][BPC_MAX_MODES*CALIB_CH_NO];    //optical black sensor geometry information by BPC_MODE
    BPC_OB_Offset_s OBOffset[2][BPC_MAX_MODES*CALIB_CH_NO];
} BPC_Obj_t;

typedef struct BPC_Cluster_Detection_s_ {
    int Channel;
    int Flip;
    int BadPixelPage;
    int BPCMode;
    int BufferIndex;
    int FirstPixelGreen;
    int ReloadFromNand;
    int NgThreshold;
    int DebugFlag;
    int DebugThd;
} BPC_Cluster_Detection_s;


/* ---------- */
// FPN BMP management
typedef struct BMPFile_Magic_s {
    unsigned char Magic[2];
} BMPFile_Magic_t;

typedef struct BMPFile_Header_s {
    UINT32 FileSize;
    UINT16 Creator1;
    UINT16 Creator2;
    UINT32 BMPOffset;
} BMPFile_Header_t;

typedef struct BMPInfo_Header_s {
    UINT32 HeaderSize;
    int Width;
    int Height;
    UINT16 Planes;
    UINT16 Bpp;
    UINT32 Compression;
    UINT32 BMPDataSize;
    UINT32 HResolution;
    UINT32 VResolution;
    UINT32 UsedColors;
    UINT32 ImportantColors;
} BMPInfo_Header_t;

typedef struct BMPPalette_Bpp2_s {
    UINT32 Color[2];
} BMPPalette_Bpp2_t;

extern int AmpUT_CalibSiteInit(void);
extern int AmbaShell_MemAlloc(void **pBuf, UINT32 Size);
extern int AmbaShell_MemFree(void * pMemBase);
extern int AmbaUT_CalibGetDspMode(AMBA_DSP_IMG_MODE_CFG_s *pMode);

