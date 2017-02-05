/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: Amba_IQParamHandler.h
 *
 *  @Copyright      :: Copyright (C) 2013 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Implementation of IQ param handler
 *
 *  @History        ::
 *      Date        Name        Comments
 *      07/16/2013  Eathan Wu Created
 *
\*-------------------------------------------------------------------------------------------------------------------*/
#ifndef __AMBA_IQPARAM_HANDLER_H__
#define __AMBA_IQPARAM_HANDLER_H__

#include "AmbaImg_AaaDef.h"
#include "AmbaImg_Adjustment_A9.h"
#include "AmbaCalib_Adjustment_A9.h"




//#define IMG_MODE_TV           (0)
//#define IMG_MODE_PC           (1)

//#define LOAD_IQ_C_OR_ROM         (0)  // form C file :1; form ROM file 0

#define ADJ_VIDEO_TABLE_MAX_NO             (2)
#define ADJ_VIDEO_HISO_TABLE_MAX_NO        (1)
#define ADJ_PHOTO_TABLE_MAX_NO             (2)
#define ADJ_STILL_LISO_TABLE_MAX_NO        (2)
#define ADJ_STILL_HISO_TABLE_MAX_NO        (1)
#define ADJ_STILL_MFHISO_TABLE_MAX_NO (1)
#define SCENE_TABLE_MAX_NO                 (5)
#define SCENE_TABLE_CONTAIN_SETS           (8)
#define SCENE_TABLE_TATOL_NO               (36)              
#define IMG_TABLE_MAX_NO                   (1)
#define AAA_TABLE_MAX_NO                   (1)
#define ADJ_TABLE_MAX_NO                   (1)
#define ADJ_STILL_IDX_INFO_TABLE_MAX_NO    (1)
#define ADJ_VIDEO_IDX_INFO_TABLE_MAX_NO    (1)
#define DE_VIDEO_TABLE_MAX_NO              (1)
#define DE_STILL_TABLE_MAX_NO              (1)

#define ADJ_VIDEO_TABLE_VALID_NO           (2)
#define ADJ_VIDEO_HISO_TABLE_VALID_NO      (1)
#define ADJ_PHOTO_TABLE_VALID_NO           (2)
#define ADJ_STILL_LISO_TABLE_VALID_NO      (2)
#define ADJ_STILL_HISO_TABLE_VALID_NO      (1)
#define ADJ_STILL_MFHISO_TABLE_VALID_NO (1)
#define SCENE_TABLE_VALID_NO               (5)
#define IMG_TABLE_VALID_NO                 (1)
#define AAA_TABLE_VALID_NO                 (1)
#define ADJ_TABLE_VALID_NO                 (1)
#define ADJ_STILL_IDX_INFO_TABLE_VALID_NO  (1)
#define ADJ_VIDEO_IDX_INFO_TABLE_VALID_NO  (1)
#define DE_VIDEO_TABLE_VALID_NO            (1)
#define DE_STILL_TABLE_VALID_NO            (1)

#define IMG_PARAM                 (0)
#define AAA_PARAM                 (1)
#define ADJ_PARAM_VIDEO           (2)
#define ADJ_PARAM_PHOTO           (3)
#define ADJ_PARAM_LISO_STILL      (4)
#define ADJ_PARAM_HISO_STILL      (5)
#define ADJ_TABLE_PARAM           (6)
#define ADJ_PARAM_HISO_VIDEO      (7)

#define ADJ_PARAM_STILL_IDX       (10)
#define ADJ_PARAM_VIDEO_IDX       (11)

#define SCENE_DATA                (12)

#define DE_PARAM_VIDEO            (16)
#define DE_PARAM_STILL            (17)
#define ADJ_CALIB_PARAM            (18)
#define ADJ_PARAM_MFHISO_STILL    (19)

#define PARAM_UNLOAD        (0)
#define PARAM_LOADED        (1)

#define DEF_CC_TABLE_NO         (5)

