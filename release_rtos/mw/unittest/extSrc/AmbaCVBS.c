/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaCVBS.c
 *
 *  @Copyright      :: Copyright (C) 2013 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Camera Operation Mode Control software task related APIs
 *
 *  @History        ::
 *      Date        Name        Comments
 *      05/06/2013  Y.H.Chen    Created
 *
\*-------------------------------------------------------------------------------------------------------------------*/

#if 0 //moved to SSP, no longer used

#include <stdio.h>
#include <string.h>

#include "AmbaDataType.h"
#include "AmbaCVBS.h"

#include "AmbaPLL.h"
#include "AmbaDSP.h"
#include "AmbaDSP_VOUT.h"
#include "AmbaDSP_Liveview.h"

static AMBA_DSP_VOUT_DISPLAY_CVBS_CONFIG_s DspVoutConfig[] = {
    [AMBA_DSP_VOUT_SYSTEM_60HZ] = {
        .BlankLevel     = 0x7A,
        .SyncLevel      = 0x00,
        .BlackLevel     = 0x7D,
        .ClampLevel     = 0x00,
        .Gain           = 0x3B8,
        .Offset         = 0x000,
        .ClampLower     = 0x000,
        .ClampUpper     = 0x3FF,
    },
    
    [AMBA_DSP_VOUT_SYSTEM_50HZ] = {
        .BlankLevel     = 0x7E,
        .SyncLevel      = 0x04,
        .BlackLevel     = 0x69,
        .ClampLevel     = 0x00,
        .Gain           = 0x3D8,
        .Offset         = 0x000,
        .ClampLower     = 0x000,
        .ClampUpper     = 0x3FF,
    },
};

static AMBA_CVBS_INFO_s CvbsConfig[] = {
    [AMBA_DSP_VOUT_SYSTEM_60HZ] = {
        .AspectRatio    = AMBA_DSP_VOUT_DAR_4_3,
        .Width          = 720,
        .Height         = 480,
        .FrameRate      = {
            .Interlace      = 1,
            .TimeScale      = 60000,
            .NumUnitsInTick = 1001
        }
    },
    
    [AMBA_DSP_VOUT_SYSTEM_50HZ] = {
        .AspectRatio    = AMBA_DSP_VOUT_DAR_4_3,
        .Width          = 720,
        .Height         = 576,
        .FrameRate      = {
            .Interlace      = 1,
            .TimeScale      = 50,
            .NumUnitsInTick = 1
        }
    },
};

AMBA_DSP_VOUT_TV_SYSTEM_e AmbaCvbsCtrl = AMBA_DSP_VOUT_SYSTEM_60HZ;

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaCVBS_Config
 *
 *  @Description:: Configure CVBS signal
 *
 *  @Input      ::
 *      TvSystem:   TV system (60Hz/50Hz field rate based system)
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *      int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
int AmbaCVBS_Config(AMBA_DSP_VOUT_TV_SYSTEM_e TvSystem)
{
    AmbaPLL_SetVoutTvClk(27000000);

    AmbaCvbsCtrl = TvSystem;
    AmbaDSP_VoutDisplayCvbsSetup(TvSystem);
    return AmbaDSP_VoutDisplayCvbsVoltageCtrl(&DspVoutConfig[TvSystem]);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaCVBS_Enable
 *
 *  @Description:: Enable CVBS signal encoder
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *      int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
int AmbaCVBS_Enable(void)
{
    AMBA_DSP_LIVEVIEW_DISP_WINDOW_s LiveviewDisplayCfg;
    AMBA_DSP_WINDOW_s VideoWin;
    AMBA_DSP_VOUT_MIXER_CONFIG_s MixerWin;
    AMBA_CVBS_INFO_s *pCvbsConfig = &CvbsConfig[AmbaCvbsCtrl];

    memcpy(&LiveviewDisplayCfg.FrameRate, &pCvbsConfig->FrameRate, sizeof(AMBA_DSP_FRAME_RATE_s));
    LiveviewDisplayCfg.Window.Width = pCvbsConfig->Width;
    LiveviewDisplayCfg.Window.Height = pCvbsConfig->Height;
    AmbaDSP_LiveviewDispConfig(AMBA_DSP_VOUT_TV, &LiveviewDisplayCfg);

    memset(&VideoWin, 0, sizeof(VideoWin));
    VideoWin.OffsetX = 0;
    VideoWin.OffsetY = 0;
    VideoWin.Width = pCvbsConfig->Width;
    VideoWin.Height = pCvbsConfig->Height >> 1;
    AmbaDSP_VoutVideoWindowSetup(AMBA_DSP_VOUT_TV, &VideoWin);

    memset(&MixerWin, 0, sizeof(MixerWin));
    memcpy(&MixerWin.FrameRate, &pCvbsConfig->FrameRate, sizeof(AMBA_DSP_FRAME_RATE_s));
    MixerWin.MixerColorFormat = MIXER_IN_YUV_444_RGB;
    MixerWin.ActiveWidth = pCvbsConfig->Width;
    MixerWin.ActiveHeight = pCvbsConfig->Height >> 1;
    MixerWin.VideoHorReverseEnable = 0;
    AmbaDSP_VoutMixerSetup(AMBA_DSP_VOUT_TV, &MixerWin);

    return OK;
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaCVBS_Disable
 *
 *  @Description:: Disable CVBS signal encoder
 *
 *  @Input      :: none
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *      int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
int AmbaCVBS_Disable(void)
{
    return AmbaDSP_VoutReset(AMBA_DSP_VOUT_TV);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaCVBS_SetColorBar
 *
 *  @Description:: Color bar patter control
 *
 *  @Input      ::
 *      EnableFlag: Enable(1)/Disable(0) color bar pattern
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *      int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
int AmbaCVBS_SetColorBar(UINT8 EnableFlag)
{
    return AmbaDSP_VoutDisplayCvbsColorBar(EnableFlag);
}

/*-----------------------------------------------------------------------------------------------*\
 *  @RoutineName:: AmbaCVBS_GetInfo
 *
 *  @Description:: Get current CVBS configuration
 *
 *  @Input      ::
 *      pInfo:  information for return
 *
 *  @Output     :: none
 *
 *  @Return     ::
 *      int : OK(0)/NG(-1)
\*-----------------------------------------------------------------------------------------------*/
int AmbaCVBS_GetInfo(AMBA_CVBS_INFO_s *pInfo)
{
    if (pInfo == NULL)
        return NG;

    memcpy(pInfo, &CvbsConfig[AmbaCvbsCtrl], sizeof(AMBA_CVBS_INFO_s));

    return OK;
}

#endif
