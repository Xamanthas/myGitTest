/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaIQParamMn34222_A12.c
 *
 *  @Copyright      :: Copyright (C) 2013 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Implementation of sonsor MN34222 related settings.
 *
 *  @History        ::
 *      Date        Name        Comments
 *      07/16/2013  Eathan Wu Created
 *
\*-------------------------------------------------------------------------------------------------------------------*/
#ifdef CONFIG_SOC_A12

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "AmbaDataType.h"
#include "AmbaPrint.h"
#include "AmbaFS.h"
#include "AmbaUtility.h"

#include "AmbaImg_Adjustment_A12.h"
#include "AmbaIQParamHandler.h"
#include "AmbaIQParamMn34222_A12_Table.h"


/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParamMn34222_A12_Init
 *
 *  @Description:: Init function for MN34222 sensor
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
static int AmbaIQParamMn34222_A12_Init(void)
{
    return 0;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParamMn34222_A12_Get_Color_Table_Path
 *
 *  @Description:: Get color table path for MN34222 sensor
 *
 *  @Input      ::
 *    None
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    COLOR_TABLE_PATH_s * : Return color table path address.
\*-----------------------------------------------------------------------------------------------*/
static COLOR_TABLE_PATH_s* AmbaIQParamMn34222_A12_Get_Color_Table_Path(void)
{
    return GCcTableMN34222;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParamMn34222_A12_Get_Adj_Video_Table_Path
 *
 *  @Description:: Get adj video table path for MN34222 sensor
 *
 *  @Input      ::
 *    None
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    IP_TABLE_PATH_s * : Return IQ table path address.
\*-----------------------------------------------------------------------------------------------*/
static IP_TABLE_PATH_s* AmbaIQParamMn34222_A12_Get_Adj_Video_Table_Path(int paramLoaded, int tableNo)
{
    return &GVideoAdjTablePathMN34222[tableNo];
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParamMN34222_A12_Get_Adj_Video_High_Iso_Table_Path
 *
 *  @Description:: Get adj video high iso table path for MN34222 sensor
 *
 *  @Input      ::
 *    None
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    IP_TABLE_PATH_s * : Return IQ table path address.
\*-----------------------------------------------------------------------------------------------*/
static IP_TABLE_PATH_s* AmbaIQParamMn34222_A12_Get_Adj_Video_High_Iso_Table_Path(int paramLoaded, int tableNo)
{
    return &GVideoHIsoAdjTablePathMN34222[tableNo];
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParamMn34222_A12_Get_Adj_Photo_Table_Path
 *
 *  @Description:: Get adj photo table path for MN34222 sensor
 *
 *  @Input      ::
 *    None
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    IP_TABLE_PATH_s * : Return IQ table path address.
\*-----------------------------------------------------------------------------------------------*/
static IP_TABLE_PATH_s* AmbaIQParamMn34222_A12_Get_Adj_Photo_Table_Path(int paramLoaded, int tableNo)
{
    return &GPhotoAdjTablePathMN34222[tableNo];
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParamMn34222_A12_Get_Adj_Still_Low_Iso_Table_Path
 *
 *  @Description:: Get adj still low iso table path for MN34222 sensor
 *
 *  @Input      ::
 *    None
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    IP_TABLE_PATH_s * : Return IQ table path address.
\*-----------------------------------------------------------------------------------------------*/
static IP_TABLE_PATH_s* AmbaIQParamMn34222_A12_Get_Adj_Still_Low_Iso_Table_Path(int paramLoaded, int tableNo)
{
    return &GStillLIsoAdjTablePathMN34222[tableNo];
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParamMn34222_A12_Get_Adj_Still_High_Iso_Table_Path
 *
 *  @Description:: Get adj still high iso table path for MN34222 sensor
 *
 *  @Input      ::
 *    None
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    IP_TABLE_PATH_s * : Return IQ table path address.
\*-----------------------------------------------------------------------------------------------*/
static IP_TABLE_PATH_s* AmbaIQParamMn34222_A12_Get_Adj_Still_High_Iso_Table_Path(int paramLoaded, int tableNo)
{
    return &GStillHIsoAdjTablePathMN34222[tableNo];
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParamMn34222_A12_Get_Scene_Data_Table_Path
 *
 *  @Description:: Get scene mode data table path for MN34222 sensor
 *
 *  @Input      ::
 *    None
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    IP_TABLE_PATH_s * : Return IQ table path address.
\*-----------------------------------------------------------------------------------------------*/
static IP_TABLE_PATH_s* AmbaIQParamMn34222_A12_Get_Scene_Data_Table_Path(int paramLoaded, int sceneSet)
{
    return &GSceneDataTablePathMN34222[sceneSet];
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParamMn34222_A12_Get_Param_Table_Path
 *
 *  @Description:: Get param table path for MN34222 sensor
 *
 *  @Input      ::
 *    None
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    PARAM_TABLE_PATH_s * : Return param table path address.
\*-----------------------------------------------------------------------------------------------*/
static PARAM_TABLE_PATH_s* AmbaIQParamMn34222_A12_Get_Param_Table_Path(void)
{
    return NULL;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParamMn34222_A12_Get_Img_Table_Path
 *
 *  @Description:: Get Image default img path for Mn34222 sensor
 *
 *  @Input      ::
 *   
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    IP_TABLE_PATH_s* : Return img table path address.
\*-----------------------------------------------------------------------------------------------*/
static IP_TABLE_PATH_s* AmbaIQParamMn34222_A12_Get_Img_Table_Path(void)
{
    return &GImgAdjTablePathMN34222;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParamMn34222_A12_Get_Aaa_Table_Path
 *
 *  @Description:: Get Aaa default param table path for Mn34222 sensor
 *
 *  @Input      ::
 *   
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    IP_TABLE_PATH_s * : Return Aaa table path address.
\*-----------------------------------------------------------------------------------------------*/
static IP_TABLE_PATH_s* AmbaIQParamMn34222_A12_Get_Aaa_Table_Path( int tableNo)
{
    return &GAaaAdjTablePathMN34222[tableNo];
}
/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParamMn34222_A12_Get_AdjTable_Table_Path
 *
 *  @Description:: Get Adj Table path for Mn34222 sensor
 *
 *  @Input      ::
 *   
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    IP_TABLE_PATH_s * : Return AdjTable table path address.
\*-----------------------------------------------------------------------------------------------*/
static IP_TABLE_PATH_s* AmbaIQParamMn34222_A12_Get_AdjTable_Table_Path(void)
{
    return &GAdjTablePathMN34222;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParamMn34222_A12_Get_Adj_Still_Idx_Info_Table_Path
 *
 *  @Description:: Get Still Idx Adj param table path for Mn34222 sensor
 *
 *  @Input      ::
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    IP_TABLE_PATH_s * : Return adj still idx info table table path address.
\*-----------------------------------------------------------------------------------------------*/
static IP_TABLE_PATH_s* AmbaIQParamMn34222_A12_Get_Adj_Still_Idx_Info_Table_Path(void)
{
    return &GStillIdxInfoAdjTablePathMN34222;
}
/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParamMn34222_A12_Get_Adj_Video_Idx_Info_Table_Path
 *
 *  @Description:: Get Video Idx Adj param table path for Mn34222 sensor
 *
 *  @Input      ::
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    IP_TABLE_PATH_s * : Return video idx info table path address.
\*-----------------------------------------------------------------------------------------------*/
static IP_TABLE_PATH_s* AmbaIQParamMn34222_A12_Get_Adj_Video_Idx_Info_Table_Path(void)
{
    return &GVideoIdxInfoAdjTablePathMN34222;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParamMn34222_A12_Get_De_Table_Path
 *
 *  @Description:: Get De param table pathfor Mn34222 sensor
 *
 *  @Input      ::
 *    int mode      : 0: Video mode, 1: still mode
 *
 *  @Output     ::
 *    None
 *
 *
 *  @Return     ::
 *    IP_TABLE_PATH_s * : Return De table path address.
\*-----------------------------------------------------------------------------------------------*/
static IP_TABLE_PATH_s* AmbaIQParamMn34222_A12_Get_De_Table_Path(int mode)
{
    switch(mode){
        case DE_PARAM_VIDEO:
        return &GDeVideoTablePathMN34222;
            break;
    case DE_PARAM_STILL:
            return &GDeStillTablePathMN34222;
        break;
    default:
        return &GDeVideoTablePathMN34222;
        break;

    }

}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaIQParamMn34222_A12_Register
 *
 *  @Description:: Register parameter proxy for MN34222 sensor
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
int AmbaIQParamMn34222_A12_Register(UINT32 chNo)
{
    PARAM_PROXY_s Par = {0};
    char DevName[] = {'m','n','3','4','2','2','2','_','a','1','2','\0'};

    Par.Id = 0;
    strcpy(Par.Name, DevName);

    Par.Init = AmbaIQParamMn34222_A12_Init;

    Par.GetColorTablePath = AmbaIQParamMn34222_A12_Get_Color_Table_Path;
    Par.GetParamTablePath = AmbaIQParamMn34222_A12_Get_Param_Table_Path;
    Par.GetAdjVideoTablePath = AmbaIQParamMn34222_A12_Get_Adj_Video_Table_Path;
    Par.GetAdjVideoHIsoTablePath = AmbaIQParamMn34222_A12_Get_Adj_Video_High_Iso_Table_Path;
    Par.GetAdjPhotoTablePath = AmbaIQParamMn34222_A12_Get_Adj_Photo_Table_Path;
    Par.GetAdjStillLIsoTablePath = AmbaIQParamMn34222_A12_Get_Adj_Still_Low_Iso_Table_Path;
    Par.GetAdjStillHIsoTablePath = AmbaIQParamMn34222_A12_Get_Adj_Still_High_Iso_Table_Path;
    Par.GetSceneDataTablePath = AmbaIQParamMn34222_A12_Get_Scene_Data_Table_Path;
    Par.GetImgTablePath = AmbaIQParamMn34222_A12_Get_Img_Table_Path;
    Par.GetAaaTablePath = AmbaIQParamMn34222_A12_Get_Aaa_Table_Path;   
    Par.GetAdjStillIdxInfoTablePath = AmbaIQParamMn34222_A12_Get_Adj_Still_Idx_Info_Table_Path;
    Par.GetAdjVideoIdxInfoTablePath = AmbaIQParamMn34222_A12_Get_Adj_Video_Idx_Info_Table_Path;
    Par.GetDeTablePath = AmbaIQParamMn34222_A12_Get_De_Table_Path;
    Par.GetAdjTablePath = AmbaIQParamMn34222_A12_Get_AdjTable_Table_Path;

    AmbaIQParam_Attach(chNo, &Par);

    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif