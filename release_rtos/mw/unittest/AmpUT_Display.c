/**
 *  @file AmpUT_Display.c
 *
 *  **History**
 *      |Date       |Name        |Comments       |
 *      |-----------|------------|---------------|
 *      |2013/3/15  |cyweng      |Created        |
 *
 *  @copyright 2013 Ambarella Corporation. All rights reserved.
 *                 No part of this file may be reproduced, stored in a retrieval system,
 *                 or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                 recording, or otherwise, without the prior consent of Ambarella Corporation.
 */

#include "AmpUT_Display.h"
#include "AmbaLCD.h"
#include "AmbaHDMI.h"
#include "util.h"

#include "UT_Pref.h"

#define UTDispMsg(Fn, Str, ...) {AmbaPrint("[AmpUT - Display] [%s]: %s", Fn, Str, ##__VA_ARGS__);}

//Display handler
static void* pDispUTMemPoolAddr = NULL;
static UINT8 DispUTRunCVBS = 0;                ///< running CVBS as TV
static AMP_DISP_HDLR_s *pDispUTHdlrLCD = NULL;
static AMP_DISP_HDLR_s *pDispUTHdlrTV = NULL;
static AMP_DISP_WINDOW_HDLR_s *pDispUTWindowHdlrLCD = NULL;
static AMP_DISP_WINDOW_HDLR_s *pDispUTWindowHdlrTV = NULL;
static UINT8 DispLCDStatus = 0;     //0:disable 1:off 2:on
static UINT8 DispTVStatus = 0;      //0:disable 1:off 2:on

int AmpUT_Display_Init(void)
{
    AMP_DISP_INIT_CFG_s DispInitCfg;
    AMP_DISP_CFG_s DispCfg;

    if (pDispUTMemPoolAddr == NULL) {
        /* Display Init */
        AmpDisplay_GetDefaultInitCfg(&DispInitCfg);
        if (AmpUtil_GetAlignedPool(&G_MMPL,
                                   (void**) &(DispInitCfg.MemoryPoolAddr),
                                   &pDispUTMemPoolAddr,
                                   DispInitCfg.MemoryPoolSize, 1 << 5) != AMP_OK) {
            AmbaPrint("%s:%u Cannot allocate memory.", __FUNCTION__, __LINE__);
            return -1;
        }
        if (AmpDisplay_Init(&DispInitCfg) != AMP_OK) {
            UTDispMsg(__FUNCTION__, "AmpDisplay_Init() Fail");
            return -1;
        }

        // Register LCD driver
        #ifdef CONFIG_LCD_WDF9648W
        {
            extern AMBA_LCD_OBJECT_s AmbaLCD_WdF9648wObj;
            AmbaLCD_Hook(AMP_DISP_CHANNEL_DCHAN, &AmbaLCD_WdF9648wObj);
        }
        #endif

        /* Create DCHAN display handler */
        AmpDisplay_GetDefaultCfg(&DispCfg);
        DispCfg.Device.Channel = AMP_DISP_CHANNEL_DCHAN;
        DispCfg.Device.DeviceId = AMP_DISP_LCD;
        DispCfg.Device.DeviceMode = 0xFFFF;
        DispCfg.ScreenRotate = 0;
        DispCfg.SystemType = AMP_DISP_NTSC;
        if (AmpDisplay_Create(&DispCfg, &pDispUTHdlrLCD) != AMP_OK) {
            UTDispMsg(__FUNCTION__, "AmpDisplay_Create() LCD Fail");
            return -1;
        }

        /* Create FCHAN display handler */
        AmpDisplay_GetDefaultCfg(&DispCfg);
        DispCfg.Device.Channel = AMP_DISP_CHANNEL_FCHAN;
        DispCfg.ScreenRotate = 0;
        DispCfg.SystemType = AMP_DISP_NTSC;
        if (DispUTRunCVBS) {
            DispCfg.Device.DeviceId = AMP_DISP_CVBS;
            DispCfg.Device.DeviceMode = AMP_DISP_ID_480I;
        } else {
            DispCfg.Device.DeviceId = AMP_DISP_HDMI;
            DispCfg.Device.DeviceMode = AMP_DISP_ID_1080P;
        }
        if (AmpDisplay_Create(&DispCfg, &pDispUTHdlrTV) != AMP_OK) {
            UTDispMsg(__FUNCTION__, "AmpDisplay_Create() TV Fail");
            return -1;
        }
        DispLCDStatus = 1;
        DispTVStatus = 1;
        UTDispMsg(__FUNCTION__, "Success");
    } else {
        UTDispMsg(__FUNCTION__, "Already done");
    }

    return 0;
}

int AmpUT_Display_ConfigSet(UINT8 Channel, AMP_DISP_DEV_CFG_s *pDevCfg)
{
    int Rval = AMP_ERROR_GENERAL_ERROR;
    AMP_DISP_HDLR_s *pDispHdlr;

    if (pDispUTMemPoolAddr == NULL) {
        UTDispMsg(__FUNCTION__, "call AmpUT_Display_Init()");
        Rval = AmpUT_Display_Init();
    }

    if (pDevCfg == NULL) {
        UTDispMsg(__FUNCTION__, "Error pDevCfg:0x%x", pDevCfg);
        return Rval;
    }

    if (Channel == 0) {
        pDispHdlr = pDispUTHdlrLCD;
    } else if (Channel == 1) {
        pDispHdlr = pDispUTHdlrTV;
    } else {
        UTDispMsg(__FUNCTION__, "Please enter device channel: dchan / fchan");
        return -1;
    }

    Rval = AmpDisplay_SetDeviceCfg(pDispHdlr, pDevCfg);
    UTDispMsg(__FUNCTION__, ((Rval==AMP_OK) ? "Success" : "Fail"));
    return 0;
}

int AmpUT_Display_DispHdlrGet(UINT8 Channel, AMP_DISP_HDLR_s **hdlr)
{
    int Rval = AMP_ERROR_GENERAL_ERROR;
    if (Channel == 0){
        *hdlr = pDispUTHdlrLCD;
    } else {
        *hdlr = pDispUTHdlrTV;
    }

    if (hdlr) {
        Rval = AMP_OK;
    }
    UTDispMsg(__FUNCTION__, ((Rval==AMP_OK) ? "Success" : "Fail"));
    return Rval;
}

int AmpUT_Display_ConfigGet(UINT8 Channel, AMP_DISP_DEV_CFG_s *pOutDevCfg)
{
    int Rval = AMP_ERROR_GENERAL_ERROR;
    AMP_DISP_HDLR_s *pDispHdlr;

    if (pDispUTMemPoolAddr == NULL) {
        UTDispMsg(__FUNCTION__, "call AmpUT_Display_Init()");
        Rval = AmpUT_Display_Init();
    }

    if (Channel == 0) {
        pDispHdlr = pDispUTHdlrLCD;
    } else if (Channel == 1) {
        pDispHdlr = pDispUTHdlrTV;
    } else {
        UTDispMsg(__FUNCTION__, "Please enter device channel: dchan / fchan");
        return -1;
    }
    Rval = AmpDisplay_GetDeviceCfg(pDispHdlr, pOutDevCfg);
    UTDispMsg(__FUNCTION__, ((Rval==AMP_OK) ? "Success" : "Fail"));
    return 0;
}

static int __AmpUT_Display_Config(int argc, char **argv)
{
    int Rval = AMP_ERROR_GENERAL_ERROR;

    AmbaPrint("AmpUT_Display_Config");

    if ((strcmp(argv[2],"dchan") == 0) || (strcmp(argv[2],"0") == 0)) {
        if (strcmp(argv[3],"0") == 0) {
            DispLCDStatus = 0;
        } else if (strcmp(argv[3],"1") == 0) {
            DispLCDStatus = 1;
        }
        if (       strcmp(argv[4],"0") == 0) {
#ifdef CONFIG_LCD_WDF9648W
            extern AMBA_LCD_OBJECT_s AmbaLCD_WdF9648wObj;
            AmbaLCD_Hook(AMP_DISP_CHANNEL_DCHAN, &AmbaLCD_WdF9648wObj);
            AmbaPrint("AmpUT_Display_Config Hook LCD WdF9648w");
#endif
        } else if (strcmp(argv[4],"1") == 0) {
#ifdef CONFIG_LCD_T20P52
            extern AMBA_LCD_OBJECT_s AmbaLCD_T20P52Obj;
            AmbaLCD_Hook(AMP_DISP_CHANNEL_DCHAN, &AmbaLCD_T20P52Obj);
            AmbaPrint("AmpUT_Display_Config Hook LCD T20P52");
#endif
        } else if (strcmp(argv[4],"2") == 0) {
#ifdef CONFIG_LCD_T27P05
            extern AMBA_LCD_OBJECT_s AmbaLCD_T27P05Obj;
            AmbaLCD_Hook(AMP_DISP_CHANNEL_DCHAN, &AmbaLCD_T27P05Obj);
            AmbaPrint("AmpUT_Display_Config Hook LCD T27P05");
#endif
        } else if (strcmp(argv[4],"3") == 0) {
#ifdef CONFIG_LCD_T30P61
            extern AMBA_LCD_OBJECT_s AmbaLCD_T30P61Obj;
            AmbaLCD_Hook(AMP_DISP_CHANNEL_DCHAN, &AmbaLCD_T30P61Obj);
            AmbaPrint("AmpUT_Display_Config Hook LCD T30P61");
#endif
        }
        AmbaPrint("DispLCDStatus: %d", DispLCDStatus);
        Rval = 0;
    } else if ((strcmp(argv[2],"fchan") == 0) || (strcmp(argv[2],"1") == 0)) {
        AMP_DISP_DEV_CFG_s DispCfg = {0};
        AmpUT_Display_ConfigGet(1, &DispCfg);
        DispCfg.DeviceId = AMP_DISP_HDMI;

        if (strcmp(argv[3],"0") == 0) {
            DispTVStatus = 0;
        } else if (strcmp(argv[3],"1") == 0) {
            DispTVStatus = 1;
        }
        AmbaPrint("DispTVStatus: %d", DispTVStatus);
//        AmbaPrint("-            0: HDMI  640x 480p/60Hz  4:3");
//        AmbaPrint("-            1: HDMI  720x 480p/60Hz  4:3");
//        AmbaPrint("-            2: HDMI  720x 480p/60Hz 16:9");
//        AmbaPrint("-            3: HDMI 1280x 720p/60Hz 16:9");
//        AmbaPrint("-            4: HDMI 1920x1080i/60Hz 16:9");
//        AmbaPrint("-            5: HDMI 1440x 480i/60Hz  4:3");
//        AmbaPrint("-            6: HDMI 1440x 480i/60Hz 16:9");
//        AmbaPrint("-            7: HDMI 1920x1080p/60Hz 16:9");
//        AmbaPrint("-            8: HDMI  720x 576p/50Hz  4:3");
//        AmbaPrint("-            9: HDMI  720x 576p/50Hz 16:9");
//        AmbaPrint("-           10: HDMI 1280x 720p/50Hz 16:9");
//        AmbaPrint("-           11: HDMI 1920x1080i/50Hz 16:9");
//        AmbaPrint("-           12: HDMI 1440x 576i/50Hz  4:3");
//        AmbaPrint("-           13: HDMI 1440x 576i/50Hz 16:9");
//        AmbaPrint("-           14: HDMI 1920x1080p/50Hz 16:9");
//        AmbaPrint("-           15: CVBS  720x 480i       4:3");
        if (       strcmp(argv[4],"0") == 0) {
            DispCfg.DeviceMode = AMP_DISP_ID_DMT0659;   //
        } else if (strcmp(argv[4],"1") == 0) {
            DispCfg.DeviceMode = AMP_DISP_ID_480P;
        } else if (strcmp(argv[4],"2") == 0) {
            DispCfg.DeviceMode = AMP_DISP_ID_480P_WIDE;
        } else if (strcmp(argv[4],"3") == 0) {
            DispCfg.DeviceMode = AMP_DISP_ID_720P;
        } else if (strcmp(argv[4],"4") == 0) {
            DispCfg.DeviceMode = AMP_DISP_ID_1080I;
        } else if (strcmp(argv[4],"5") == 0) {
            DispCfg.DeviceMode = AMP_DISP_ID_480I;
        } else if (strcmp(argv[4],"6") == 0) {
            DispCfg.DeviceMode = AMP_DISP_ID_480I_WIDE;
        } else if (strcmp(argv[4],"7") == 0) {
            DispCfg.DeviceMode = AMP_DISP_ID_1080P;
        } else if (strcmp(argv[4],"8") == 0) {
            DispCfg.DeviceMode = AMP_DISP_ID_576P;
        } else if (strcmp(argv[4],"9") == 0) {
            DispCfg.DeviceMode = AMP_DISP_ID_576P_WIDE;
        } else if (strcmp(argv[4],"10") == 0) {
            DispCfg.DeviceMode = AMP_DISP_ID_720P50;
        } else if (strcmp(argv[4],"11") == 0) {
            DispCfg.DeviceMode = AMP_DISP_ID_1080I25;
        } else if (strcmp(argv[4],"12") == 0) {
            DispCfg.DeviceMode = AMP_DISP_ID_576I;
        } else if (strcmp(argv[4],"13") == 0) {
            DispCfg.DeviceMode = AMP_DISP_ID_576I_WIDE;
        } else if (strcmp(argv[4],"14") == 0) {
            DispCfg.DeviceMode = AMP_DISP_ID_1080P50;
        } else if (strcmp(argv[4],"15") == 0) {
            DispCfg.DeviceId = AMP_DISP_CVBS;
            DispCfg.DeviceMode = AMP_DISP_ID_480I;
        } else {
            AmbaPrint("Mode: %s error", argv[4]);
            Rval = -1;
        }
        Rval = AmpDisplay_SetDeviceCfg(pDispUTHdlrTV, &DispCfg);
    } else {
        AmbaPrint(" Please enter device channel: dchan / fchan ");
    }
    AmbaPrint("AmpUT_Display_Config %s", (Rval==AMP_OK) ? "Success" : "Fail");
    return Rval;
}

