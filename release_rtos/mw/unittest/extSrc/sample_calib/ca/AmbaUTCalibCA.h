/**
 * @file mw/unittest/extSrc/sample_calib/ca/AmbaUTCalibCA.h
 *
 * header file for CA calibration
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
#include "AmbaCalib_Ca.h"

//Total 12388 bytes
#define CA_MAX_WIDTH  (64)
#define CA_MAX_HEIGHT (48)
#define CA_MAX_SIZE (CA_MAX_WIDTH*CA_MAX_HEIGHT)
// 64x48. Maximum size for 64  tile is 4032x3008
//                     for 128 tile is 8064x6016

#define MAX_CA_TABLE_COUNT (5)
#define CAL_CA_BASE          (0x00000000)
#define CAL_CA_DATA(x)       ((x) + CAL_CA_BASE)
#define CAL_CA_ENABLE        CAL_CA_DATA(0x00)
#define CAL_CA_TABLE_COUNT   CAL_CA_DATA(0x04)
#define CAL_CA_TABLE_BASE    CAL_CA_DATA(0x20)
#define CAL_CA_TABLE(x)      (CAL_CA_TABLE_BASE + sizeof(CA_Storage_s)*(x))


//warp information for each site, to be similar with IK
typedef __packed struct Ca_Storage_Header_s_ {
    UINT32                              Enable;
    UINT32                              Version;        /* 0x20130101 */
    int                                 HorGridNum;     /* Horizontal grid number. */
    int                                 VerGridNum;     /* Vertical grid number. */
    int                                 TileWidthExp;   /* 4:16, 5:32, 6:64, 7:128, 8:256, 9:512 */
    int                                 TileHeightExp;  /* 4:16, 5:32, 6:64, 7:128, 8:256, 9:512 */
    AMBA_DSP_IMG_VIN_SENSOR_GEOMETRY_s  VinSensorGeo;   /* Vin sensor geometry when calibrating. */
    INT16                               RedScaleFactor; //Red scale factor
    INT16                               BlueScaleFactor;//blue scale factor
    UINT32                              Reserved;       /* Reserved for extention. */
    UINT32                              Reserved1;      /* Reserved for extention. */
    UINT32                              CAZoomStep;     /* zoom step*/
    UINT32                              CAChannel;      /* channel ID*/
} CA_Storage_Header_s;





typedef struct CA_Storage_s_ {
    CA_Storage_Header_s CAHeader;
    //6KB for x,y each = 12KB
    AMBA_DSP_IMG_GRID_POINT_s CAVector[CA_MAX_SIZE]; // CA vectors
} CA_Storage_s;


typedef struct Cal_CA_Table_Info_s_ {
    AMBA_DSP_IMG_GRID_POINT_s *Wp;
    UINT32  GridW;
    UINT32  GridH;
    UINT32  TileW;
    UINT32  TileH;
    UINT32  ImgW;
    UINT32  ImgH;
    UINT32  StartX;
    UINT32  StartY;
    INT32    OffSensorFlag;
} Cal_CA_Table_Info_s;


typedef struct CA_Control_s_ {
    UINT32 Enable;
    UINT32 CATableCount;
    UINT32 Debug;
    UINT32 Reserved[5];
    CA_Storage_s *CATable[MAX_CA_TABLE_COUNT];
} CA_Control_s;

typedef struct Cal_Ca_Entry_s_ {
    double RedHor;
    double RedVer;
    double BlueHor;
    double BlueVer;
} Cal_CA_Entry_s;

enum CA_CALIB_ERROR_MSG_e_ {
    CA_CALIB_OK                          =  0,
    CA_CALIB_INCORRECT_ARGUMENT_NUM   = -1,
    CA_CALIB_READSCRIPT_ERROR         = -2,
    CA_CALIB_INCORRECT_PARAMETER      = -3,
    CA_CALIB_OPEN_FILE_ERROR          = -4,
    CA_CALIB_CLOSE_FILE_ERROR         = -5,
    CA_CALIB_INCORRECT_INPUT_NUM      = -6,
    CA_CALIB_TOO_MUCH_INPUT_NUM       = -7,
    CA_CALIB_INTERVAL_NOT_FOUND       = -8,
    CA_CALIB_CREATE_BUFFER_ERROR      = -9,
    CA_CALIB_COMPENSATE_RATIO_ERROR   = -10,
    CA_CALIB_RRATIO_ERROR             = -11,
    CA_CALIB_CENTER_POSITION_ERROR    = -12,
    CA_CALIB_UNDEFINED_METHOD         = -13,
    CA_CALIB_WRITE_FILE_ERROR         = -14,
};

extern CA_Control_s AppCAControl;

extern int AmpUT_CalibSiteInit(void);
extern int AmbaUT_CalibGetDspMode(AMBA_DSP_IMG_MODE_CFG_s *pMode);
