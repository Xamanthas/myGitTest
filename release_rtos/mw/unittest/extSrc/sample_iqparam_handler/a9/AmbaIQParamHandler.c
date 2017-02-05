/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaIQParamHandler.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "AmbaDataType.h"
#include "AmbaPrint.h"
#include "AmbaFS.h"
#include "AmbaUtility.h"
//#include <common/common.h>

#include "AmbaImg_Impl_Cmd.h"
#include "AmbaImg_Adjustment_A9.h"
#include "AmbaIQParamHandler.h"
#include "AmbaROM.h"

#define __AMBA_IQ_PARAM_HANDLER_PRINTK__ 1
#ifdef __AMBA_IQ_PARAM_HANDLER_PRINTK__
#define AmbaIqParamHandlerPrintk AmbaPrint
#else
#define AmbaIqParamHandlerPrintk(...)
#endif

//PARAM_PROXY_s IqphParamProxy;
int GVideoLoaded = 0;
int GPhotoLoaded = 0;



static    UINT16 IqphSceneModeCc3d = SYSTEM_DEFAULT;
static    UINT16 IqphDeCc3d = SYSTEM_DEFAULT;

PARAM_HANDLER_s *pParamHdlr[2]; 

GIpParam_Info_s GIpParamInfo[2];
UINT16 GIpParamCategoryMaxNum[GIpParamCategoryNum];// = {0x01,0x01,0x02,0x01,0x02,0x02,0x01,0x05,0x01,0x01};
UINT16 GIpParamValidTableFlag[GIpParamCategoryNum];// = {0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x05,0x01,0x01};


IP_PARAM_s GIpParam_tmp_1[] = {
		{"img_default", IMG_PARAM, NULL, sizeof(IMG_PARAM_s)},
		{"aaa_default", AAA_PARAM, NULL, sizeof(AAA_PARAM_s)},
	
		{"adj_still_default_00", ADJ_PARAM_LISO_STILL, NULL, sizeof(ADJ_STILL_LOW_ISO_PARAM_s)},
		{"adj_still_default_01", ADJ_PARAM_LISO_STILL, NULL, sizeof(ADJ_STILL_LOW_ISO_PARAM_s)},
	
		{"adj_hiso_still_default_00", ADJ_PARAM_HISO_STILL, NULL , sizeof(ADJ_STILL_HISO_PARAM_s)},
	
		{"adj_video_default_00", ADJ_PARAM_VIDEO, NULL, sizeof(ADJ_VIDEO_PARAM_s)},
		{"adj_video_default_01", ADJ_PARAM_VIDEO, NULL, sizeof(ADJ_VIDEO_PARAM_s)},
	
		{"adj_photo_default_00", ADJ_PARAM_PHOTO, NULL, sizeof(ADJ_PHOTO_PARAM_s)},
		{"adj_photo_default_01", ADJ_PARAM_PHOTO, NULL, sizeof(ADJ_PHOTO_PARAM_s)},
	
		{"adj_still_idx",	  ADJ_PARAM_STILL_IDX, NULL, sizeof(ADJ_STILL_IDX_INFO_s)},
//		{"adj_video_idx",	  ADJ_PARAM_VIDEO_IDX, NULL, sizeof(ADJ_VIDEO_IDX_INFO_s)},
	
		{"scene_data_s01",	  SCENE_DATA, NULL, sizeof(SCENE_DATA_s)*SCENE_TABLE_CONTAIN_SETS},
		{"scene_data_s02",	  SCENE_DATA, NULL, sizeof(SCENE_DATA_s)*SCENE_TABLE_CONTAIN_SETS},
		{"scene_data_s03",	  SCENE_DATA, NULL, sizeof(SCENE_DATA_s)*SCENE_TABLE_CONTAIN_SETS},
		{"scene_data_s04",	  SCENE_DATA, NULL, sizeof(SCENE_DATA_s)*SCENE_TABLE_CONTAIN_SETS},
		{"scene_data_s05",	  SCENE_DATA, NULL, sizeof(SCENE_DATA_s)*SCENE_TABLE_CONTAIN_SETS},
	
		{"de_default_video", DE_PARAM_VIDEO, NULL, sizeof(DE_PARAM_s)},
		{"de_default_still", DE_PARAM_STILL, NULL, sizeof(DE_PARAM_s)},

		{"adj_table_param_default", ADJ_TABLE_PARAM, NULL, sizeof(ADJ_TABLE_PARAM_s)},
//		{"adj_hiso_video_default_00", ADJ_PARAM_HISO_VIDEO, NULL, sizeof(ADJ_VIDEO_HISO_PARAM_s)},	 
	//	  {"adj_calib_param",	 ADJ_CALIB_PARAM, NULL, sizeof(CALIBRATION_PARAM_s)},
	
		{ (const char *) 0x0, 0x0, (void*) 0x0, -1 }
	};


IP_PARAM_s GIpParam_tmp_2[] = {
		{"img_default", IMG_PARAM, NULL, sizeof(IMG_PARAM_s)},
		{"aaa_default", AAA_PARAM, NULL, sizeof(AAA_PARAM_s)},
	
		{"adj_still_default_00", ADJ_PARAM_LISO_STILL, NULL, sizeof(ADJ_STILL_LOW_ISO_PARAM_s)},
		{"adj_still_default_01", ADJ_PARAM_LISO_STILL, NULL, sizeof(ADJ_STILL_LOW_ISO_PARAM_s)},
	
		{"adj_hiso_still_default_00", ADJ_PARAM_HISO_STILL, NULL , sizeof(ADJ_STILL_HISO_PARAM_s)},
	
		{"adj_video_default_00", ADJ_PARAM_VIDEO, NULL, sizeof(ADJ_VIDEO_PARAM_s)},
		{"adj_video_default_01", ADJ_PARAM_VIDEO, NULL, sizeof(ADJ_VIDEO_PARAM_s)},
	
		{"adj_photo_default_00", ADJ_PARAM_PHOTO, NULL, sizeof(ADJ_PHOTO_PARAM_s)},
		{"adj_photo_default_01", ADJ_PARAM_PHOTO, NULL, sizeof(ADJ_PHOTO_PARAM_s)},
	
		{"adj_still_idx",	  ADJ_PARAM_STILL_IDX, NULL, sizeof(ADJ_STILL_IDX_INFO_s)},
//		{"adj_video_idx",	  ADJ_PARAM_VIDEO_IDX, NULL, sizeof(ADJ_VIDEO_IDX_INFO_s)},
	
		{"scene_data_s01",	  SCENE_DATA, NULL, sizeof(SCENE_DATA_s)*SCENE_TABLE_CONTAIN_SETS},
		{"scene_data_s02",	  SCENE_DATA, NULL, sizeof(SCENE_DATA_s)*SCENE_TABLE_CONTAIN_SETS},
		{"scene_data_s03",	  SCENE_DATA, NULL, sizeof(SCENE_DATA_s)*SCENE_TABLE_CONTAIN_SETS},
		{"scene_data_s04",	  SCENE_DATA, NULL, sizeof(SCENE_DATA_s)*SCENE_TABLE_CONTAIN_SETS},
		{"scene_data_s05",	  SCENE_DATA, NULL, sizeof(SCENE_DATA_s)*SCENE_TABLE_CONTAIN_SETS},
	
		{"de_default_video", DE_PARAM_VIDEO, NULL, sizeof(DE_PARAM_s)},
		{"de_default_still", DE_PARAM_STILL, NULL, sizeof(DE_PARAM_s)},
	
		{"adj_table_param_default", ADJ_TABLE_PARAM, NULL, sizeof(ADJ_TABLE_PARAM_s)},
//		{"adj_hiso_video_default_00", ADJ_PARAM_HISO_VIDEO, NULL, sizeof(ADJ_VIDEO_HISO_PARAM_s)},	 
	//	  {"adj_calib_param",	 ADJ_CALIB_PARAM, NULL, sizeof(CALIBRATION_PARAM_s)},
	
		{ (const char *) 0x0, 0x0, (void*) 0x0, -1 }
	};

    
    IP_PARAM_s *pGIpParam[2] = {NULL,NULL};


//////////////////////////////////////////////////////////////////////////////////////////
/** Module parameter get interface */