static int __AmpUT_Display_SetPxlfmt(int argc, char **argv)
{
    int Rval = AMP_ERROR_GENERAL_ERROR;

    AmbaPrint("AmpUT_Display_SetPxlfmt");

    if ((strcmp(argv[2],"dchan") == 0) || (strcmp(argv[2],"0") == 0)) {
        AmbaPrint("Do not support lcd pixel format setup now");
//        Rval = 0;
    } else if ((strcmp(argv[2],"fchan") == 0) || (strcmp(argv[2],"1") == 0)) {
        AMP_DISP_DEV_CFG_s DispCfg = {0};
        AmpUT_Display_ConfigGet(1, &DispCfg);
        DispCfg.DeviceId = AMP_DISP_HDMI;
        DispCfg.CustomCfg.EnCustomCfg = 1;
        if (strcmp(argv[3],"0") == 0) {
            AmbaPrint("HDMI pixel format RGB444_8B");
            DispCfg.CustomCfg.Cfg.HDMI.FrameLayout = AMBA_HDMI_VIDEO_2D;
            DispCfg.CustomCfg.Cfg.HDMI.PixelFormat = AMBA_DSP_VOUT_HDMI_RGB444_8B;
            DispCfg.CustomCfg.Cfg.HDMI.QuantRange = AMBA_HDMI_QRANGE_DEFAULT;
            DispCfg.CustomCfg.Cfg.HDMI.SampleRate = HDMI_AUDIO_FS_48K;
            DispCfg.CustomCfg.Cfg.HDMI.SpeakerAlloc = HDMI_CA_2CH_FL_FR;            
            DispCfg.CustomCfg.Cfg.HDMI.OverSample = HDMI_AUDIO_CLK_FREQ_128FS;
        } else if (strcmp(argv[3],"1") == 0) {
            AmbaPrint("HDMI pixel format YUV444_8B");
            DispCfg.CustomCfg.Cfg.HDMI.FrameLayout = AMBA_HDMI_VIDEO_2D;
            DispCfg.CustomCfg.Cfg.HDMI.PixelFormat = AMBA_DSP_VOUT_HDMI_YCC444_8B;
            DispCfg.CustomCfg.Cfg.HDMI.QuantRange = AMBA_HDMI_QRANGE_DEFAULT;
            DispCfg.CustomCfg.Cfg.HDMI.SampleRate = HDMI_AUDIO_FS_48K;
            DispCfg.CustomCfg.Cfg.HDMI.SpeakerAlloc = HDMI_CA_2CH_FL_FR;            
            DispCfg.CustomCfg.Cfg.HDMI.OverSample = HDMI_AUDIO_CLK_FREQ_128FS;
        } else if (strcmp(argv[3],"2") == 0) {
            AmbaPrint("HDMI pixel format YUV422_12B");
            DispCfg.CustomCfg.Cfg.HDMI.FrameLayout = AMBA_HDMI_VIDEO_2D;
            DispCfg.CustomCfg.Cfg.HDMI.PixelFormat = AMBA_DSP_VOUT_HDMI_YCC422_12B;
            DispCfg.CustomCfg.Cfg.HDMI.QuantRange = AMBA_HDMI_QRANGE_DEFAULT;
            DispCfg.CustomCfg.Cfg.HDMI.SampleRate = HDMI_AUDIO_FS_48K;
            DispCfg.CustomCfg.Cfg.HDMI.SpeakerAlloc = HDMI_CA_2CH_FL_FR;            
            DispCfg.CustomCfg.Cfg.HDMI.OverSample = HDMI_AUDIO_CLK_FREQ_128FS;
        }

        Rval = AmpDisplay_SetDeviceCfg(pDispUTHdlrTV, &DispCfg);
    } else {
        AmbaPrint(" Please enter device channel: dchan / fchan ");
    }
    AmbaPrint("AmpUT_Display_SetPxlfmt %s", (Rval==AMP_OK) ? "Success" : "Fail");
    return Rval;
}

int AmpUT_Display_Start(UINT8 Channel)
{
    int Rval = AMP_ERROR_GENERAL_ERROR;
    if (pDispUTMemPoolAddr == NULL) {
        UTDispMsg(__FUNCTION__, "call AmpUT_Display_Init()");
        Rval = AmpUT_Display_Init();
    }
    if (Channel == 0) {
        if (DispLCDStatus == 1) {
            Rval = AmpDisplay_Start(pDispUTHdlrLCD);
            DispLCDStatus = 2;
        } else if (DispLCDStatus == 2) {
            UTDispMsg(__FUNCTION__, "LCD started");
            Rval = AMP_OK;
        } else {
            UTDispMsg(__FUNCTION__, "LCD disabled");
            Rval = AMP_OK;
        }
    } else if (Channel == 1) {
        if (DispTVStatus == 1) {
            Rval = AmpDisplay_Start(pDispUTHdlrTV);
            DispTVStatus = 2;
        } else if (DispTVStatus == 2) {
            UTDispMsg(__FUNCTION__, "TV started");
            Rval = AMP_OK;
        } else {
            UTDispMsg(__FUNCTION__, "TV disabled");
            Rval = AMP_OK;
        }
    } else {
        UTDispMsg(__FUNCTION__, "Please enter device channel: dchan / fchan");
        return Rval;
    }
    UTDispMsg(__FUNCTION__, ((Rval==AMP_OK) ? "Success" : "Fail"));
    return 0;
}

static int __AmpUT_Display_Start(int argc, char **argv)
{
    int Rval = AMP_ERROR_GENERAL_ERROR;
    AmbaPrint("AmpUT_Display_Start");

    if ((strcmp(argv[2],"dchan") == 0) || (strcmp(argv[2],"0") == 0)) {
        Rval = AmpUT_Display_Start(0);
    } else if ((strcmp(argv[2],"fchan") == 0) || (strcmp(argv[2],"1") == 0)) {
        Rval = AmpUT_Display_Start(1);
    } else {
        AmbaPrint(" Please enter device channel: dchan / fchan ");
    }
    //AmbaDSP_VoutVideoSourceSel(AMBA_DSP_VOUT_TV, 1);   // To BG
    //AmpResource_DSPBoot(AMBA_DSP_SYS_STATE_PLAYBACK);
    AmbaPrint("AmpUT_Display_Start %s", (Rval==AMP_OK) ? "Success" : "Fail");
    return Rval;
}

int AmpUT_Display_pause(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("AmpUT_Display_pause");
    return 0;
}

int AmpUT_Display_resume(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("AmpUT_Display_resume");
    return 0;
}

int AmpUT_Display_Stop(UINT8 Channel)
{
    int Rval = AMP_ERROR_GENERAL_ERROR;
    if (pDispUTMemPoolAddr == NULL) {
        UTDispMsg(__FUNCTION__, "call AmpUT_Display_Init()");
        Rval = AmpUT_Display_Init();
    }
    if (Channel == 0) {
        if (DispLCDStatus == 2) {
            Rval = AmpDisplay_Stop(pDispUTHdlrLCD);
            DispLCDStatus = 1;
        } else if (DispLCDStatus == 1) {
            UTDispMsg(__FUNCTION__, "LCD stoped");
            Rval = AMP_OK;
        } else {
            UTDispMsg(__FUNCTION__, "LCD disabled");
            Rval = AMP_OK;
        }
    } else if (Channel == 1) {
        if (DispTVStatus == 2) {
            Rval = AmpDisplay_Stop(pDispUTHdlrTV);
            DispTVStatus = 1;
        } else if (DispTVStatus == 1) {
            UTDispMsg(__FUNCTION__, "TV stoped");
            Rval = AMP_OK;
        } else {
            UTDispMsg(__FUNCTION__, "TV disabled");
            Rval = AMP_OK;
        }
    } else {
        UTDispMsg(__FUNCTION__, "Please enter device channel: dchan / fchan");
        return Rval;
    }
    UTDispMsg(__FUNCTION__, ((Rval==AMP_OK) ? "Success" : "Fail"));
    return 0;
}