#define GIpParamCategoryNum               (13)
#define GIpParamCategory_IMG              (0)
#define GIpParamCategory_AAA              (1)
#define GIpParamCategory_ADJ_STILL        (2)
#define GIpParamCategory_ADJ_HISO_STILL   (3)
#define GIpParamCategory_ADJ_VIDEO        (4)
#define GIpParamCategory_ADJ_PHOTO        (5)
#define GIpParamCategory_ADJ_STILL_IDX    (6)
#define GIpParamCategory_ADJ_VIDEO_IDX    (7)
#define GIpParamCategory_SCENE            (8)
#define GIpParamCategory_DE_VIDEO         (9)
#define GIpParamCategory_DE_STILL         (10)
#define GIpParamCategory_ADJ_TABLE        (11)
#define GIpParamCategory_ADJ_HISO_VIDEO   (12)
#define GIpParamCategory_CALIB            (13)
#define GIpParamCategory_ADJ_MFHISO_STILL (14)

//0: img_default
//1: aaa_default
//2: adj_still_default
//3: adj_hiso_still_default
//4: adj_video_default XXXX
//5: adj_photo_default
//6: adj_still_idx
//7: scene_data_s XXXX
//8: de_default_video
//9: de_default_still
//10:adj_table_param_default
//11:adj_calib_param
typedef enum _IP_SENSOR_ID_e_ {
    SESNOR_IMX117 = 1,
    SESNOR_IMX144 = 2,
    SESNOR_IMX183 = 3
} IP_SENSOR_ID_e;

typedef struct _PARAM_TABLE_PATH_s_ {
    const char *ImageParam;
    const char *AaaParam;
    const char *AdjPhotoParam;
    const char *AdjVideoParam;
    const char *AdjVideoHisoParam;    
    const char *AdjStillLowIsoParam;
} PARAM_TABLE_PATH_s;

typedef struct _COLOR_TABLE_PATH_s_ {
    const int     ColorIdx;
    const char    *Video[5];
    const char    *Still[5];
} COLOR_TABLE_PATH_s;

typedef struct _IP_TABLE_PATH_s_ {
    const char *IpTablePath;
} IP_TABLE_PATH_s;

typedef struct _PARAM_PROXY_s_ {
    /** Sensor ID */
    UINT32 Id;
    /** Sensor name */
    char Name[80];
    /** Module init interface */
    int (*Init)(void);
    /** Module parameter get interface */
    IMG_PARAM_s* (*GetImgParam)(int paramLoaded);
    AAA_PARAM_s* (*GetAaaParam)(void);
    ADJ_VIDEO_PARAM_s* (*GetAdjVideoParam)(int paramLoaded, int tableNo);
    ADJ_PHOTO_PARAM_s* (*GetAdjPhotoParam)(int paramLoaded, int tableNo);
    ADJ_STILL_LOW_ISO_PARAM_s* (*GetAdjStillLIsoParam)(int paramLoaded, int tableNo);
    ADJ_STILL_HISO_PARAM_s* (*GetAdjStillHIsoParam)(int paramLoaded, int tableNo);
    ADJ_STILL_HISO_PARAM_s* (*GetAdjStillMfHIsoParam)(int paramLoaded, int tableNo);      
    ADJ_STILL_IDX_INFO_s* (*GetAdjStillIdxInfoParam)(int paramLoaded);
    SCENE_DATA_s* (*GetSceneData)(int sceneLoaded, int sceneSet);
    DE_PARAM_s* (*GetDeParam)(int mode, int paramLoaded);
    CALIBRATION_PARAM_s* (*GetCalibParam)(void);
    ADJ_TABLE_PARAM_s* (*GetAdjTableParam)(void);

    /** Public API */
    COLOR_TABLE_PATH_s* (*GetColorTablePath)(void);
    PARAM_TABLE_PATH_s* (*GetParamTablePath)(void);
    IP_TABLE_PATH_s*    (*GetAdjVideoTablePath)(int paramLoaded, int tableNo);
    IP_TABLE_PATH_s*    (*GetAdjPhotoTablePath)(int paramLoaded, int tableNo);
    IP_TABLE_PATH_s*    (*GetAdjStillLIsoTablePath)(int paramLoaded, int tableNo);
    IP_TABLE_PATH_s*    (*GetAdjStillHIsoTablePath)(int paramLoaded, int tableNo);
    IP_TABLE_PATH_s*    (*GetSceneDataTablePath)(int paramLoaded, int sceneSet);
    IP_TABLE_PATH_s*    (*GetImgTablePath)(void);
    IP_TABLE_PATH_s*    (*GetAaaTablePath)(void);
    IP_TABLE_PATH_s*    (*GetAdjStillIdxInfoTablePath)(void);
    IP_TABLE_PATH_s*    (*GetDeTablePath)(int mode);
    IP_TABLE_PATH_s*    (*GetAdjTablePath)(void);      
    
} PARAM_PROXY_s;

