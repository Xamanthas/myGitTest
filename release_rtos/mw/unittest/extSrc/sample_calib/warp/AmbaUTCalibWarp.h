/**
 * @file mw/unittest/extSrc/sample_calib/warp/AmbaUTCalibWarp.h
 *
 * header for warp calibration
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
#include "../AmbaUTCalibMgr.h"
#include "AmbaDSP_WarpCore.h"
#include "AmbaCalib_Warp.h"

//Total 12388 bytes
#define WARP_MAX_WIDTH  (72)
#define WARP_MAX_HEIGHT (72)
#define WARP_MAX_SIZE (WARP_MAX_WIDTH*WARP_MAX_HEIGHT)
// 64x48. Maximum size for 64  tile is 4032x3008
//                     for 128 tile is 8064x6016

#define MAX_WARP_TABLE_COUNT (5)
#define CAL_WARP_BASE          (0x00000000)
#define CAL_WARP_DATA(x)       ((x) + CAL_WARP_BASE)
#define CAL_WARP_ENABLE        CAL_WARP_DATA(0x00)
#define CAL_WARP_TABLE_COUNT   CAL_WARP_DATA(0x04)
#define CAL_WARP_TABLE_BASE    CAL_WARP_DATA(0x20)
#define CAL_WARP_TABLE(x)      (CAL_WARP_TABLE_BASE + sizeof(Warp_Storage_s)*(x))


//warp information for each site, to be similar with IK
//typedef union Warp_Storage_Header_s_ {
typedef __packed struct Warp_Storage_Header_s_ {            //64 bytes
    UINT32                              Enable;
    UINT32                              Version;        /* 0x20130101 */
    int                                 HorGridNum;     /* Horizontal grid number. */
    int                                 VerGridNum;     /* Vertical grid number. */
    int                                 TileWidthExp;   /* 4:16, 5:32, 6:64, 7:128, 8:256, 9:512 */
    int                                 TileHeightExp;  /* 4:16, 5:32, 6:64, 7:128, 8:256, 9:512 */
    AMBA_DSP_IMG_VIN_SENSOR_GEOMETRY_s  VinSensorGeo;   /* Vin sensor geometry when calibrating. */
    UINT32                              Warp2StageFlag; /**< warp 2 stage compensation */
    UINT32                              Reserved1;      /* Reserved for extention. */
    UINT32                              Reserved2;      /* Reserved for extention. */
    UINT32                              WarpZoomStep;   /* zoom step number*/
    UINT32                              WarpChannel;    /* channel ID*/
} Warp_Storage_Header_s;

typedef struct Warp_Storage_s_ {
    Warp_Storage_Header_s WarpHeader;
    //6KB for x,y each = 12KB
    AMBA_DSP_IMG_GRID_POINT_s WarpVector[WARP_MAX_SIZE]; // WARP vectors
} Warp_Storage_s;


typedef struct Cal_Warp_Table_Info_s_ {
    AMBA_DSP_IMG_GRID_POINT_s *Wp;
    UINT32  GridW;
    UINT32    GridH;
    UINT32    TileW;
    UINT32    TileH;
    UINT32    ImgW;
    UINT32    ImgH;
    UINT32    StartX;
    UINT32    StartY;
    INT32   OffSensorFlag;
} Cal_Warp_Table_Info_s;

typedef struct Warp_Control_s_ {
    UINT32 Enable;
    UINT32 WarpTableCount;
    UINT32 Debug;
    UINT32 Reserved[5];
    Warp_Storage_s *WarpTable[MAX_WARP_TABLE_COUNT];
} Warp_Control_s;


extern Warp_Control_s AppWarpControl;

extern int AmpUT_CalibSiteInit(void);
extern int AmbaUT_CalibGetDspMode(AMBA_DSP_IMG_MODE_CFG_s *pMode);