static int __AmpUT_Display_Stop(int argc, char **argv)
{
    int Rval = AMP_ERROR_GENERAL_ERROR;

    AmbaPrint("AmpUT_Display_Stop Begin");
    if ((strcmp(argv[2],"dchan") == 0) || (strcmp(argv[2],"0") == 0)) {
        Rval = AmpUT_Display_Stop(0);
    } else if ((strcmp(argv[2],"fchan") == 0) || (strcmp(argv[2],"1") == 0)) {
        Rval = AmpUT_Display_Stop(1);
    } else {
        AmbaPrint(" Please enter device channel: dchan / fchan ");
    }
    //AmbaDSP_VoutVideoSourceSel(AMBA_DSP_VOUT_TV, 1);   // To BG
    //AmpResource_DSPBoot(AMBA_DSP_SYS_STATE_PLAYBACK);
    AmbaPrint("AmpUT_Display_Stop %s", (Rval==AMP_OK) ? "Success" : "Fail");
    return Rval;
}


int AmpUT_Display_Window_Create(UINT8 Channel,
                                AMP_DISP_WINDOW_CFG_s *pWindowCfg)
{
    int Rval = AMP_ERROR_GENERAL_ERROR;

    if (pWindowCfg == NULL) {
        UTDispMsg(__FUNCTION__, "pWindowCfg invalid");
        return Rval;
    }

    if (Channel == 0) {
        if (pDispUTWindowHdlrLCD != NULL) {
            Rval = AmpDisplay_SetWindowCfg(pDispUTWindowHdlrLCD, pWindowCfg);
//            UTDispMsg(__FUNCTION__, "LCD window created, do update");
        } else {
            Rval = AmpDisplay_CreateWindow(pDispUTHdlrLCD, pWindowCfg, &pDispUTWindowHdlrLCD);
        }
    } else if (Channel == 1) {
        if (pDispUTWindowHdlrTV != NULL) {
            Rval = AmpDisplay_SetWindowCfg(pDispUTWindowHdlrTV, pWindowCfg);
//            UTDispMsg(__FUNCTION__, "TV window created, do update");
        } else {
            if (DispUTRunCVBS) {
                pWindowCfg->TargetAreaOnPlane.Width = 720;
                pWindowCfg->TargetAreaOnPlane.Height = 480;
            }
            Rval = AmpDisplay_CreateWindow(pDispUTHdlrTV, pWindowCfg, &pDispUTWindowHdlrTV);
        }
    } else {
        UTDispMsg(__FUNCTION__, "Error Channel:%d", Channel);
    }
    UTDispMsg(__FUNCTION__, ((Rval==AMP_OK) ? "Success" : "Fail"));
    return Rval;
}

static int __AmpUT_Display_add_window(int argc, char **argv)
{
    char *StrTail = NULL;
    int Rval;
    AMP_DISP_WINDOW_CFG_s WindowCfg;
    UINT8 Chan = strtoul(argv[3], &StrTail, 0);

    AmbaPrint("AmpUT_Display_add_window Begin");
    memset(&WindowCfg, 0, sizeof(AMP_DISP_WINDOW_CFG_s));
    WindowCfg.Source = (AMP_DISP_WINDOW_SRC_e)strtoul(argv[4], &StrTail, 0);
    WindowCfg.CropArea.Width = 0;
    WindowCfg.CropArea.Height = 0;
    WindowCfg.CropArea.X = 0;
    WindowCfg.CropArea.Y = 0;
    WindowCfg.TargetAreaOnPlane.Width = strtoul(argv[5], &StrTail, 0);
    WindowCfg.TargetAreaOnPlane.Height = strtoul(argv[6], &StrTail, 0);
    WindowCfg.TargetAreaOnPlane.X = strtoul(argv[7], &StrTail, 0);
    WindowCfg.TargetAreaOnPlane.Y = strtoul(argv[8], &StrTail, 0);

    Rval = AmpUT_Display_Window_Create(Chan, &WindowCfg);
    AmbaPrint("AmpUT_Display_add_window %s", (Rval==AMP_OK) ? "Success" : "Fail");
    return Rval;
}

int AmpUT_Display_Window_Delete(UINT8 Channel)
{
    int Rval = AMP_ERROR_GENERAL_ERROR;

    if (Channel == 0) {
        Rval = AmpDisplay_DeleteWindow(pDispUTWindowHdlrLCD);
        pDispUTWindowHdlrLCD = NULL;
    } else if (Channel == 1) {
        Rval = AmpDisplay_DeleteWindow(pDispUTWindowHdlrTV);
        pDispUTWindowHdlrTV = NULL;
    } else {
        UTDispMsg(__FUNCTION__, "Error Channel:%d", Channel);
    }
    UTDispMsg(__FUNCTION__, ((Rval==AMP_OK) ? "Success" : "Fail"));

    return Rval;
}

static int __AmpUT_Display_del_window(int argc, char **argv)
{
    char *StrTail = NULL;
    int Rval = AMP_ERROR_GENERAL_ERROR;
    AMP_DISP_WINDOW_HDLR_s *pWindowHdlr;

    AmbaPrint("AmpUT_Display_Delete_Window Begin");
    if (strcmp(argv[3], "0") == 0) {
        Rval = AmpUT_Display_Window_Delete(0);
    } else if (strcmp(argv[3], "1") == 0) {
        Rval = AmpUT_Display_Window_Delete(1);
    } else {
        pWindowHdlr = (AMP_DISP_WINDOW_HDLR_s *)strtoul(argv[3], &StrTail, 0);
        AmbaPrint("AmpUT_Display_del_window, Addr = 0x%X", pWindowHdlr);
        if (pWindowHdlr == NULL) {
            AmbaPrint("The window address is NULL !!!");
            return Rval;
        }

        Rval = AmpDisplay_DeleteWindow(pWindowHdlr);
    }
    AmbaPrint("AmpUT_Display_del_window %s", (Rval==AMP_OK) ? "Success" : "Fail");
    return Rval;
}

int AmpUT_Display_Window_Update(UINT8 Channel,
                                AMP_DISP_WINDOW_SRC_e Source,
                                AMP_AREA_s *pCropArea,
                                AMP_AREA_s *pTargetAreaOnPlane)
{
    int Rval;
    AMP_DISP_WINDOW_CFG_s WindowCfg = {0};
    AMP_DISP_WINDOW_HDLR_s *pWindowHdlr;

    if (Channel == 0) {
        pWindowHdlr = pDispUTWindowHdlrLCD;
    } else if (Channel == 1) {
        pWindowHdlr = pDispUTWindowHdlrTV;
    } else {
        UTDispMsg(__FUNCTION__, "Error Channel:%d", Channel);
        return AMP_ERROR_GENERAL_ERROR;
    }

    if (pWindowHdlr == NULL) {
        UTDispMsg(__FUNCTION__, "Error window:%d not created", Channel);
        return AMP_ERROR_GENERAL_ERROR;
    }

    Rval = AmpDisplay_GetWindowCfg(pWindowHdlr, &WindowCfg);
    WindowCfg.Source = Source;
    WindowCfg.CropArea.Width = pCropArea->Width;
    WindowCfg.CropArea.Height = pCropArea->Height;
    WindowCfg.CropArea.X = pCropArea->X;
    WindowCfg.CropArea.Y = pCropArea->Y;
    WindowCfg.TargetAreaOnPlane.Width = pTargetAreaOnPlane->Width;
    WindowCfg.TargetAreaOnPlane.Height = pTargetAreaOnPlane->Height;
    WindowCfg.TargetAreaOnPlane.X = pTargetAreaOnPlane->X;
    WindowCfg.TargetAreaOnPlane.Y = pTargetAreaOnPlane->Y;
    Rval |= AmpDisplay_SetWindowCfg(pWindowHdlr, &WindowCfg);
    UTDispMsg(__FUNCTION__, ((Rval==AMP_OK) ? "Success" : "Fail"));
    return Rval;
}