typedef struct _IP_PARAM_s_ {
    const char    *Name;
    const int     Type;
    const void    *Data;
    const int     Length;
} IP_PARAM_s;

typedef struct _IP_TABLE_ADDR_s_ {
    void    *Data;
} IP_TABLE_ADDR_s;


//karl_20141002>>
typedef struct _GIpParam_Info_s_{
UINT16 GIpParamCategoryMaxNum[GIpParamCategoryNum];
UINT16 GIpParamValidTableFlag[GIpParamCategoryNum];  
}GIpParam_Info_s;

#define IP_PARAM_MAX_NUM                (30)
typedef struct _PARAM_HANDLER_s_ {
    PARAM_PROXY_s IqphParamProxy;
    int GVideoLoaded;
    int GPhotoLoaded;
    int IqphCcSceneTable[56];
    int IqphCcDeVideoTable[DIGITAL_LAST];
    UINT16 IqphSceneModeCc3d;
    UINT16 IqphDeCc3d;
    /* ADJ parameters index table */
    UINT8 IqphVideoCcAddr[DEF_CC_TABLE_NO][CC_3D_SIZE];
    UINT8 IqphStillCcAddr[DEF_CC_TABLE_NO][CC_3D_SIZE];
    char *IqphCcRegID;
    UINT8 IqphCcReg[CC_REG_SIZE];
    ADJ_VIDEO_PARAM_s IqphAdjVideo[ADJ_VIDEO_TABLE_MAX_NO];
    ADJ_PHOTO_PARAM_s IqphAdjPhoto[ADJ_PHOTO_TABLE_MAX_NO];
    ADJ_STILL_LOW_ISO_PARAM_s IqphAdjLisoStill[ADJ_STILL_LISO_TABLE_MAX_NO];
    ADJ_STILL_HISO_PARAM_s IqphAdjHisoStill[ADJ_STILL_HISO_TABLE_MAX_NO];
    SCENE_DATA_s IqphSceneData[SCENE_TABLE_CONTAIN_SETS*SCENE_TABLE_MAX_NO];
    UINT16 GIpParamCategoryMaxNum[GIpParamCategoryNum];// = {0x01,0x01,0x02,0x01,0x02,0x02,0x01,0x05,0x01,0x01};
    UINT16 GIpParamValidTableFlag[GIpParamCategoryNum];// = {0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x05,0x01,0x01};
    IP_PARAM_s GIpParam[IP_PARAM_MAX_NUM];
    IMG_PARAM_s IqphImg;
    AAA_PARAM_s IqphAaaDefault;
    ADJ_STILL_IDX_INFO_s IqphAdjStillIdx;
    DE_PARAM_s IqphDeVideo;               
    DE_PARAM_s IqphDeStill;              
    ADJ_TABLE_PARAM_s IqphAdjTable;    
    
} PARAM_HANDLER_s;
//karl_20141002<<

/*************************************************************************
 * IQ Param Handler Public APIs
 ************************************************************************/

int AmbaIQParam_Init_Param_Proxy(UINT32 chNo);
int AmbaIQParam_Register(UINT32 chNo, int sensorID);
int AmbaIQParam_Attach(UINT32 chNo, PARAM_PROXY_s *par);
int AmbaIQParam_Remove(UINT32 chNo);
int AmbaIQParam_Get_By_Name(UINT32 chNo, const char *name, const int type, IP_PARAM_s **param);
#endif /* __PARAM_PROXY_H__ */