int _Init_Color_De_Video_Table(UINT32 chNo)
{
    int idx;
    //DE_PARAM_s *de = AmbaIQParam_Get_DE_PARAM(DE_PARAM_VIDEO,PARAM_LOADED);
    DE_PARAM_s *de;
    de = &(pParamHdlr[chNo]->IqphDeVideo);
    for (idx = 0; idx < DIGITAL_LAST; idx++) {
        pParamHdlr[chNo]->IqphCcDeVideoTable[idx] = de->DeInfo[idx].Cc3dNo;
		//AmbaPrint("<%s>, %d, DeCcIdx : %d", __FUNCTION__, idx, IqphCcDeVideoTable[idx]);
    }
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParam_Get_Adj_Video_Table_Path
 *
 *  @Description:: Get Video Adj Table path
 *
 *  @Input      ::
 *    int paramLoaded : 0(PARAM_UNLOAD), 1(PARAM_LOADED)
 *    int tableNo : load table number
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    IP_TABLE_PATH_s * : Return Video Adj param table rom path.
\*-----------------------------------------------------------------------------------------------*/
IP_TABLE_PATH_s* AmbaIQParam_Get_Adj_Video_Table_Path(UINT32 chNo, int paramLoaded, int tableNo)
{
    return pParamHdlr[chNo]->IqphParamProxy.GetAdjVideoTablePath(PARAM_UNLOAD,tableNo);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParam_Get_Adj_Photo_Table_Path
 *
 *  @Description:: Get Photo Adj Table path
 *
 *  @Input      ::
 *    int paramLoaded : 0(PARAM_UNLOAD), 1(PARAM_LOADED)
 *    int tableNo : load table number
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    IP_TABLE_PATH_s * : Return Photo Adj param table rom path.
\*-----------------------------------------------------------------------------------------------*/
IP_TABLE_PATH_s* AmbaIQParam_Get_Adj_Photo_Table_Path(UINT32 chNo, int paramLoaded, int tableNo)
{
    return pParamHdlr[chNo]->IqphParamProxy.GetAdjPhotoTablePath(PARAM_UNLOAD,tableNo);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParam_Get_Adj_Still_LIso_Table_Path
 *
 *  @Description:: Get Still Low Iso Adj Table path
 *
 *  @Input      ::
 *    int paramLoaded : 0(PARAM_UNLOAD), 1(PARAM_LOADED)
 *    int tableNo : load table number
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    IP_TABLE_PATH_s * : Return Still Low Iso Adj param table rom path.
\*-----------------------------------------------------------------------------------------------*/
IP_TABLE_PATH_s* AmbaIQParam_Get_Adj_Still_LIso_Table_Path(UINT32 chNo, int paramLoaded, int tableNo)
{
    return pParamHdlr[chNo]->IqphParamProxy.GetAdjStillLIsoTablePath(PARAM_UNLOAD,tableNo);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParam_Get_Adj_Still_HIso_Table_Path
 *
 *  @Description:: Get still high iso Adj Table path
 *
 *  @Input      ::
 *    int paramLoaded : 0(PARAM_UNLOAD), 1(PARAM_LOADED)
 *    int tableNo : load table number
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    IP_TABLE_PATH_s * : Return Video Adj param table rom path.
\*-----------------------------------------------------------------------------------------------*/
IP_TABLE_PATH_s* AmbaIQParam_Get_Adj_Still_HIso_Table_Path(UINT32 chNo, int paramLoaded, int tableNo)
{
    return pParamHdlr[chNo]->IqphParamProxy.GetAdjStillHIsoTablePath(PARAM_UNLOAD,tableNo);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParam_Get_Scene_Data_Table_Path
 *
 *  @Description:: Get scene data Table path
 *
 *  @Input      ::
 *    int paramLoaded : 0(PARAM_UNLOAD), 1(PARAM_LOADED)
 *    int tableNo : load table number
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    IP_TABLE_PATH_s * : Return scene data table rom path.
\*-----------------------------------------------------------------------------------------------*/
IP_TABLE_PATH_s* AmbaIQParam_Get_Scene_Data_Table_Path(UINT32 chNo, int paramLoaded, int tableNo)
{
    return pParamHdlr[chNo]->IqphParamProxy.GetSceneDataTablePath(PARAM_UNLOAD,tableNo);
}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParam_Get_Img_Table_Path
 *
 *  @Description:: Get Img Table path
 *
 *  @Input      ::
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    IP_TABLE_PATH_s * : Return Img param table rom path.
\*-----------------------------------------------------------------------------------------------*/
IP_TABLE_PATH_s* AmbaIQParam_Get_Img_Table_Path(UINT32 chNo)
{
    return pParamHdlr[chNo]->IqphParamProxy.GetImgTablePath();
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParam_Get_Aaa_Table_Path
 *
 *  @Description:: Get Aaa Table path
 *
 *  @Input      ::
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    IP_TABLE_PATH_s * : Return Aaa param table rom path.
\*-----------------------------------------------------------------------------------------------*/
IP_TABLE_PATH_s* AmbaIQParam_Get_Aaa_Table_Path(UINT32 chNo)
{
    return pParamHdlr[chNo]->IqphParamProxy.GetAaaTablePath();
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParam_Get_Adj_Still_Idx_Info_Table_Path
 *
 *  @Description:: Get still idx info Table path
 *
 *  @Input      ::
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    IP_TABLE_PATH_s * : Return Still Idx Info param table rom path.
\*-----------------------------------------------------------------------------------------------*/
IP_TABLE_PATH_s* AmbaIQParam_Get_Adj_Still_Idx_Info_Table_Path(UINT32 chNo)
{
    return pParamHdlr[chNo]->IqphParamProxy.GetAdjStillIdxInfoTablePath();
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParam_Get_De_Table_Path
 *
 *  @Description:: Get still/video De Table path
 *
 *  @Input      ::
 *    int mode : still or video mode
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    IP_TABLE_PATH_s * : Return De param table rom path.
\*-----------------------------------------------------------------------------------------------*/
IP_TABLE_PATH_s* AmbaIQParam_Get_De_Table_Path(UINT32 chNo,int mode)
{
    return pParamHdlr[chNo]->IqphParamProxy.GetDeTablePath(mode);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParam_Get_Adj_Table_Path
 *
 *  @Description:: Get Adj Table path
 *
 *  @Input      ::
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    IP_TABLE_PATH_s * : Return Adj param table rom path.
\*-----------------------------------------------------------------------------------------------*/
IP_TABLE_PATH_s* AmbaIQParam_Get_Adj_Table_Path(UINT32 chNo)
{
    return pParamHdlr[chNo]->IqphParamProxy.GetAdjTablePath();
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: _Load_Iq_Param_RomTable
 *
 *  @Description:: Load IQ parameter table form ROM (bin files)
 *
 *  @Input      ::
 *    IP_PARAM_s *pIpParam : Iq parameters address
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    int : 0(OK)/-1(NG)
\*-----------------------------------------------------------------------------------------------*/
int _Load_Iq_Param_RomTable(IP_PARAM_s *pIpParam,const char* pTablePath)
{

    int                     RVal = -1;
    int                     FSize;

//    FSize = AmbaROM_GetSize(AMBA_ROM_SYS_DATA, pIpParam->Name, 0x0);  //size
//                                                 //name     //read to memeory
//    RVal = AmbaROM_LoadByName(AMBA_ROM_SYS_DATA, pIpParam->Name, (UINT8*)pIpParam->Data, FSize, 0);

    FSize = AmbaROM_GetSize(AMBA_ROM_SYS_DATA, pTablePath, 0x0);  //size
                                                 //name     //read to memeory
    RVal = AmbaROM_LoadByName(AMBA_ROM_SYS_DATA, pTablePath , (UINT8*)pIpParam->Data, FSize, 0);

    if (RVal == -1) {
        AmbaIqParamHandlerPrintk("load IQ Params from ROMFS error");
    }else {
        //AmbaIqParamHandlerPrintk("load IQ Params %s from ROMFS: %d", pIpParam->Name, FSize);
	AmbaIqParamHandlerPrintk("load IQ Params %s from ROMFS: %d", pTablePath, FSize);
    }

    return RVal;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: _Validate_Param
 *
 *  @Description:: Validate parametere tables
 *
 *  @Input      ::
 *    IP_PARAM_s *pIpParam : Pointer to Image parameter tables
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    int : 0(OK)/-1(NG)
\*-----------------------------------------------------------------------------------------------*/
int _Validate_Param(UINT32 chNo, IP_PARAM_s *pIpParam, UINT16 tableNo)
{

static IP_TABLE_PATH_s* pTablePath;


    if (pIpParam == 0) {
        return -1;
    }

    switch (pIpParam->Type) {
        case IMG_PARAM:
                pTablePath =  AmbaIQParam_Get_Img_Table_Path(chNo);
                _Load_Iq_Param_RomTable(pIpParam,pTablePath->IpTablePath);  
            break;

        case AAA_PARAM:
                pTablePath =  AmbaIQParam_Get_Aaa_Table_Path(chNo);
                _Load_Iq_Param_RomTable(pIpParam,pTablePath->IpTablePath);        
            break;
        case ADJ_TABLE_PARAM:
                pTablePath =  AmbaIQParam_Get_Adj_Table_Path(chNo);
                _Load_Iq_Param_RomTable(pIpParam,pTablePath->IpTablePath);                
            break;

        case ADJ_PARAM_VIDEO:
                pTablePath =  AmbaIQParam_Get_Adj_Video_Table_Path(chNo, PARAM_UNLOAD,tableNo);
                _Load_Iq_Param_RomTable(pIpParam,pTablePath->IpTablePath);
            break;
          
        case ADJ_PARAM_PHOTO:
                pTablePath =  AmbaIQParam_Get_Adj_Photo_Table_Path(chNo, PARAM_UNLOAD,tableNo);
                _Load_Iq_Param_RomTable(pIpParam,pTablePath->IpTablePath);
            break;

        case ADJ_PARAM_LISO_STILL:
                pTablePath =  AmbaIQParam_Get_Adj_Still_LIso_Table_Path(chNo, PARAM_UNLOAD,tableNo);
                _Load_Iq_Param_RomTable(pIpParam,pTablePath->IpTablePath);
            break;

        case ADJ_PARAM_HISO_STILL:
                pTablePath =  AmbaIQParam_Get_Adj_Still_HIso_Table_Path(chNo, PARAM_UNLOAD,tableNo);
                _Load_Iq_Param_RomTable(pIpParam,pTablePath->IpTablePath);
            break;

        case ADJ_PARAM_STILL_IDX:
                pTablePath =  AmbaIQParam_Get_Adj_Still_Idx_Info_Table_Path(chNo);
                _Load_Iq_Param_RomTable(pIpParam,pTablePath->IpTablePath);          
            break;

        case SCENE_DATA:
                pTablePath =  AmbaIQParam_Get_Scene_Data_Table_Path(chNo, PARAM_UNLOAD,tableNo);
                _Load_Iq_Param_RomTable(pIpParam,pTablePath->IpTablePath);
            break;
        case DE_PARAM_VIDEO:
                pTablePath =  AmbaIQParam_Get_De_Table_Path(chNo, IP_MODE_VIDEO);
                _Load_Iq_Param_RomTable(pIpParam,pTablePath->IpTablePath);            
            break;            
            
            break;
        case DE_PARAM_STILL:
                pTablePath =  AmbaIQParam_Get_De_Table_Path(chNo, IP_MODE_STILL);
                _Load_Iq_Param_RomTable(pIpParam,pTablePath->IpTablePath);
            break;            
            
            break;
//        case ADJ_CALIB_PARAM:
//            pIpParam->Data = (CALIBRATION_PARAM_s*)AmbaIQParam_Get_Calib_Param();
//            break;
        default:
            break;

    }
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: _Init_GIpParam_Flag
 *
 *  @Description:: Initialize parametere tables number
 *
 *  @Input      ::
 *    None
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    None
\*-----------------------------------------------------------------------------------------------*/
void _Init_GIpParam_Flag(void)
{
    UINT16 i;
    for(i=0;i<GIpParamCategoryNum;i++) {
        GIpParamCategoryMaxNum[i] =1;
        GIpParamValidTableFlag[i] =1;
    }

    GIpParamCategoryMaxNum[GIpParamCategory_ADJ_STILL]= ADJ_STILL_LISO_TABLE_MAX_NO;
    GIpParamCategoryMaxNum[GIpParamCategory_ADJ_HISO_STILL]= ADJ_STILL_HISO_TABLE_MAX_NO;
    GIpParamCategoryMaxNum[GIpParamCategory_ADJ_VIDEO]= ADJ_VIDEO_TABLE_MAX_NO;
    GIpParamCategoryMaxNum[GIpParamCategory_ADJ_PHOTO]= ADJ_PHOTO_TABLE_MAX_NO;
    GIpParamCategoryMaxNum[GIpParamCategory_SCENE]= SCENE_TABLE_MAX_NO;

    GIpParamValidTableFlag[GIpParamCategory_ADJ_STILL]= ADJ_STILL_LISO_TABLE_VALID_NO;
    GIpParamValidTableFlag[GIpParamCategory_ADJ_HISO_STILL]= ADJ_STILL_HISO_TABLE_VALID_NO;
    GIpParamValidTableFlag[GIpParamCategory_ADJ_VIDEO]= ADJ_VIDEO_TABLE_VALID_NO;
    GIpParamValidTableFlag[GIpParamCategory_ADJ_PHOTO]= ADJ_PHOTO_TABLE_VALID_NO;
    GIpParamValidTableFlag[GIpParamCategory_SCENE]= SCENE_TABLE_VALID_NO;

    }


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: _Init_Param
 *
 *  @Description:: Initialize parametere tables
 *
 *  @Input      ::
 *    UINT8 modeSwitch : mode switch
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    None
\*-----------------------------------------------------------------------------------------------*/
//void _Init_Param(UINT8 modeSwitch)
//{
//    UINT16 i,j,k;
//    UINT32 chNo;
//    chNo = 0;
//    i = 0;
//   
//    _Init_GIpParam_Flag();
//    for(j=0;j<GIpParamCategoryNum;j++) {
//        for(k=0;k<GIpParamCategoryMaxNum[j];k++) {
//            if (k<GIpParamValidTableFlag[j]) {
//                _Validate_Param(chNo, &GIpParam[i],k);
//            }
//            i++;
//        }
//    }
//
//}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: _Get_Cc_Table_Addr
 *
 *  @Description:: Retrieve Cc table address
 *
 *  @Input      ::
 *    int type   : IMG_MODE_TV(TV mode), IMG_MODE_PC(PC mode)
 *    int ccIdx  : Cc table number
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    UINT32 : Return Cc table address
\*-----------------------------------------------------------------------------------------------*/
UINT32 _Get_Cc_Table_Addr(UINT32 chNo,int type, int ccIdx)
{
    if (ccIdx < DEF_CC_TABLE_NO) {
        switch (type) {
        case IP_MODE_VIDEO:
            //return (UINT32)&IqphVideoCcAddr[ccIdx];
            return (UINT32)&(pParamHdlr[chNo]->IqphVideoCcAddr[ccIdx]);  
        case IP_MODE_STILL:
            //return (UINT32)&IqphStillCcAddr[ccIdx];
            return (UINT32)&(pParamHdlr[chNo]->IqphStillCcAddr[ccIdx]);            
        default:
            //printk("unknown type");
            return 0;
        }
    }
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: _Init_Adj_Video_Color_Table_Addr
 *
 *  @Description:: Initialize Video Adj table Cc table address
 *
 *  @Input      ::
 *    None
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    int : 0(OK)/-1(NG)
\*-----------------------------------------------------------------------------------------------*/
int _Init_Adj_Video_Color_Table_Addr(UINT32 chNo) {
    int i, CcIdx, CcType;
    ADJ_VIDEO_PARAM_s *pAdjVideo = 0;
//    ADJ_VIDEO_HISO_PARAM_s *pAdjVideoHiso = 0;

      
    int tableNum = 0;

    /* Assign VIDEO ADJ color table address */
    //pAdjVideo = AmbaIQParam_Get_Adj_Video_Param(GVideoLoaded,tableNum);
    pAdjVideo = &(pParamHdlr[chNo]->IqphAdjVideo[tableNum]);
    if(pAdjVideo){
        for (i=0; i<ADJ_VIDEO_TABLE_VALID_NO; i++) {
                if (pAdjVideo) {
                    CcType = pAdjVideo[i].FilterParam.Def.Color.Type;
                    for (CcIdx=0; CcIdx<DEF_CC_TABLE_NO; CcIdx++) {
                        pAdjVideo[i].FilterParam.Def.Color.Table[CcIdx].MatrixThreeDTableAddr =
                            _Get_Cc_Table_Addr(chNo,CcType, CcIdx);
                    }
                }
            }
    }    
     
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: _Init_Adj_Still_Color_Table_Addr
 *
 *  @Description:: Initialize Still Adj table Cc table address
 *
 *  @Input      ::
 *    None
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    int : 0(OK)/-1(NG)
\*-----------------------------------------------------------------------------------------------*/
int _Init_Adj_Still_Color_Table_Addr(UINT32 chNo) {

    int CcIdx, CcType, LutIdx;
    ADJ_STILL_LOW_ISO_PARAM_s *pAdjStillLiso = 0;
    ADJ_STILL_HISO_PARAM_s *pAdjStillHiso = 0;

    /* Assign STILL ADJ color table address */
    /* Low-ISO mode */
    //pAdjStillLiso = AmbaIQParam_Get_Adj_Still_Low_Iso_Param(PARAM_LOADED, 0); //FIXME
    pAdjStillLiso = &(pParamHdlr[chNo]->IqphAdjLisoStill[0]);
    if (pAdjStillLiso) {
        for(LutIdx = 0; LutIdx < ADJ_STILL_LISO_TABLE_VALID_NO; LutIdx++) {
            CcType = pAdjStillLiso[LutIdx].Def.Color.Type;
            for (CcIdx=0; CcIdx<DEF_CC_TABLE_NO; CcIdx++) {
                pAdjStillLiso[LutIdx].Def.Color.Table[CcIdx].MatrixThreeDTableAddr =
                _Get_Cc_Table_Addr(chNo,CcType, CcIdx);
            }
        }
    }


	/* High-ISO mode */
    //pAdjStillHiso = AmbaIQParam_Get_Adj_Still_High_Iso_Param(PARAM_LOADED, 0); //FIXME
    pAdjStillHiso = &(pParamHdlr[chNo]->IqphAdjHisoStill[0]);
    if (pAdjStillHiso) {
        for(LutIdx = 0; LutIdx < ADJ_STILL_HISO_TABLE_VALID_NO; LutIdx++) {
            CcType = pAdjStillHiso[LutIdx].Def.Color.Type;
            for (CcIdx=0; CcIdx<5; CcIdx++) {
                pAdjStillHiso[LutIdx].Def.Color.Table[CcIdx].MatrixThreeDTableAddr =
               _Get_Cc_Table_Addr(chNo,CcType, CcIdx);
            }
        }
    }



    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: _Init_Color_Table_Addr
 *
 *  @Description:: Initialize Cc table address
 *
 *  @Input      ::
 *    UINT8 modeSwitch : mode switch
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    int : 0(OK)/-1(NG)
\*-----------------------------------------------------------------------------------------------*/
int _Init_Color_Table_Addr(UINT32 chNo)
{
    int i, CcIdx, CcType;
    IMG_PARAM_s *Img = 0;
    ADJ_PHOTO_PARAM_s *AdjPhoto = 0;

    //Img = AmbaIQParam_Get_Img_Param(PARAM_LOADED);
    Img= &(pParamHdlr[chNo]->IqphImg);
    if (Img) {
    /* Assign Image Param color table address */
        CcIdx = 1; // use index 1 as default
        CcType = IMG_MODE_VIDEO;
        Img->ColorCorrVideo.MatrixThreeDTableAddr =
            _Get_Cc_Table_Addr(chNo,CcType, CcIdx);
        CcType = IMG_MODE_STILL;
        Img->ColorCorrStill.MatrixThreeDTableAddr =
            _Get_Cc_Table_Addr(chNo,CcType, CcIdx);
    }

    /* Assign PHOTO ADJ color table address */
    //AdjPhoto = AmbaIQParam_Get_Adj_Photo_Param(PARAM_LOADED, 0); //FIXME
    AdjPhoto = &(pParamHdlr[chNo]->IqphAdjPhoto[0]);
    for (i=0; i< ADJ_PHOTO_TABLE_VALID_NO; i++) {
        if (AdjPhoto) {
            CcType = AdjPhoto[i].FilterParam.Def.Color.Type;
        for (CcIdx = 0; CcIdx < DEF_CC_TABLE_NO; CcIdx++) {
            AdjPhoto[i].FilterParam.Def.Color.Table[CcIdx].MatrixThreeDTableAddr =
            _Get_Cc_Table_Addr(chNo,CcType, CcIdx);
        }
        }
    }

    _Init_Adj_Video_Color_Table_Addr(chNo);
    _Init_Adj_Still_Color_Table_Addr(chNo);

    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: _Load_Still_Color_Table
 *
 *  @Description:: Load Still Color table
 *
 *  @Input      ::
 *    int colorIdx : color table index
 *    int post     :
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    int : 0(OK)/-1(NG)
\*-----------------------------------------------------------------------------------------------*/
int _Load_Still_Color_Table(UINT32 chNo,int colorIdx, int post)
{
    //static int              PreColorIdx1 = 16383;
    int                     RVal = -1;
    int                     i, FSize;
    COLOR_TABLE_PATH_s      *OneColorCcPath;

    OneColorCcPath = pParamHdlr[chNo]->IqphParamProxy.GetColorTablePath();

    for (i=0; i< DEF_CC_TABLE_NO; i++) {
        FSize = AmbaROM_GetSize(AMBA_ROM_SYS_DATA, OneColorCcPath[colorIdx].Still[i], 0x0);
        RVal = AmbaROM_LoadByName(AMBA_ROM_SYS_DATA, OneColorCcPath[colorIdx].Still[i], pParamHdlr[chNo]->IqphStillCcAddr[i], FSize, 0);
    if (RVal == -1) {
            AmbaIqParamHandlerPrintk("load still cc 3D from ROMFS error, %d", i);
    }else {
        AmbaIqParamHandlerPrintk("load still cc 3D %s from ROMFS: %d", OneColorCcPath[colorIdx].Still[i], FSize);
    }
    }

    return RVal;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: _Load_Video_Color_Table
 *
 *  @Description:: Load Video Color table
 *
 *  @Input      ::
 *    int colorIdx : color table index
 *    int post     :
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    int : 0(OK)/-1(NG)
\*-----------------------------------------------------------------------------------------------*/
int _Load_Video_Color_Table(UINT32 chNo,int colorIdx, int post)
{
    //static int              PreColorIdx0 = 16383;
    int                     RVal = -1;
    int                     i, FSize;
    COLOR_TABLE_PATH_s      *OneColorCcPath;

    OneColorCcPath = pParamHdlr[chNo]->IqphParamProxy.GetColorTablePath();

    for (i=0; i< DEF_CC_TABLE_NO; i++) {
        FSize = AmbaROM_GetSize(AMBA_ROM_SYS_DATA, OneColorCcPath[colorIdx].Video[i], 0x0);
        RVal = AmbaROM_LoadByName(AMBA_ROM_SYS_DATA, OneColorCcPath[colorIdx].Video[i], pParamHdlr[chNo]->IqphVideoCcAddr[i], FSize, 0);
        if (RVal == -1) {
            AmbaIqParamHandlerPrintk("load video cc 3D from ROMFS error, %d", i);
    }else {
        AmbaIqParamHandlerPrintk("load video cc 3D %s from ROMFS: %d", OneColorCcPath[colorIdx].Video[i], FSize);
    }
    }

    _Load_Still_Color_Table(chNo,colorIdx,0);

    return RVal;
}

int  AmbaIQParam_Get_ColorIdx_By_De(UINT32 chNo, int deMode)
{
    int ColorIdx;


    if (deMode >= DIGITAL_LAST) {
        AmbaPrint("digital_effect %d exceeds max. digital effect", deMode);
        return -1;
    }

    ColorIdx = pParamHdlr[chNo]->IqphCcDeVideoTable[deMode];
    IqphDeCc3d = ColorIdx;
    if (IqphDeCc3d == SYSTEM_DEFAULT && IqphSceneModeCc3d == SYSTEM_DEFAULT) {
        ColorIdx = 0;
    }else if (IqphDeCc3d == SYSTEM_DEFAULT && IqphSceneModeCc3d != SYSTEM_DEFAULT) {
        ColorIdx = IqphSceneModeCc3d;
    }
    AmbaPrint("Digital Effect CC table : %d",ColorIdx);

    return ColorIdx;
}




int AmbaIQParam_DigitalEffect_Load_Color_Table(UINT32 chNo, int DigitalEffect, int post, UINT8 modeSwitch)
{
        int RVal = -1;
        int ColorIdx = 0;

    ColorIdx = AmbaIQParam_Get_ColorIdx_By_De(chNo, DigitalEffect);
    RVal = _Load_Video_Color_Table(chNo, ColorIdx, post);
	// If user switch digital effect or scene mode then it need to load cc still table
    if (modeSwitch)
            _Load_Still_Color_Table(chNo, ColorIdx,0);
    return RVal;
}

int _Load_Cc_Reg_Table(UINT32 chNo)
{
    int                     RVal = -1;
    int                     FSize;

    FSize = AmbaROM_GetSize(AMBA_ROM_SYS_DATA, (pParamHdlr[chNo]->IqphCcRegID), 0x0);
    RVal = AmbaROM_LoadByName(AMBA_ROM_SYS_DATA, (pParamHdlr[chNo]->IqphCcRegID), &(pParamHdlr[chNo]->IqphCcReg[0]), FSize, 0);

    if (RVal == -1) {
        AmbaIqParamHandlerPrintk("load video Cc Reg from ROMFS error");
    }else {
        AmbaIqParamHandlerPrintk("load Cc Reg %s from ROMFS: %d, RVal :%d", (pParamHdlr[chNo]->IqphCcRegID), FSize, RVal);
    }
    AmbaImg_Proc_Cmd(MW_IP_SET_CC_REG_ADD, 0, (UINT32)&(pParamHdlr[chNo]->IqphCcReg[0]), 0);
    return RVal;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParam_Register
 *
 *  @Description:: Register parameter proxy
 *
 *  @Input      ::
 *    None
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    int : 0(OK)/-1(NG)
\*-----------------------------------------------------------------------------------------------*/
int AmbaIQParam_Register(UINT32 chNo, int sensorID)
{
    int RVal = -1;

    extern int AmbaIQParamImx117_A9_Register(UINT32 chNo);   
    extern int AmbaIQParamImx144_A9_Register(UINT32 chNo);      
    
    switch(sensorID){
#ifdef CONFIG_SENSOR_SONY_IMX117
        case SESNOR_IMX117:
            RVal = AmbaIQParamImx117_A9_Register(chNo);	
            break;
#endif
#ifdef CONFIG_SENSOR_SONY_IMX144	    
        case SESNOR_IMX144:
            RVal = AmbaIQParamImx144_A9_Register(chNo);	
            break;            
#endif
#ifdef CONFIG_SENSOR_SONY_IMX183	    
        case SESNOR_IMX183:
            RVal = AmbaIQParamImx183_A9_Register(chNo);	
            break;            
#endif		    
        default:
            break;    
    }
   
    return RVal;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParam_Remove
 *
 *  @Description:: Remove the Param Proxy instance.
 *
 *  @Input      ::
 *    None
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    int : 0(OK)/-1(NG)
\*-----------------------------------------------------------------------------------------------*/
int AmbaIQParam_Remove(UINT32 chNo)
{
    memset(&(pParamHdlr[chNo]->IqphParamProxy), 0x0, sizeof(PARAM_PROXY_s));
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParam_Attach
 *
 *  @Description:: Attach the Sensor input device and enable the device control.
 *
 *  @Input      ::
 *    PARAM_PROXY_s *par : pionter to param proxy
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    int : 0(OK)/-1(NG)
\*-----------------------------------------------------------------------------------------------*/
int AmbaIQParam_Attach(UINT32 chNo, PARAM_PROXY_s *par)
{
    if (par == NULL)
        return -1;

    AmbaIQParam_Remove(chNo);
    memcpy(&pParamHdlr[chNo]->IqphParamProxy, par, sizeof(PARAM_PROXY_s));
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParam_Get_By_Name
 *
 *  @Description:: Attach the Sensor input device and enable the device control.
 *
 *  @Input      ::
 *    const char *name : Pointer to param name
 *    const int type : Param type
 *
 *  @Output     ::
 *    IP_PARAM_s **param : Pointer to IP param
 *
 *
 *  @Return     ::
 *    int : 0(OK)/-1(NG)
\*-----------------------------------------------------------------------------------------------*/
int AmbaIQParam_Get_By_Name(UINT32 chNo, const char *name, const int type, IP_PARAM_s **param)
{
    int i;

    *param = NULL;

//    for (i = 0; GIpParam[i].Length >= 0; i++) {
//        if (strcmp(name, GIpParam[i].Name) == 0) {
//            if (type != GIpParam[i].Type) {
//                AmbaPrint("AmbaIQParam_Get_By_Name: type mismatch");
//                return -1;
//            }
//            *param = &GIpParam[i];
//            return 0;
//        }
//    }

    
    for (i = 0; pGIpParam[chNo][i].Length >= 0; i++) {
        if (strcmp(name, pGIpParam[chNo][i].Name) == 0) {
            if (type != pGIpParam[chNo][i].Type) {
                AmbaPrint("AmbaIQParam_Get_By_Name: type mismatch");
                return -1;
            }
            *param = &(pGIpParam[chNo][i]);
            return 0;
        }
    }



    return -1;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: _Init_Color_Scene_Table
 *
 *  @Description:: init color scene table
 *
 *  @Input      ::
 *    None
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    int : 0(OK)/-1(NG)
\*-----------------------------------------------------------------------------------------------*/
int _Init_Color_Scene_Table(UINT32 chNo)
{
    int SceneIdx;
    SCENE_DATA_s *Scene;

    //Scene = AmbaIQParam_Get_Scene_Data(PARAM_LOADED,0);
    Scene = &(pParamHdlr[chNo]->IqphSceneData[0]);

    for(SceneIdx = 0;SceneIdx<SCENE_TABLE_TATOL_NO;SceneIdx++){
        pParamHdlr[chNo]->IqphCcSceneTable[SceneIdx] = Scene[SceneIdx].Def.ColorTable;  
    }
    
//    _Post_Init_Color_Scene_Table();

    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParam_Get_Color_Idx_By_Scene
 *
 *  @Description:: get cc table index for scene mode
 *
 *  @Input      ::
*    int sceneMode: scene mode
 *
 *  @Output     ::
*   static	UINT16 IqphSceneModeCc3d : cc table index
 *
 *
 *  @Return     ::
 *    int : 0(OK)/-1(NG)
\*-----------------------------------------------------------------------------------------------*/
int AmbaIQParam_Get_Color_Idx_By_Scene(UINT32 chNo, int sceneMode)
{
    int ColorIdx;

    if (sceneMode == SCENE_AUTO) {
        sceneMode = SCENE_OFF;
    }
    if (sceneMode >= SCENE_LAST) {
        AmbaPrint("sceneMode %d exceeds max. scene mode", sceneMode);
        return -1;
    }

    IqphSceneModeCc3d = pParamHdlr[chNo]->IqphCcSceneTable[sceneMode];
    ColorIdx = IqphSceneModeCc3d;

      if (IqphDeCc3d != SYSTEM_DEFAULT) {
          ColorIdx = IqphDeCc3d;
      }else if (IqphSceneModeCc3d == SYSTEM_DEFAULT) {
          ColorIdx = 0;
      }

    AmbaPrint("Scene Mode CC table : %d",ColorIdx);

    return ColorIdx;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParam_Scene_Mode_Load_Color_Table
 *
 *  @Description:: load  cc table index
 *
 *  @Input      ::
 *    int sceneMode: select scene mode
 *    int post:
 *    UINT8 modeSwitch:select mode
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    int : 0(OK)/-1(NG)
\*-----------------------------------------------------------------------------------------------*/
int AmbaIQParam_Scene_Mode_Load_Color_Table(UINT32 chNo, int sceneMode, int post, UINT8 modeSwitch)
{
    int Rval = -1;
    int ColorIdx = 0;

    ColorIdx = AmbaIQParam_Get_Color_Idx_By_Scene(chNo, sceneMode);
    Rval = _Load_Video_Color_Table(chNo,ColorIdx, post);
	// If user switch digital effect or scene mode then it need to load cc still table
    if (modeSwitch)
    _Load_Still_Color_Table(chNo,ColorIdx,0);

    return Rval;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParam_Init_Param_Proxy
 *
 *  @Description:: Initialize parameter proxy
 *
 *  @Input      ::
 *    UINT8 modeSwitch : mode switch
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    int : 0(OK)/-1(NG)
\*-----------------------------------------------------------------------------------------------*/

int AmbaIQParam_Init_Param_Mem(UINT32 chNo);

int AmbaIQParam_Init_Param_Proxy(UINT32 chNo)
{
 
    AmbaIQParam_Init_Param_Mem(chNo);  
    //_Init_Param(modeSwitch);
    _Init_Color_Table_Addr(chNo);
    _Load_Cc_Reg_Table(chNo);
    _Load_Video_Color_Table(chNo,0, 0);// Test
    _Init_Color_Scene_Table(chNo);
    _Init_Color_De_Video_Table(chNo);

    return 0;
}

void _Init_GIpParam_Flag_Test(GIpParam_Info_s *pInfo)
{
    UINT16 i;
    for(i=0;i<GIpParamCategoryNum;i++) {
        pInfo->GIpParamCategoryMaxNum[i] =1;
        pInfo->GIpParamValidTableFlag[i] =1;
    }

    pInfo->GIpParamCategoryMaxNum[GIpParamCategory_ADJ_STILL]= ADJ_STILL_LISO_TABLE_MAX_NO;
    pInfo->GIpParamCategoryMaxNum[GIpParamCategory_ADJ_HISO_STILL]= ADJ_STILL_HISO_TABLE_MAX_NO;
    pInfo->GIpParamCategoryMaxNum[GIpParamCategory_ADJ_VIDEO]= ADJ_VIDEO_TABLE_MAX_NO;
    pInfo->GIpParamCategoryMaxNum[GIpParamCategory_ADJ_PHOTO]= ADJ_PHOTO_TABLE_MAX_NO;
    pInfo->GIpParamCategoryMaxNum[GIpParamCategory_SCENE]= SCENE_TABLE_MAX_NO;

    pInfo->GIpParamValidTableFlag[GIpParamCategory_ADJ_STILL]= ADJ_STILL_LISO_TABLE_VALID_NO;
    pInfo->GIpParamValidTableFlag[GIpParamCategory_ADJ_HISO_STILL]= ADJ_STILL_HISO_TABLE_VALID_NO;
    pInfo->GIpParamValidTableFlag[GIpParamCategory_ADJ_VIDEO]= ADJ_VIDEO_TABLE_VALID_NO;
    pInfo->GIpParamValidTableFlag[GIpParamCategory_ADJ_PHOTO]= ADJ_PHOTO_TABLE_VALID_NO;
    pInfo->GIpParamValidTableFlag[GIpParamCategory_SCENE]= SCENE_TABLE_VALID_NO;
}

//extern AMBA_KAL_BYTE_POOL_t G_MMPL;
extern AMBA_KAL_BYTE_POOL_t  G_NC_MMPL;

void _Assgin_Data_Addr(UINT32 chNo, IP_PARAM_s *pGIpParamTmp)
{
    (pGIpParamTmp+0)->Data  = &(pParamHdlr[chNo]->IqphImg);
    (pGIpParamTmp+1)->Data  = &(pParamHdlr[chNo]->IqphAaaDefault);     
    (pGIpParamTmp+2)->Data  = &(pParamHdlr[chNo]->IqphAdjLisoStill[0]);
    (pGIpParamTmp+3)->Data  = &(pParamHdlr[chNo]->IqphAdjLisoStill[1]);  
    (pGIpParamTmp+4)->Data  = &(pParamHdlr[chNo]->IqphAdjHisoStill[0]);      
    (pGIpParamTmp+5)->Data  = &(pParamHdlr[chNo]->IqphAdjVideo[0]);    
    (pGIpParamTmp+6)->Data  = &(pParamHdlr[chNo]->IqphAdjVideo[1]);         
    (pGIpParamTmp+7)->Data  = &(pParamHdlr[chNo]->IqphAdjPhoto[0]);    
    (pGIpParamTmp+8)->Data  = &(pParamHdlr[chNo]->IqphAdjPhoto[1]);    
    (pGIpParamTmp+9)->Data  = &(pParamHdlr[chNo]->IqphAdjStillIdx);    
    (pGIpParamTmp+10)->Data = &(pParamHdlr[chNo]->IqphSceneData[SCENE_TABLE_CONTAIN_SETS*0]);    
    (pGIpParamTmp+11)->Data = &(pParamHdlr[chNo]->IqphSceneData[SCENE_TABLE_CONTAIN_SETS*1]);     
    (pGIpParamTmp+12)->Data = &(pParamHdlr[chNo]->IqphSceneData[SCENE_TABLE_CONTAIN_SETS*2]);    
    (pGIpParamTmp+13)->Data = &(pParamHdlr[chNo]->IqphSceneData[SCENE_TABLE_CONTAIN_SETS*3]);         
    (pGIpParamTmp+14)->Data = &(pParamHdlr[chNo]->IqphSceneData[SCENE_TABLE_CONTAIN_SETS*4]);    
    (pGIpParamTmp+15)->Data = &(pParamHdlr[chNo]->IqphDeVideo);    
    (pGIpParamTmp+16)->Data = &(pParamHdlr[chNo]->IqphDeStill);    
    (pGIpParamTmp+17)->Data = &(pParamHdlr[chNo]->IqphAdjTable);        
   
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParam_Allocate_Param_Mem
 *
 *  @Description:: Allocate parameter memory
 *
 *  @Input      ::
 *    UINT32 chNo : chNo
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    int : 0(OK)/-1(NG)
\*-----------------------------------------------------------------------------------------------*/
static PARAM_HANDLER_s IQParamHdlr;
int AmbaIQParam_Allocate_Param_Mem(UINT32 chNo)
{
    int RVal = 0;
    
    //RVal = AmbaKAL_BytePoolAllocate(&G_MMPL, (void **)&pParamHdlr[chNo], sizeof(PARAM_HANDLER_s), 100);
    //RVal = AmbaKAL_BytePoolAllocate(&G_NC_MMPL, (void **)&pParamHdlr[chNo], sizeof(PARAM_HANDLER_s), 100);
    pParamHdlr[chNo] = &IQParamHdlr;
    if(RVal == 0){
        memset(pParamHdlr[chNo], 0, sizeof(PARAM_HANDLER_s));
        pParamHdlr[chNo]->IqphSceneModeCc3d = SYSTEM_DEFAULT;
        pParamHdlr[chNo]->IqphDeCc3d = SYSTEM_DEFAULT;
        pParamHdlr[chNo]->IqphCcRegID = "CC_Reg.bin";
    }
    //Init table address API
    AmbaPrint("<%s>, RVal = %d, chNo : %d, size : %d", __FUNCTION__, RVal, chNo, sizeof(PARAM_HANDLER_s));
  
    return (RVal);    
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParam_Init_Param_Mem
 *
 *  @Description:: Initialize parameter memory
 *
 *  @Input      ::
 *    UINT32 chCount : chCount
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    int : 0(OK)/-1(NG)
\*-----------------------------------------------------------------------------------------------*/
int AmbaIQParam_Init_Param_Mem(UINT32 chNo)
{
    UINT16 i,j,k; 
    int RVal = 0;
    
    _Init_GIpParam_Flag_Test(GIpParamInfo);
    _Init_GIpParam_Flag_Test(&GIpParamInfo[1]);

    pGIpParam[0] = GIpParam_tmp_1;
    pGIpParam[1] = GIpParam_tmp_2;
    // Init GIpP aram     
    _Assgin_Data_Addr(chNo,pGIpParam[chNo]);

    i=0;
    for(j=0;j<GIpParamCategoryNum;j++) {
        for(k=0;k<GIpParamInfo[chNo].GIpParamCategoryMaxNum[j];k++) {
            if (k<GIpParamInfo[chNo].GIpParamValidTableFlag[j]) {
                _Validate_Param(0,&pGIpParam[chNo][i],k);  
            }
            i++;            
        }
    }
    return (RVal); 
}

int AmbaIQParam_Free_Param_Mem(PARAM_HANDLER_s *pParamHdlr)
{
    int RVal = 0;
    
    RVal = AmbaKAL_BytePoolFree(pParamHdlr);
    return (RVal);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////