static int __AmpUT_Display_update_window(int argc, char **argv)
{
    char *StrTail = NULL;
    int Rval;
    AMP_AREA_s CropArea = {0};
    AMP_AREA_s TargetAreaOnPlane = {0};
    AMP_DISP_WINDOW_SRC_e Source;

    AmbaPrint("AmpUT_Display_update_window Begin");

    Source = (AMP_DISP_WINDOW_SRC_e)strtoul(argv[3], &StrTail, 0);
    TargetAreaOnPlane.Width = strtoul(argv[4], &StrTail, 0);
    TargetAreaOnPlane.Height = strtoul(argv[5], &StrTail, 0);
    TargetAreaOnPlane.X = strtoul(argv[6], &StrTail, 0);
    TargetAreaOnPlane.Y = strtoul(argv[7], &StrTail, 0);
    CropArea.Width = strtoul(argv[8], &StrTail, 0);
    CropArea.Height = strtoul(argv[9], &StrTail, 0);
    CropArea.X = strtoul(argv[10], &StrTail, 0);
    CropArea.Y = strtoul(argv[11], &StrTail, 0);

    if (strcmp(argv[3], "0") == 0) {
        Rval = AmpUT_Display_Window_Update(0, Source, &CropArea, &TargetAreaOnPlane);
    } else if (strcmp(argv[3], "1") == 0) {
        Rval = AmpUT_Display_Window_Update(1, Source, &CropArea, &TargetAreaOnPlane);
    } else {
        AMP_DISP_WINDOW_CFG_s WindowCfg = {0};
        AMP_DISP_WINDOW_HDLR_s *pWindowHdlr;
        pWindowHdlr = (AMP_DISP_WINDOW_HDLR_s *)strtoul(argv[3], &StrTail, 0);
        AmbaPrint("AmpUT_Display_update_window, Addr = 0x%X", pWindowHdlr);
        if (pWindowHdlr == NULL) {
            AmbaPrint("AmpUT_Display_update_window Error address is NULL !!!");
            return AMP_ERROR_GENERAL_ERROR;
        }
        Rval = AmpDisplay_GetWindowCfg(pWindowHdlr, &WindowCfg);
        WindowCfg.CropArea.Width = CropArea.Width;
        WindowCfg.CropArea.Height = CropArea.Height;
        WindowCfg.CropArea.X = CropArea.X;
        WindowCfg.CropArea.Y = CropArea.Y;
        WindowCfg.TargetAreaOnPlane.Width = TargetAreaOnPlane.Width;
        WindowCfg.TargetAreaOnPlane.Height = TargetAreaOnPlane.Height;
        WindowCfg.TargetAreaOnPlane.X = TargetAreaOnPlane.X;
        WindowCfg.TargetAreaOnPlane.Y = TargetAreaOnPlane.Y;
        Rval |= AmpDisplay_SetWindowCfg(pWindowHdlr, &WindowCfg);
    }
    AmbaPrint("AmpUT_Display_update_window %s", (Rval==AMP_OK) ? "Success" : "Fail");
    return 0;
}

int AmpUT_Display_Act_Window(UINT8 Channel)
{
    int Rval = AMP_ERROR_GENERAL_ERROR;
    AMP_DISP_WINDOW_HDLR_s *pWindowHdlr;
    AMP_DISP_HDLR_s *pDispHdlr;
    UINT8 DispStatus;

    if (Channel == 0) {
        pWindowHdlr = pDispUTWindowHdlrLCD;
        pDispHdlr = pDispUTHdlrLCD;
        DispStatus = DispLCDStatus;
    } else if (Channel == 1) {
        pWindowHdlr = pDispUTWindowHdlrTV;
        pDispHdlr = pDispUTHdlrTV;
        DispStatus = DispTVStatus;
    } else {
        UTDispMsg(__FUNCTION__, "Error Channel:%d", Channel);
        return Rval;
    }
    if (pWindowHdlr == NULL) {
        UTDispMsg(__FUNCTION__, "Error window:%d not created", Channel);
        return Rval;
    }

    if (DispStatus == 0) {
        UTDispMsg(__FUNCTION__, "Disp:%d disabled not act", Channel);
        Rval = AMP_OK;
    } else if (DispStatus == 1) {
    #if 1
        UTDispMsg(__FUNCTION__, "Disp:%d off, Config it first", Channel);
        Rval = AMP_OK;
    #else
        UTDispMsg(__FUNCTION__, "Disp:%d off, starting...", Channel);
        Rval = AmpUT_Display_Start(Channel);
        Rval = AmpDisplay_SetWindowActivateFlag(pWindowHdlr, 1);
        Rval |= AmpDisplay_Update(pDispHdlr);
        Rval = AMP_OK;
    #endif
    } else {
        Rval = AmpDisplay_SetWindowActivateFlag(pWindowHdlr, 1);
        Rval |= AmpDisplay_Update(pDispHdlr);
    }
    UTDispMsg(__FUNCTION__, ((Rval==AMP_OK) ? "Success" : "Fail"));
    return Rval;
}

static int __AmpUT_Display_act_window(int argc, char **argv)
{
    char *StrTail = NULL;
    int Rval;

    AmbaPrint("AmpUT_Display_Act_Window Begin");

    if (strcmp(argv[3], "0") == 0) {
        Rval = AmpUT_Display_Act_Window(0);
    } else if (strcmp(argv[3], "1") == 0) {
        Rval = AmpUT_Display_Act_Window(1);
    } else {
        AMP_DISP_WINDOW_HDLR_s *pWindowHdlr;
        pWindowHdlr = (AMP_DISP_WINDOW_HDLR_s *)strtoul(argv[3], &StrTail, 0);
        AmbaPrint("AmpUT_Display_act_window, Addr = 0x%X", pWindowHdlr);
        if (pWindowHdlr == NULL) {
            AmbaPrint("AmpUT_Display_act_window Error address is NULL !!!");
            return AMP_ERROR_GENERAL_ERROR;
        }
        Rval = AmpDisplay_SetWindowActivateFlag(pWindowHdlr, 1);
        Rval |= AmpDisplay_Update(pDispUTHdlrLCD);
    }

    AmbaPrint("AmpUT_Display_update_window %s", (Rval==AMP_OK) ? "Success" : "Fail");
    return Rval;
}

int AmpUT_Display_DeAct_Window(UINT8 Channel)
{
    int Rval = AMP_ERROR_GENERAL_ERROR;
    AMP_DISP_WINDOW_HDLR_s *pWindowHdlr;
    AMP_DISP_HDLR_s *pDispHdlr;
    UINT8 DispStatus;

    if (Channel == 0) {
        pWindowHdlr = pDispUTWindowHdlrLCD;
        pDispHdlr = pDispUTHdlrLCD;
        DispStatus = DispLCDStatus;
    } else if (Channel == 1) {
        pWindowHdlr = pDispUTWindowHdlrTV;
        pDispHdlr = pDispUTHdlrTV;
        DispStatus = DispTVStatus;
    } else {
        AmbaPrint("AmpUT_Display_DeAct_Window Error Channel:%d", Channel);
        return Rval;
    }
    if (pWindowHdlr == NULL) {
        UTDispMsg(__FUNCTION__, "Error window:%d not created", Channel);
        return Rval;
    }
    if (DispStatus == 0) {
        UTDispMsg(__FUNCTION__, "Disp:%d disabled not act", Channel);
        Rval = AMP_OK;
    } else {
        Rval = AmpDisplay_SetWindowActivateFlag(pWindowHdlr, 0);
        Rval |= AmpDisplay_Update(pDispHdlr);
    }
    UTDispMsg(__FUNCTION__, ((Rval==AMP_OK) ? "Success" : "Fail"));
    return Rval;
}

