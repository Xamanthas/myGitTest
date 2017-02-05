/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmpU_Image.c
 *
 *  @Copyright      :: Copyright (C) 2013 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: app init parameters function
 *
 *  @History        ::
 *      Date        Name             Comments
 *      01/08/2006  Wei-Kang Che     Created
 *
 \*-------------------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "AmbaDataType.h"
#include "AmbaPrint.h"

#include "AmbaImg_Proc.h"
#include "AmbaImg_AeAwb.h"
#include "AmbaSample_AeAwbAdj.h"
#include "AmbaImg_Impl_Cmd.h"
#include "AmbaROM.h"


#ifdef CONFIG_SOC_A9
    #include "../sample_iqparam_handler/a9/AmbaIQParamHandler.h"
#endif
#ifdef CONFIG_SOC_A12
    #include "../sample_iqparam_handler/a12/AmbaIQParamHandler.h"
#endif

static UINT8 ImageInitFlg[MAX_CHAN_NUM] = {0,0,0,0,0,0,0,0,0,0};

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: App_Image_Init_Image_Params
 *
 *  @Description:: Entry point of inital IQ parameters
 *
 *  @Input      ::
 *    const char *name : inital module string
 *    UINT32 type      : IMG_PARAM (0), AAA_PARAM  (1), ADJ_PARAM_VIDEO (2), ADJ_PARAM_PHOTO (3), ADJ_PARAM_LISO_STILL (4)
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    UINT32 RVal      : retrun 0 means command done
\*-----------------------------------------------------------------------------------------------*/
UINT32 App_Image_Init_Image_Params(UINT32 chNo, UINT32 ParamHandlerNo, const char *name, UINT32 type)
{
    UINT32 RVal;
    IP_PARAM_s *IpParamTmp = NULL;

    AmbaIQParam_Get_By_Name(ParamHandlerNo,name, type, &IpParamTmp);
    RVal = AmbaImg_Proc_Cmd(MW_IP_CHK_IQ_PARAM_VER, (UINT32) chNo, IQ_PARAMS_IMG_DEF, (UINT32)IpParamTmp->Data);
    if (IpParamTmp && (RVal == 0)) {
        RVal = AmbaImg_Proc_Cmd(MW_IP_SET_IMG_PARAM_ADD, chNo, (UINT32)IpParamTmp->Data, 0);
    }
    return RVal;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: App_Image_Init_AAA_Params
 *
 *  @Description:: Entry point of inital AAA parameters
 *
 *  @Input      ::
 *    const char *name : inital module string
 *    UINT32 type      : IMG_PARAM (0), AAA_PARAM  (1), ADJ_PARAM_VIDEO (2), ADJ_PARAM_PHOTO (3), ADJ_PARAM_LISO_STILL (4)
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    UINT32 RVal      : retrun 0 means command done
\*-----------------------------------------------------------------------------------------------*/
UINT32 App_Image_Init_AAA_Params(UINT32 chNo, UINT32 ParamHandlerNo, const char *name, UINT32 type)
{
    UINT32 RVal;
    IP_PARAM_s *IpParamTmp = NULL;

    AmbaIQParam_Get_By_Name(ParamHandlerNo,name, type, &IpParamTmp);
    RVal = AmbaImg_Proc_Cmd(MW_IP_CHK_IQ_PARAM_VER, (UINT32) chNo, IQ_PARAMS_AAA, (UINT32)IpParamTmp->Data);
    if (IpParamTmp && (RVal == 0)) {
        RVal = AmbaImg_Proc_Cmd(MW_IP_SET_AAA_PARAM, (UINT32)chNo, (UINT32)IpParamTmp->Data, 0);
    }
    return RVal;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: App_Image_Init_AdjTable_Param
 *
 *  @Description:: Entry point of inital Adj Table parameters
 *
 *  @Input      ::
 *    const char *name : inital module string
 *    UINT32 type      : IMG_PARAM (0), AAA_PARAM  (1), ADJ_PARAM_VIDEO (2), ADJ_PARAM_PHOTO (3), ADJ_PARAM_LISO_STILL (4)
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    UINT32 RVal      : retrun 0 means command done
\*-----------------------------------------------------------------------------------------------*/
UINT32 App_Image_Init_AdjTable_Param(UINT32 chNo, UINT32 ParamHandlerNo, const char *name, UINT32 type)
{
    UINT32 RVal = -1;
    IP_PARAM_s *IpParamTmp = NULL;

    AmbaIQParam_Get_By_Name(ParamHandlerNo,name, type, &IpParamTmp);
    if (IpParamTmp) {
        RVal = AmbaImg_Proc_Cmd(MW_IP_SET_ADJTABLE_PARAM, (UINT32)chNo, (UINT32)IpParamTmp->Data, 0);
    }
    return RVal;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: App_Image_Init_Adj_Params
 *
 *  @Description:: Entry point of inital ADJ parameters
 *
 *  @Input      ::
 *    const char *name : inital module string
 *    UINT32 type      : IMG_PARAM (0), AAA_PARAM  (1), ADJ_PARAM_VIDEO (2), ADJ_PARAM_PHOTO (3), ADJ_PARAM_LISO_STILL (4)
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    UINT32 RVal      : retrun 0 means command done
\*-----------------------------------------------------------------------------------------------*/
UINT32 App_Image_Init_Adj_Params(UINT32 chNo, UINT32 ParamHandlerNo, const char *name, UINT32 type)
{
    UINT32 RVal = -1;
    IP_PARAM_s *IpParamTmp = NULL;

    switch (type) {
        case ADJ_PARAM_VIDEO:

            AmbaIQParam_Get_By_Name(ParamHandlerNo,name, ADJ_PARAM_VIDEO, &IpParamTmp);
            RVal = AmbaImg_Proc_Cmd(MW_IP_CHK_IQ_PARAM_VER, (UINT32) chNo, IQ_PARAMS_VIDEO_ADJ, (UINT32)IpParamTmp->Data);
            if (RVal == 0) {
            RVal = AmbaImg_Proc_Cmd(MW_IP_SET_ADJ_PARAMS_ADD, (UINT32) chNo, IQ_PARAMS_VIDEO_ADJ, (UINT32)IpParamTmp->Data);
            }
            break;
#ifdef CONFIG_SOC_A12
        case ADJ_PARAM_HISO_VIDEO:

            AmbaIQParam_Get_By_Name(ParamHandlerNo,name, ADJ_PARAM_HISO_VIDEO, &IpParamTmp);
            RVal = AmbaImg_Proc_Cmd(MW_IP_CHK_IQ_PARAM_VER, (UINT32) chNo, IQ_PARAMS_VIDEO_HISO_ADJ, (UINT32)IpParamTmp->Data);
            if (RVal == 0) {
            RVal = AmbaImg_Proc_Cmd(MW_IP_SET_ADJ_PARAMS_ADD, (UINT32) chNo, IQ_PARAMS_VIDEO_HISO_ADJ, (UINT32)IpParamTmp->Data);
            }
            break;
#endif
        case ADJ_PARAM_PHOTO:

            AmbaIQParam_Get_By_Name(ParamHandlerNo,name, ADJ_PARAM_PHOTO, &IpParamTmp);
            RVal = AmbaImg_Proc_Cmd(MW_IP_CHK_IQ_PARAM_VER, (UINT32) chNo, IQ_PARAMS_PHOTO_ADJ, (UINT32)IpParamTmp->Data);
            if (RVal == 0) {
                RVal = AmbaImg_Proc_Cmd(MW_IP_SET_ADJ_PARAMS_ADD, (UINT32) chNo, IQ_PARAMS_PHOTO_ADJ, (UINT32)IpParamTmp->Data);
            }
            break;

        case ADJ_PARAM_LISO_STILL:

            AmbaIQParam_Get_By_Name(ParamHandlerNo,name, ADJ_PARAM_LISO_STILL, &IpParamTmp);
            RVal = AmbaImg_Proc_Cmd(MW_IP_CHK_IQ_PARAM_VER, (UINT32) chNo, IQ_PARAMS_STILL_LISO_ADJ, (UINT32)IpParamTmp->Data);
            if (RVal == 0) {
                RVal = AmbaImg_Proc_Cmd(MW_IP_SET_ADJ_PARAMS_ADD, (UINT32) chNo, IQ_PARAMS_STILL_LISO_ADJ, (UINT32)IpParamTmp->Data);
            }
            break;

        case ADJ_PARAM_HISO_STILL:

            AmbaIQParam_Get_By_Name(ParamHandlerNo,name, ADJ_PARAM_HISO_STILL, &IpParamTmp);
            RVal = AmbaImg_Proc_Cmd(MW_IP_CHK_IQ_PARAM_VER, (UINT32) chNo, IQ_PARAMS_STILL_HISO_ADJ, (UINT32)IpParamTmp->Data);
            if (RVal == 0) {
                RVal = AmbaImg_Proc_Cmd(MW_IP_SET_ADJ_PARAMS_ADD, (UINT32) chNo, IQ_PARAMS_STILL_HISO_ADJ, (UINT32)IpParamTmp->Data);
            }
            break;

        case ADJ_PARAM_STILL_IDX:

            AmbaIQParam_Get_By_Name(ParamHandlerNo,name, ADJ_PARAM_STILL_IDX, &IpParamTmp);
            RVal = AmbaImg_Proc_Cmd(MW_IP_CHK_IQ_PARAM_VER, (UINT32) chNo, IQ_PARAMS_STILL_IDX_INFO_ADJ, (UINT32)IpParamTmp->Data);
            if (RVal == 0) {
                RVal = AmbaImg_Proc_Cmd(MW_IP_SET_ADJ_PARAMS_ADD, (UINT32) chNo, IQ_PARAMS_STILL_IDX_INFO_ADJ, (UINT32)IpParamTmp->Data);
            }
            break;
#ifdef CONFIG_SOC_A12
        case ADJ_PARAM_VIDEO_IDX:

            AmbaIQParam_Get_By_Name(ParamHandlerNo,name, ADJ_PARAM_VIDEO_IDX, &IpParamTmp);
            RVal = AmbaImg_Proc_Cmd(MW_IP_CHK_IQ_PARAM_VER, (UINT32) chNo, IQ_PARAMS_VIDEO_IDX_INFO_ADJ, (UINT32)IpParamTmp->Data);
            if (RVal == 0) {
                RVal = AmbaImg_Proc_Cmd(MW_IP_SET_ADJ_PARAMS_ADD, (UINT32) chNo, IQ_PARAMS_VIDEO_IDX_INFO_ADJ, (UINT32)IpParamTmp->Data);
            }
            break;
#endif
        default:
            AmbaPrint("Unknown ADJ param type");
              break;
    }
    return RVal;

}


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: App_Image_Init_Scene_Params
 *
 *  @Description:: Entry point of inital Scene parameters
 *
 *  @Input      ::
 *    const char *name : inital module string
 *    UINT32 type      : SCENE_DATA_S01, SCENE_DATA_S02, SCENE_DATA_S03, SCENE_DATA_S04, SCENE_DATA_S05
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    UINT32 RVal      : retrun 0 means command done
\*-----------------------------------------------------------------------------------------------*/
UINT32 App_Image_Init_Scene_Params(UINT32 chNo, UINT32 ParamHandlerNo, const char *name, UINT32 type)
{
    UINT32 RVal = 0;
    UINT16 SceneSetCount;
    UINT16 SceneSetMaxNum = 8;
    IP_PARAM_s *IpParamTmp = NULL;


    AmbaIQParam_Get_By_Name(ParamHandlerNo,name, type, &IpParamTmp);

//    if (IpParamTmp) {
//        return AmbaImg_Proc_Cmd(MW_IP_SET_SCENE_MODE_INFO, SceneMode, (UINT32)IpParamTmp->Data, 0);
//    }
    if (IpParamTmp) {
        if (strcmp(name, "scene_data_s01") == 0) {
            for (SceneSetCount=0; SceneSetCount<SceneSetMaxNum;SceneSetCount++) {
                RVal |= AmbaImg_Proc_Cmd(MW_IP_SET_SCENE_MODE_INFO, SceneSetCount, (((UINT32)IpParamTmp->Data)+SceneSetCount*sizeof(SCENE_DATA_s)), 0);
            }
        } else if (strcmp(name, "scene_data_s02") == 0) {
            for (SceneSetCount=0; SceneSetCount<SceneSetMaxNum;SceneSetCount++) {
                RVal |= AmbaImg_Proc_Cmd(MW_IP_SET_SCENE_MODE_INFO, (SceneSetCount+SceneSetMaxNum*1), (((UINT32)IpParamTmp->Data)+SceneSetCount*sizeof(SCENE_DATA_s)), 0);
            }
        } else if (strcmp(name, "scene_data_s03") == 0) {
            for (SceneSetCount=0; SceneSetCount<SceneSetMaxNum;SceneSetCount++) {
                RVal |= AmbaImg_Proc_Cmd(MW_IP_SET_SCENE_MODE_INFO, (SceneSetCount+SceneSetMaxNum*2), (((UINT32)IpParamTmp->Data)+SceneSetCount*sizeof(SCENE_DATA_s)), 0);
            }
        } else if (strcmp(name, "scene_data_s04") == 0) {
            for(SceneSetCount=0; SceneSetCount<SceneSetMaxNum;SceneSetCount++) {
                RVal |= AmbaImg_Proc_Cmd(MW_IP_SET_SCENE_MODE_INFO, (SceneSetCount+SceneSetMaxNum*3), (((UINT32)IpParamTmp->Data)+SceneSetCount*sizeof(SCENE_DATA_s)), 0);
            }
        } else if (strcmp(name, "scene_data_s05") == 0) {
            for (SceneSetCount=0; SceneSetCount<SceneSetMaxNum;SceneSetCount++) {
                RVal |= AmbaImg_Proc_Cmd(MW_IP_SET_SCENE_MODE_INFO, (SceneSetCount+SceneSetMaxNum*4), (((UINT32)IpParamTmp->Data)+SceneSetCount*sizeof(SCENE_DATA_s)), 0);
            }
        }
    }

    return RVal;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: App_Image_Init_De_Params
 *
 *  @Description:: Entry point of inital DE parameters
 *
 *  @Input      ::
 *    const char *name : inital module string
 *    UINT32 type      :
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    UINT32 RVal      : retrun 0 means command done
\*-----------------------------------------------------------------------------------------------*/
UINT32 App_Image_Init_De_Params(UINT32 chNo, UINT32 ParamHandlerNo, const char *name, UINT32 type)
{
    UINT32 RVal = -1;
    IP_PARAM_s *IpParamTmp = NULL;

    AmbaIQParam_Get_By_Name(ParamHandlerNo, name, type, &IpParamTmp);
    if (IpParamTmp) {
        if (strcmp(name, "de_default_still") == 0) {
        RVal = AmbaImg_Proc_Cmd(MW_IP_SET_DE_PARAM, IP_MODE_STILL, (UINT32)IpParamTmp->Data, 0);
        } else {
        RVal = AmbaImg_Proc_Cmd(MW_IP_SET_DE_PARAM, IP_MODE_VIDEO, (UINT32)IpParamTmp->Data, 0);
        }
    }
    return RVal;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: App_Image_Init
 *
 *  @Description:: Entry point of inital of IQ parameters
 *
 *  @Input      ::  None
 *
 *  @Output     ::  None
 *
 *  @Return     ::
 *    UINT32 RVal      : retrun 0 means command done
\*-----------------------------------------------------------------------------------------------*/
extern void Amba_AeAwbAdj_Init(UINT32 chNo,UINT8 initFlg, AMBA_KAL_BYTE_POOL_t *pMMPL);
extern void Amba_AeAwbAdj_Control(UINT32 chNo);
extern void Amba_Ae_Ctrl(UINT32 chNo);
extern void Amba_Awb_Ctrl(UINT32 chNo);
extern void Amba_Adj_Ctrl(UINT32 chNo);
extern int AmbaIQParam_Allocate_Param_Mem(UINT32 chNo);
extern AMBA_KAL_BYTE_POOL_t G_MMPL;
UINT32 App_Image_Init(UINT32 ChCount, int sensorID)
{
    UINT32 RVal = 0;

    IMG_PROC_FUNC_s IpFuncTmp = {NULL, NULL, NULL, NULL};
    UINT32 i; //chNo = 0,
    //static UINT8 InitFlg = 0;
    //>>>>>>>>>>>>>>>>>>>>>>>>
    // ChCount = 1; //FIXME, for testing multi-channel
    //ChCount = 4;//
    //>>>>>>>>>>>>>>>>>>>>>>>>
    AmbaImg_Proc_Cmd(MW_IP_SET_TOTAL_CH_COUNT,(UINT32)ChCount , 0, 0);
    AmbaImg_Proc_Cmd(MW_IP_SET_MEM_CTRLADDR, (UINT32)&G_MMPL, 0, 0);
    AmbaImg_Proc_Cmd(MW_IP_TASK_INIT, 0, ChCount, 0);
    AmbaImg_Proc_Cmd(MW_IP_PARAM_INIT, ChCount, 0, 0);
    AmbaImg_Proc_Cmd(MW_IP_GET_TOTAL_CH_COUNT,(UINT32)&ChCount , 0, 0);
    AmbaImg_Proc_Cmd(MW_IP_VDSP_HDRL_MEM_INIT, ChCount,0, 0);

    IpFuncTmp.AeAwbAdj_Init    = Amba_AeAwbAdj_Init;
    IpFuncTmp.AeAwbAdj_Control = Amba_AeAwbAdj_Control;
    IpFuncTmp.Ae_Ctrl  =  Amba_Ae_Ctrl;
    IpFuncTmp.Awb_Ctrl =  Amba_Awb_Ctrl;
    IpFuncTmp.Adj_Ctrl =  Amba_Adj_Ctrl;

    for(i = 0;i < ChCount; i++) {
        AmbaImg_Proc_Cmd(MW_IP_RESET_VIDEO_PIPE_CTRL_PARAMS, i, 0, 0);
        AmbaImg_Proc_Cmd(MW_IP_RESET_STILL_PIPE_CTRL_PARAMS, i, 0, 0);
        AmbaImg_Proc_Cmd(MW_IP_REGISTER_FUNC, i, (UINT32)&IpFuncTmp, 0);
        AmbaIQParam_Allocate_Param_Mem(i);
    }

    return RVal;
}

UINT32 App_Image_Init_Iq_Params(UINT32 chNo, int sensorID)
{
    int RVal = 0;
    int ParamHandlerNo = 0;
    AMBA_3A_OP_INFO_s  AaaOpInfo = {ENABLE, ENABLE, ENABLE, ENABLE};

    UINT8 HdrEnable = 0;
    AmbaImg_Proc_Cmd(MW_IP_GET_VIDEO_HDR_ENABLE, (chNo / 2) * 2, (UINT32)&HdrEnable, 0);

    ParamHandlerNo = chNo;
    if(HdrEnable == IMG_DSP_HDR_MODE_0){
        sensorID += 100;
        ParamHandlerNo = (chNo / 2) * 2;
    }else if(HdrEnable == IMG_SENSOR_HDR_MODE_0){
        sensorID += 200;
        ParamHandlerNo = (chNo / 2) * 2;
    }
    //>>>>>
    AmbaPrint("\n\n@@@@ chNo : %3d, sensorID : %3d, ParamHandlerNo : %3d @@@ \n\n", chNo, sensorID, ParamHandlerNo);
    if (ImageInitFlg[chNo] == 0) {
        AmbaIQParam_Register(ParamHandlerNo, sensorID);
        AmbaIQParam_Init_Param_Proxy(ParamHandlerNo);

        RVal += App_Image_Init_Image_Params(chNo,ParamHandlerNo, "img_default", IMG_PARAM);

        if((HdrEnable == IMG_DSP_HDR_MODE_0)&&((chNo % 2)==1)){
            RVal += App_Image_Init_AAA_Params(chNo,ParamHandlerNo, "aaa_default_01", AAA_PARAM);
        }else{
            RVal += App_Image_Init_AAA_Params(chNo,ParamHandlerNo, "aaa_default_00", AAA_PARAM);
        }

        RVal += App_Image_Init_AdjTable_Param(chNo,ParamHandlerNo, "adj_table_param_default", ADJ_TABLE_PARAM);
        RVal += App_Image_Init_Adj_Params(chNo,ParamHandlerNo, "adj_video_default_00", ADJ_PARAM_VIDEO);
#ifdef CONFIG_SOC_A12
        //RVal += App_Image_Init_Adj_Params(chNo,ParamHandlerNo, "adj_hiso_video_default_00", ADJ_PARAM_HISO_VIDEO);
#endif
        RVal += App_Image_Init_Adj_Params(chNo,ParamHandlerNo, "adj_photo_default_00", ADJ_PARAM_PHOTO);
        RVal += App_Image_Init_Adj_Params(chNo,ParamHandlerNo, "adj_still_default_00", ADJ_PARAM_LISO_STILL);
        RVal += App_Image_Init_Adj_Params(chNo,ParamHandlerNo, "adj_hiso_still_default_00", ADJ_PARAM_HISO_STILL);
        RVal += App_Image_Init_Adj_Params(chNo,ParamHandlerNo, "adj_still_idx", ADJ_PARAM_STILL_IDX);
#ifdef CONFIG_SOC_A12
        //RVal += App_Image_Init_Adj_Params(chNo,ParamHandlerNo, "adj_video_idx", ADJ_PARAM_VIDEO_IDX);
#endif
        RVal += App_Image_Init_Scene_Params(chNo,ParamHandlerNo, "scene_data_s01", SCENE_DATA);
        RVal += App_Image_Init_Scene_Params(chNo,ParamHandlerNo, "scene_data_s02", SCENE_DATA);
        RVal += App_Image_Init_Scene_Params(chNo,ParamHandlerNo, "scene_data_s03", SCENE_DATA);
        RVal += App_Image_Init_Scene_Params(chNo,ParamHandlerNo, "scene_data_s04", SCENE_DATA);
        RVal += App_Image_Init_Scene_Params(chNo,ParamHandlerNo, "scene_data_s05", SCENE_DATA);
        RVal += App_Image_Init_De_Params(chNo,ParamHandlerNo, "de_default_video", DE_PARAM_VIDEO);
        RVal += App_Image_Init_De_Params(chNo,ParamHandlerNo, "de_default_still", DE_PARAM_STILL);
        if (RVal == 0) {
            AmbaImg_Proc_Cmd(MW_IP_SET_AAA_OP_INFO, chNo, (UINT32)&AaaOpInfo, 0);
            //ImageInitFlg[chNo] = 1;
        }else{
            RVal = 1;
            AmbaPrint("Check <%s>", __FUNCTION__);
        }
    }
    return RVal;
}