int __AmpUT_Display_dact_window(int argc, char **argv)
{
    char *StrTail = NULL;
    int Rval = AMP_ERROR_GENERAL_ERROR;

    AmbaPrint("AmpUT_Display_dact_window Begin");

    if (strcmp(argv[3], "0") == 0) {
        Rval = AmpUT_Display_Act_Window(0);
    } else if (strcmp(argv[3], "1") == 0) {
        Rval = AmpUT_Display_Act_Window(1);
    } else {
        AMP_DISP_WINDOW_HDLR_s *pWindowHdlr;
        pWindowHdlr = (AMP_DISP_WINDOW_HDLR_s *)strtoul(argv[3], &StrTail, 0);
        AmbaPrint("AmpUT_Display_dact_window, Addr = 0x%X", pWindowHdlr);
        if (pWindowHdlr == NULL) {
            AmbaPrint("AmpUT_Display_dact_window Error address is NULL !!!");
            return Rval;
        }
        Rval |= AmpDisplay_SetWindowActivateFlag(pWindowHdlr, 0);
        Rval = AmpDisplay_Update(pDispUTHdlrLCD);
    }
    AmbaPrint("AmpUT_Display_dact_window %s", (Rval==AMP_OK) ? "Success" : "Fail");
    return Rval;
}

int AmpUT_Display_layout_content_info(AMP_DISP_WINDOW_CFG_s window_info);
int AmpUT_Display_twindow_info(int argc, char **argv)
{
    char *StrTail = NULL;
    int Rval;
    AMP_DISP_WINDOW_HDLR_s *pWindowHdlr;
    AMP_DISP_WINDOW_CFG_s WindowCfg = {0};

    if (strcmp(argv[3], "0") == 0) {
        pWindowHdlr = pDispUTWindowHdlrLCD;
    } else if (strcmp(argv[3], "1") == 0) {
        pWindowHdlr = pDispUTWindowHdlrTV;
    } else {
        pWindowHdlr = (AMP_DISP_WINDOW_HDLR_s *)strtoul(argv[3], &StrTail, 0);
        AmbaPrint("AmpUT_Display_update_window, Addr = 0x%X", pWindowHdlr);
        if (pWindowHdlr == NULL) {
            AmbaPrint("AmpUT_Display_twindow_info Error address is NULL !!!");
            return AMP_ERROR_GENERAL_ERROR;
        }
    }
    Rval = AmpDisplay_GetWindowCfg(pWindowHdlr, &WindowCfg);
    AmbaPrint("Window Info=================");
    Rval |= AmpUT_Display_layout_content_info(WindowCfg);
    return Rval;
}

int AmpUT_Display_GetInfo(UINT8 Channel, AMP_DISP_INFO_s *pOutDispInfo)
{
    int Rval = AMP_ERROR_GENERAL_ERROR;
    AMP_DISP_HDLR_s *pDispHdlr;

    if (pOutDispInfo == NULL) {
        UTDispMsg(__FUNCTION__, "Error pOutWindowCfg:0x%x", pOutDispInfo);
        return Rval;
    }

    if (Channel == 0) {
        pDispHdlr = pDispUTHdlrLCD;
    } else if (Channel == 1) {
        pDispHdlr = pDispUTHdlrTV;
    } else {
        UTDispMsg(__FUNCTION__, "Error Channel:%d", Channel);
        return Rval;
    }
    Rval = AmpDisplay_GetInfo(pDispHdlr, pOutDispInfo);
    UTDispMsg(__FUNCTION__, ((Rval==AMP_OK) ? "Success" : "Fail"));
    return Rval;
}

int AmpUT_Display_GetWindowCfg(UINT8 Channel, AMP_DISP_WINDOW_CFG_s *pOutWindowCfg)
{
    int Rval = AMP_ERROR_GENERAL_ERROR;
    AMP_DISP_WINDOW_HDLR_s *pWindowHdlr;

    if (pOutWindowCfg == NULL) {
        UTDispMsg(__FUNCTION__, "Error pOutWindowCfg:0x%x", pOutWindowCfg);
        return Rval;
    }

    if (Channel == 0) {
        pWindowHdlr = pDispUTWindowHdlrLCD;
    } else if (Channel == 1) {
        pWindowHdlr = pDispUTWindowHdlrTV;
    } else {
        UTDispMsg(__FUNCTION__, "Error Channel:%d", Channel);
        return Rval;
    }
    if (pWindowHdlr == NULL) {
//        Rval = AmpDisplay_GetDefaultWindowCfg(pOutWindowCfg);
        UTDispMsg(__FUNCTION__, "WARN window:%d not created", Channel);
        return Rval;
    }
    Rval = AmpDisplay_GetWindowCfg(pWindowHdlr, pOutWindowCfg);
    UTDispMsg(__FUNCTION__, ((Rval==AMP_OK) ? "Success" : "Fail"));
    return Rval;
}

int AmpUT_Display_SetWindowCfg(UINT8 Channel, AMP_DISP_WINDOW_CFG_s *pWindowCfg)
{
    int Rval = AMP_ERROR_GENERAL_ERROR;
    AMP_DISP_WINDOW_HDLR_s *pWindowHdlr;

    if (pWindowCfg == NULL) {
        UTDispMsg(__FUNCTION__, "Error pWindowCfg:0x%x", pWindowCfg);
        return Rval;
    }

    if (Channel == 0) {
        pWindowHdlr = pDispUTWindowHdlrLCD;
    } else if (Channel == 1) {
        pWindowHdlr = pDispUTWindowHdlrTV;
    } else {
        UTDispMsg(__FUNCTION__, "Error Channel:%d", Channel);
        return Rval;
    }
    if (pWindowHdlr == NULL) {
        UTDispMsg(__FUNCTION__, "Error window:%d not created", Channel);
        return Rval;
    }
    Rval = AmpDisplay_SetWindowCfg(pWindowHdlr, pWindowCfg);
    UTDispMsg(__FUNCTION__, ((Rval==AMP_OK) ? "Success" : "Fail"));
    return Rval;
}

int AmpUT_Display_Update(UINT8 Channel)
{
    int Rval = AMP_ERROR_GENERAL_ERROR;
    AMP_DISP_HDLR_s *pDispHdlr;
    UINT8 DispStatus;

    if (Channel == 0) {
        pDispHdlr = pDispUTHdlrLCD;
        DispStatus = DispLCDStatus;
    } else if (Channel == 1) {
        pDispHdlr = pDispUTHdlrTV;
        DispStatus = DispTVStatus;
    } else {
        UTDispMsg(__FUNCTION__, "Error Channel:%d", Channel);
        return Rval;
    }
    if (pDispHdlr == NULL) {
        UTDispMsg(__FUNCTION__, "Error DispHdlr:%d not created", Channel);
        return Rval;
    }
    if (DispStatus == 0) {
        UTDispMsg(__FUNCTION__, "Disp:%d disabled not act", Channel);
        Rval = AMP_OK;
    } else {
        Rval = AmpDisplay_Update(pDispHdlr);
    }

    UTDispMsg(__FUNCTION__, ((Rval==AMP_OK) ? "Success" : "Fail"));
    return Rval;
}

int AmpUT_Display_CvbsCtrl(UINT8 Enable)
{
    int Rval = OK;
    if (Enable == 0) {
        DispUTRunCVBS = 0;
    } else {
        DispUTRunCVBS = 1;
    }
    AmbaPrint("Cvbs:%d", DispUTRunCVBS);
    return Rval;
}

void AmpUT_Display_window_list(int argc, char **argv)
{
#if 0
    char *stop = NULL;
    AMP_DISP_INTER_HDLR_s *dispInterHdlr;
    AMP_DISP_WINDOW_HDLR_s *windowHdlr = NULL;
    UINT8 chan = strtoul(argv[3], &stop, 0);
    int i;

    if (chan == 0) {
        dispInterHdlr = pDispUTHdlrLCD->ctx;;
    }else if (chan == 1) {
        dispInterHdlr = pDispUTHdlrTV->ctx;;
    }

    AmbaPrint("AmpUT_Display_window_list, Channel = %d", chan);
    AmbaPrint("=========================================");
    AmbaPrint("maxNumWindow = %d", dispInterHdlr->dispCfg.maxNumWindow);
    for (i = 0; i<dispInterHdlr->dispCfg.maxNumWindow; i++) {
        windowHdlr = &((dispInterHdlr->dispCfg.layout.windowArray + i)->public);
        if ((dispInterHdlr->dispCfg.layout.windowArray + i)->used == 1) {
            AmbaPrint("Window (%d) Addr = 0x%X", i, windowHdlr);
        } else {
            AmbaPrint("Window (%d) Addr = Empty", i);
        }
    }
#endif
}

int AmpUT_Display_window_test(int argc, char **argv)
{
    AmbaPrint("AmpUT_Display_window_test");
    if (strcmp(argv[2],"add") == 0) {
        __AmpUT_Display_add_window(argc, argv);
    }else if (strcmp(argv[2],"del") == 0) {
        __AmpUT_Display_del_window(argc, argv);
    }else if (strcmp(argv[2],"update") == 0) {
        __AmpUT_Display_update_window(argc, argv);
    }else if (strcmp(argv[2],"act") == 0) {
        __AmpUT_Display_act_window(argc, argv);
    }else if (strcmp(argv[2],"deact") == 0) {
        __AmpUT_Display_dact_window(argc, argv);
    }else if (strcmp(argv[2],"info") == 0) {
        AmpUT_Display_twindow_info(argc, argv);
    }else if (strcmp(argv[2],"list") == 0) {
        AmpUT_Display_window_list(argc, argv);
    }else {
        AmbaPrint("Please Enter Test Cmd : ");
        AmbaPrint("- add [Channel(0:LCD/1:TV)] [Source] [Width] [Height] [Off_X] [Off_Y]");
        AmbaPrint("- del [Window_Addr]");
        AmbaPrint("- update [Window_Addr] [Source] [Width] [Height] [Off_X] [Off_Y]");
        AmbaPrint("- act [Window_Addr]");
        AmbaPrint("- deact [Window_Addr]");
        AmbaPrint("- info [Window_Addr]");
        AmbaPrint("- list [Channel(0:LCD/1:TV)]");
    }
    return 0;
}

int AmpUT_Display_dev_info(AMP_DISP_HDLR_s *dispHdlr)
{
    AMP_DISP_DEV_CFG_s devCfg;

    AmpDisplay_GetDeviceCfg(dispHdlr, &devCfg);

    AmbaPrint("AmpUT_Display_dev_info");
    AmbaPrint("=========================================");
    AmbaPrint("Channel  = %d", devCfg.Channel);
    AmbaPrint("Dev_ID   = %d", devCfg.DeviceId);
    AmbaPrint("Dev_Mode = %d", devCfg.DeviceMode);
    AmbaPrint("Dev_Ar   = %d", devCfg.DeviceAr);
    return 0;
}

int AmpUT_Display_layout_content_info(AMP_DISP_WINDOW_CFG_s window_info)
{
    AmbaPrint("===============");
    AmbaPrint("source      = %d", window_info.Source);
    AmbaPrint("===============");
    AmbaPrint("cropArea.X = %d", window_info.CropArea.X);
    AmbaPrint("cropArea.Y = %d", window_info.CropArea.Y);
    AmbaPrint("cropArea.Width  = %d", window_info.CropArea.Width);
    AmbaPrint("cropArea.Height = %d", window_info.CropArea.Height);
    AmbaPrint("===============");
    AmbaPrint("targetAreaOnPlane.X  = %d", window_info.TargetAreaOnPlane.X);
    AmbaPrint("targetAreaOnPlane.Y  = %d", window_info.TargetAreaOnPlane.Y);
    AmbaPrint("targetAreaOnPlane.Width   = %d", window_info.TargetAreaOnPlane.Width);
    AmbaPrint("targetAreaOnPlane.Height  = %d", window_info.TargetAreaOnPlane.Height);


    return 0;
}

int AmpUT_Display_window_info(AMP_DISP_HDLR_s *dispHdlr)
{
#if 0
    AMP_DISP_INTER_HDLR_s *dispInterHdlr = dispHdlr->ctx;
    AMP_DISP_WINDOW_HDLR_s *windowHdlr = NULL;
    AMP_DISP_WINDOW_CFG_s windowInfo;
    int i;

    AmbaPrint("AmpUT_Display_layout_info");
    AmbaPrint("=========================================");
    AmbaPrint("maxNumWindow = %d", dispInterHdlr->dispCfg.maxNumWindow);
    for (i = 0; i<dispInterHdlr->dispCfg.maxNumWindow; i++) {
        windowHdlr = &((dispInterHdlr->dispCfg.layout.windowArray + i)->public);
        AmpDisplay_GetWindowCfg(windowHdlr, &windowInfo);
        AmbaPrint("Window (%d) Info=================", i);
        AmpUT_Display_layout_content_info(windowInfo);
    }
#endif
    return 0;
}

int AmpUT_Display_info(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
#if 0
    AMP_DISP_HDLR_s *dispHdlr;
    AMP_DISP_CFG_s dispCfg;

    if (strcmp(argv[2],"dchan") == 0) {
        dispHdlr = pDispUTHdlrLCD;
    }else if (strcmp(argv[2],"fchan") == 0) {
        dispHdlr = pDispUTHdlrTV;
    }

    AmbaPrint("AmpUT_Display_info");
    AmbaPrint("=========================================");
    if (strcmp(argv[3],"dev") == 0) {
        AmpUT_Display_dev_info(dispHdlr);
    }else if (strcmp(argv[3],"window") == 0) {
        AmpUT_Display_window_info(dispHdlr);
    }else if (strcmp(argv[3],"others") == 0) {
        AMP_DISP_INTER_HDLR_s *dispInterHdlr = dispHdlr->ctx;
        AmbaPrint("enable      = %d", dispInterHdlr->dispCfg.enable);
        AmbaPrint("systemType   = %d", dispInterHdlr->dispCfg.systemType);
        AmbaPrint("screenRotate = %d", dispInterHdlr->dispCfg.ScreenRotate);
        AmbaPrint("in3dMode     = %d", dispInterHdlr->dispCfg.in3dMode);
        AmbaPrint("out3dMode    = %d", dispInterHdlr->dispCfg.out3dMode);
    }else {
        AmbaPrint("Please Select Info : ");
        AmbaPrint("- dev ");
        AmbaPrint("- window ");
        AmbaPrint("- others ");
    }
#endif
    AmpUT_Display_dev_info(pDispUTHdlrTV);
    return 0;
}

/**
 * Get LCD/TV display handler.\n
 * Also, create the handler if it hasn't been created.
 *
 * @param[in]  Channel          The channel of the display handler. 0 - LCD, 1 - TV.
 * @param[out] pOutDispHdlr     Return the display handler.
 *
 * @return 0 - OK, others - AMP_ER_CODE_e
 * @see AMP_ER_CODE_e
 */
int AmpUT_Display_GetDisplayHandler(UINT8 Channel, AMP_DISP_HDLR_s **pOutDispHdlr)
{
    int Rval = AMP_ERROR_GENERAL_ERROR;

    UTDispMsg(__FUNCTION__, "Begin");

    // If it's not initialized, call AmpUT_Display_Init() to create display handlers.
    if (pDispUTMemPoolAddr == NULL) {
        UTDispMsg(__FUNCTION__, "call AmpUT_Display_Init()");
        if (AmpUT_Display_Init() != OK) {
            UTDispMsg(__FUNCTION__, "Error AmpUT_Display_Init failed");
            return Rval;
        }
    }

    // Display handlers should already be created after AmpUT_Display_Init(), so the handlers should not be NULL.
    // If they're NULL, then AmpUT_Display_Init failed.
    if (pDispUTHdlrLCD == NULL || pDispUTHdlrTV == NULL) {
        UTDispMsg(__FUNCTION__, "Error AmpUT_Display_Init failed");
        return Rval;
    }

    if (pOutDispHdlr == NULL) {
        UTDispMsg(__FUNCTION__, "Error pOutDispHdlr = NULL");
        return Rval;
    }

    if (Channel == 0) {
        *pOutDispHdlr = pDispUTHdlrLCD;
        Rval = AMP_OK;
    } else if (Channel == 1) {
        *pOutDispHdlr = pDispUTHdlrTV;
        Rval = AMP_OK;
    } else {
        UTDispMsg(__FUNCTION__, "Please enter device channel: dchan / fchan");
        return AMP_ERROR_GENERAL_ERROR;
    }

    UTDispMsg(__FUNCTION__, ((Rval==AMP_OK) ? "Success" : "Fail"));
    return Rval;
}

static void AmpUT_Usage(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("Please Enter Test Cmd : ");
    AmbaPrint("- init ");
    AmbaPrint("- cfg [Channel (dchan/fchan)] [Enable (0/1)] [Mode] ");
    AmbaPrint("-     [dchan_Mode] 0: LCD WDF9648W");
    AmbaPrint("-                  1: LCD T20P52");
    AmbaPrint("-                  2: LCD T27P05");
    AmbaPrint("-                  3: LCD T30P61");
    AmbaPrint("-     [fchan_Mode] 0: HDMI  640x 480p/60Hz  4:3");
    AmbaPrint("-                  1: HDMI  720x 480p/60Hz  4:3");
    AmbaPrint("-                  2: HDMI  720x 480p/60Hz 16:9");
    AmbaPrint("-                  3: HDMI 1280x 720p/60Hz 16:9");
    AmbaPrint("-                  4: HDMI 1920x1080i/60Hz 16:9");
    AmbaPrint("-                  5: HDMI 1440x 480i/60Hz  4:3");
    AmbaPrint("-                  6: HDMI 1440x 480i/60Hz 16:9");
    AmbaPrint("-                  7: HDMI 1920x1080p/60Hz 16:9");
    AmbaPrint("-                  8: HDMI  720x 576p/50Hz  4:3");
    AmbaPrint("-                  9: HDMI  720x 576p/50Hz 16:9");
    AmbaPrint("-                 10: HDMI 1280x 720p/50Hz 16:9");
    AmbaPrint("-                 11: HDMI 1920x1080i/50Hz 16:9");
    AmbaPrint("-                 12: HDMI 1440x 576i/50Hz  4:3");
    AmbaPrint("-                 13: HDMI 1440x 576i/50Hz 16:9");
    AmbaPrint("-                 14: HDMI 1920x1080p/50Hz 16:9");
    AmbaPrint("-                 15: CVBS  720x 480i       4:3");

    AmbaPrint("- pxlfmt [Channel (fchan only)] [pxlfmt] ");
    AmbaPrint("-       [pxlfmt]    0: RGB444 8Bits");
    AmbaPrint("-                   1: YCC444 8Bits");
    AmbaPrint("-                   2: YCC422 12Bits");
    AmbaPrint("- start [Channel (dchan/fchan)]");
    AmbaPrint("- stop [Channel (dchan/fchan)]");
    AmbaPrint("- win (window test)");
    AmbaPrint("- info [Channel (dchan/fchan)]");
}

int AmpUT_DisplayTest(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    AmbaPrint("AmpUT_DisplayTest cmd: %s", argv[1]);
    if (strcmp(argv[1],"init") == 0) {
        AmpUT_Display_Init();
    } else if ((strcmp(argv[1],"cfg") == 0) && (argc > 2)) {
        __AmpUT_Display_Config(argc, argv);
    } else if ((strcmp(argv[1],"pxlfmt") == 0) && (argc > 2)) {
        __AmpUT_Display_SetPxlfmt(argc, argv);
    } else if ((strcmp(argv[1],"start") == 0) && (argc > 2)) {
        __AmpUT_Display_Start(argc, argv);
    } else if ( strcmp(argv[1],"pause") == 0) {
        AmpUT_Display_pause(env, argc, argv);
    } else if ( strcmp(argv[1],"resume") == 0) {
        AmpUT_Display_resume(env, argc, argv);
    } else if ((strcmp(argv[1],"stop") == 0) && (argc > 2)) {
        __AmpUT_Display_Stop(argc, argv);
    } else if (strcmp(argv[1],"win") == 0) {
        AmpUT_Display_window_test(argc, argv);
    } else if ( strcmp(argv[1],"end") == 0) {

    } else if ((strcmp(argv[1],"info") == 0) && (argc > 2)) {
        AmpUT_Display_info(env, argc, argv);
    } else if (strcmp(argv[1],"cvbs") == 0) {
        DispUTRunCVBS = atoi(argv[2]);
        AmbaPrint("AmpUT_Cvbs:%d", DispUTRunCVBS);
    } else if (strcmp(argv[1],"seamless") == 0) {
        if (strcmp(argv[2],"on") == 0) {
            UT_Pref_Set(UT_PREF_SEAMLESS, UT_PREF_SEAMLESS_ENABLE);
        } else if (strcmp(argv[2],"off") == 0) {
            UT_Pref_Set(UT_PREF_SEAMLESS, UT_PREF_SEAMLESS_DISABLE);
        }
        AmbaPrint("seamless %d", UT_Pref_Check(UT_PREF_SEAMLESS));
    } else {
        AmpUT_Usage(env, argc, argv);
    }
    return 0;
}

int AmpUT_DisplayInit(void)
{
    // hook command
    AmbaTest_RegisterCommand("disp", AmpUT_DisplayTest);

    return AMP_OK;
}
