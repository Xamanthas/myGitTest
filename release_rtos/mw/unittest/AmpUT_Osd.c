/**
 *  @file AmpUT_Osd.c
 *
 *  **History**
 *      |Date       |Name        |Comments       |
 *      |-----------|------------|---------------|
 *      |2013/7/4 |cyweng       |Created        |
 *
 *  @copyright 2013 Ambarella Corporation. All rights reserved.
 *                 No part of this file may be reproduced, stored in a retrieval system,
 *                 or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                 recording, or otherwise, without the prior consent of Ambarella Corporation.
 */

#include <display/Osd.h>
#include <display/Display.h>
#include "AmpUnitTest.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "AmpUT_Display.h"
#include "AmbaCache.h"
//#include "AmbaGlib.h"

// global var
static AMP_DISP_HDLR_s *LCDDispHdlr = NULL;
static AMP_DISP_HDLR_s *TVDispHdlr = NULL;
static AMP_OSD_HDLR_s *LcdOsdHdlr = NULL;              ///< osd handler for lcd
static AMP_OSD_HDLR_s *TvOsdHdlr = NULL;               ///< osd handler for tv
static AMP_DISP_WINDOW_HDLR_s *LcdWindowHdlr = NULL;   ///< display window handler for LCD
static AMP_DISP_WINDOW_HDLR_s *TvWindowHdlr = NULL;    ///< display window handler for TV
static void *OsdBuf1 = NULL;                           ///< osd buffer address
static void *OsdBuf2 = NULL;                           ///< osd buffer address
static AMP_OSD_BUFFER_CFG_s Buf1Cfg;                   ///< osd config 1
static AMP_OSD_BUFFER_CFG_s Buf2Cfg;                   ///< osd config 2

static UINT8 OsdPixelFmt = AMP_OSD_16BIT_ARGB_4444;

/**
 * Init osd unit test.\n
 * Note: DSP should be boot by other ut first
 * @param osdBits [in] - number of bits per osd pixel
 * @param rgb [in] - is rgb osd or yuv osd.
 * @return
 */
int AmpUT_Osd_init(int osdBits,
                   int rgb)
{
    AMP_OSD_INIT_CFG_s OsdInitCfg = {
            0 };
    AMP_OSD_CFG_s OsdCfg;
    AMP_DISP_WINDOW_CFG_s OsdWindowCfg;
    static int OsdInit = 0;
    static void *OsdhdlrAddr = NULL;
    UINT32 BufPitch = 0;

    AmbaPrint("AmpUT_Osd_init");
    AmpUT_Display_Init();

    // Init OSD module. Provide module required memory
    if (OsdInit == 0) {
        AmpOsd_GetDefaultInitCfg(&OsdInitCfg);
        AmbaKAL_BytePoolAllocate(&G_MMPL, &OsdhdlrAddr, OsdInitCfg.MemoryPoolSize, 100);
        OsdInitCfg.MemoryPoolAddr = OsdhdlrAddr;
        AmpOsd_Init(&OsdInitCfg);
        OsdInit = 1;

        /* Allocatre osd buffer */
        AmbaKAL_BytePoolAllocate(&G_MMPL, &OsdBuf1, 960 * 480 * 4, 100);
        AmbaKAL_BytePoolAllocate(&G_MMPL, &OsdBuf2, 960 * 480 * 4, 100);
        memset(OsdBuf1, 0, 960 * 480 * 4);
        memset(OsdBuf2, 0x80, 960 * 480 * 4);
        AmbaPrint("osdBuf1 = 0x%X, osdBuf2 = 0x%X", OsdBuf1, OsdBuf2);
    }

    // select osd pixel format
    if (osdBits == 8) {
        OsdPixelFmt = AMP_OSD_8BIT_CLUT_MODE;
        BufPitch = 960;
        AmbaPrint("OSD mode set to AMP_OSD_8BIT_CLUT_MODE");
    } else if (osdBits == 16) {
        if (rgb != 0) {
            OsdPixelFmt = AMP_OSD_16BIT_ARGB_4444;
            AmbaPrint("OSD mode set to AMP_OSD_16BIT_ARGB_4444");
        } else {
            OsdPixelFmt = AMP_OSD_16BIT_AYUV_4444;
            AmbaPrint("OSD mode set to AMP_OSD_16BIT_AYUV_4444");
        }
        BufPitch = 960 << 1;
    } else if (osdBits == 32) {
        if (rgb != 0) {
            OsdPixelFmt = AMP_OSD_32BIT_ARGB_8888;
            AmbaPrint("OSD mode set to AMP_OSD_32BIT_ARGB_8888");
        } else {
            OsdPixelFmt = AMP_OSD_32BIT_AYUV_8888;
            AmbaPrint("OSD mode set to AMP_OSD_32BIT_AYUV_8888");
        }
        BufPitch = 960 << 2;
    } else {
        OsdPixelFmt = AMP_OSD_16BIT_AYUV_4444;
        AmbaPrint("OSD mode set to AMP_OSD_16BIT_AYUV_4444");
        BufPitch = 960 << 1;
    }

    // assign disp hdlr
    AmpUT_Display_DispHdlrGet(0, &LCDDispHdlr);
    AmpUT_Display_DispHdlrGet(1, &TVDispHdlr);

    // Create osd handler
    AmpOsd_GetDefaultCfg(&OsdCfg);
    OsdCfg.HwScalerType = HW_OSD_RESCALER_ANY;
    OsdCfg.OsdBufRepeatField = 0;
    OsdCfg.GlobalBlend = 0xFF;
    OsdCfg.BufCfg.BufAddr     = Buf1Cfg.BufAddr = OsdBuf1;
    OsdCfg.BufCfg.BufWidth    = Buf1Cfg.BufWidth = 960;
    OsdCfg.BufCfg.BufHeight   = Buf1Cfg.BufHeight = 480;
    OsdCfg.BufCfg.BufPitch    = Buf1Cfg.BufPitch = BufPitch;
    OsdCfg.BufCfg.PixelFormat = Buf1Cfg.PixelFormat = OsdPixelFmt;
    AmpOsd_Create(&OsdCfg, &LcdOsdHdlr);

    AmpOsd_GetDefaultCfg(&OsdCfg);
    OsdCfg.HwScalerType = HW_OSD_RESCALER_ANY;
    OsdCfg.OsdBufRepeatField = 0;
    OsdCfg.GlobalBlend = 0xFF;
    OsdCfg.BufCfg.BufAddr     = Buf2Cfg.BufAddr = OsdBuf2;
    OsdCfg.BufCfg.BufWidth    = Buf2Cfg.BufWidth = 960;
    OsdCfg.BufCfg.BufHeight   = Buf2Cfg.BufHeight = 480;
    OsdCfg.BufCfg.BufPitch    = Buf2Cfg.BufPitch = BufPitch;
    OsdCfg.BufCfg.PixelFormat = Buf2Cfg.PixelFormat = OsdPixelFmt;
    AmpOsd_Create(&OsdCfg, &TvOsdHdlr);

    // create osd window
    AmpDisplay_GetDefaultWindowCfg(&OsdWindowCfg);
    OsdWindowCfg.Source = AMP_DISP_OSD;
    OsdWindowCfg.SourceDesc.Osd.OsdHdlr = LcdOsdHdlr;
    OsdWindowCfg.CropArea.X = 0;
    OsdWindowCfg.CropArea.X = 0;
    OsdWindowCfg.CropArea.Width = 960;
    OsdWindowCfg.CropArea.Height = 480;
    OsdWindowCfg.TargetAreaOnPlane.Width = 960;
    OsdWindowCfg.TargetAreaOnPlane.Height = 480;
    OsdWindowCfg.TargetAreaOnPlane.X = 0;
    OsdWindowCfg.TargetAreaOnPlane.Y = 0;
    AmpDisplay_CreateWindow(LCDDispHdlr, &OsdWindowCfg, &LcdWindowHdlr);

    OsdWindowCfg.SourceDesc.Osd.OsdHdlr = TvOsdHdlr;
    OsdWindowCfg.TargetAreaOnPlane.X = 100;
    OsdWindowCfg.TargetAreaOnPlane.Y = 100;
    AmpDisplay_CreateWindow(TVDispHdlr, &OsdWindowCfg, &TvWindowHdlr);

    // assgin buf to OSD handler
//    Buf1Cfg.BufAddr = OsdBuf1;
//    Buf1Cfg.BufWidth = 960;
//    Buf1Cfg.BufHeight = 480;
//    Buf1Cfg.BufPitch = BufPitch;
//    Buf1Cfg.PixelFormat = OsdPixelFmt;
//    Buf2Cfg.BufAddr = OsdBuf2;
//    Buf2Cfg.BufWidth = 960;
//    Buf2Cfg.BufHeight = 480;
//    Buf2Cfg.BufPitch = BufPitch;
//    Buf2Cfg.PixelFormat = OsdPixelFmt;

//    AmpOsd_SetBufferCfg(LcdOsdHdlr, &Buf1Cfg);
//    AmpOsd_SetBufferCfg(TvOsdHdlr, &Buf2Cfg);

    return 0;
}

/**
 * active osd window
 *
 * @param argv [in] - window index
 * @return 0: OK
 */
int AmpUT_Osd_start(char **argv)
{
    char *Stop = NULL;
    UINT8 Chan = strtoul(argv[2], &Stop, 0);

    AmbaPrint("AmpUT_Osd_start %d", Chan);
    // active osd window
    if (Chan == 0) {
        AmpUT_Display_Start(0);
        AmpDisplay_SetWindowActivateFlag(LcdWindowHdlr, 1);
        AmpDisplay_Update(LCDDispHdlr);
        //AmpOsd_ActivateOsdWindow(OsdLcdHdlr, WindowLcdHdlr);
    } else if (Chan == 1) {
        AmpUT_Display_Start(1);
        AmpDisplay_SetWindowActivateFlag(TvWindowHdlr, 1);
        AmpDisplay_Update(TVDispHdlr);
        //AmpOsd_ActivateOsdWindow(OsdTvHdlr, WindowTvHdlr);
    }
    return 0;
}

/**
 * deactivate osd window
 *
 * @param argv [in] - window index
 * @return 0: OK
 */
int AmpUT_Osd_stop(char **argv)
{
    char *Stop = NULL;
    UINT8 Chan = strtoul(argv[2], &Stop, 0);

    // deactive osd window
    AmbaPrint("AmpUT_Osd_stop");
    if (Chan == 0) {
//        AmpOsd_DeActivateOsdWindow(OsdLcdHdlr, WindowLcdHdlr);
        AmpDisplay_SetWindowActivateFlag(LcdWindowHdlr, 0);
        AmpDisplay_Update(LCDHdlr);
    } else if (Chan == 1) {
//        AmpOsd_DeActivateOsdWindow(OsdTvHdlr, WindowTvHdlr);
        AmpDisplay_SetWindowActivateFlag(TvWindowHdlr, 0);
        AmpDisplay_Update(TVHdlr);
    }
    return 0;
}

/**
 * Set buffer to window, used to test buffer switch
 * @param argv - [2] channel [3] buffer config
 */
int AmpUT_Osd_SetBuf(char **argv)
{
    char *Stop = NULL;
    UINT8 Chan = strtoul(argv[2], &Stop, 0);
    UINT8 BufIdx = strtoul(argv[3], &Stop, 0);

    AMP_OSD_HDLR_s *OsdHdlr = NULL;

    if (Chan == 0) {
        OsdHdlr = LcdOsdHdlr;
    } else if (Chan == 1) {
        OsdHdlr = TvOsdHdlr;
    }

    // assinge buffer to osd handler
    if (BufIdx == 0) {
        AmpOsd_SetBufferCfg(OsdHdlr, &Buf1Cfg);
    } else if (BufIdx == 1) {
        AmpOsd_SetBufferCfg(OsdHdlr, &Buf2Cfg);
    }

    // active window
/*    if (Chan == 0) {
        AmpOsd_ActivateOsdWindow(OsdHdlr, WindowLcdHdlr);
    } else if (Chan == 1) {
        AmpOsd_ActivateOsdWindow(OsdHdlr, WindowTvHdlr);
    }
*/
    AmbaPrint("AmpUT_Osd_SetBuf: set buf:%d to ch:%d", BufIdx, Chan);
    return 0;
}

/**
 * Used to test OSD buffer crop and scaling.
 *
 * @param argv [in] - window setting
 * @return 0: ok
 */
int AmpUT_Osd_SetWindow(char **argv)
{
    char *Stop = NULL;
    UINT8 Chan = strtoul(argv[2], &Stop, 0);
    UINT16 Cropx = strtoul(argv[3], &Stop, 0);
    UINT16 Cropy = strtoul(argv[4], &Stop, 0);
    UINT16 Cropw = strtoul(argv[5], &Stop, 0);
    UINT16 Croph = strtoul(argv[6], &Stop, 0);
    UINT16 Dispx = strtoul(argv[7], &Stop, 0);
    UINT16 Dispy = strtoul(argv[8], &Stop, 0);
    UINT16 Dispw = strtoul(argv[9], &Stop, 0);
    UINT16 Disph = strtoul(argv[10], &Stop, 0);

    AMP_DISP_WINDOW_CFG_s OsdWindow;
    AMP_DISP_WINDOW_HDLR_s *WindowHdlr = NULL;

    if (Chan == 0)
        WindowHdlr = LcdWindowHdlr;
    else if (Chan == 1)
        WindowHdlr = TvWindowHdlr;

    // set new window parameter, need to update window again.
    AmpDisplay_GetWindowCfg(WindowHdlr, &OsdWindow);
    OsdWindow.CropArea.Width = Cropw;
    OsdWindow.CropArea.Height = Croph;
    OsdWindow.CropArea.X = Cropx;
    OsdWindow.CropArea.Y = Cropy;
    OsdWindow.TargetAreaOnPlane.Width = Dispw;
    OsdWindow.TargetAreaOnPlane.Height = Disph;
    OsdWindow.TargetAreaOnPlane.X = Dispx;
    OsdWindow.TargetAreaOnPlane.Y = Dispy;
    AmpDisplay_SetWindowCfg(WindowHdlr, &OsdWindow);

    AmbaPrint("AmpUT_Osd_SetWindow");
    return 0;
}

/**
 * Paint test pattern to buf.
 *
 * @param argv [in] - [2] buffer index, [3] - color test block
 * @return
 */
int AmpUT_Osd_PaintBuf(char **argv)
{
    char *Stop = NULL;
    UINT8 BufIdx = strtoul(argv[2], &Stop, 0);
    UINT32 Color = strtoul(argv[3], &Stop, 0);
    UINT8* Buf;
    UINT32* U32Cur;
    UINT16* U16Cur;
    UINT8* U8Cur;
    AMP_OSD_BUFFER_CFG_s *BufCfg;
    int C, R;

    if (BufIdx == 0) {
        Buf = OsdBuf1;
        BufCfg = &Buf1Cfg;
    } else {
        Buf = OsdBuf2;
        BufCfg = &Buf2Cfg;
    }

    AmbaPrint("Paint GRGB on OSD Buf");
    switch (BufCfg->PixelFormat) {
    case AMP_OSD_32BIT_AYUV_8888:
        U32Cur = (UINT32*) Buf;
        for (R = 0; R < 480; R++) {
            for (C = 0; C < 960; C++) {
                switch (((R / 100) + (C / 100)) % 4) {
                case 0:
                    *U32Cur = 0x8000FF00;
                    break;
                case 1:
                    *U32Cur = 0x80FF0000;
                    break;
                case 2:
                    *U32Cur = 0x80FF8080;
                    break;
                case 3:
                    *U32Cur = Color;
                    break;
                default:
                    *U32Cur = 0x80FFFFFF;
                    break;
                }
                U32Cur++;
            }
        }
        break;
    case AMP_OSD_32BIT_ARGB_8888:
        U32Cur = (UINT32*) Buf;
        for (R = 0; R < 480; R++) {
            for (C = 0; C < 960; C++) {
                switch (((R / 100) + (C / 100)) % 4) {
                case 0:
                    *U32Cur = 0x80FFFFFF;
                    break;
                case 1:
                    *U32Cur = 0x80FF0000;
                    break;
                case 2:
                    *U32Cur = 0x8000FF00;
                    break;
                case 3:
                    *U32Cur = Color;
                    break;
                default:
                    *U32Cur = 0x80FFFFFF;
                    break;
                }
                U32Cur++;
            }
        }
        break;
    case AMP_OSD_16BIT_AYUV_4444:
        U16Cur = (UINT16*) Buf;
        for (R = 0; R < 480; R++) {
            for (C = 0; C < 960; C++) {
                switch (((R / 100) + (C / 100)) % 4) {
                case 0:
                    *U16Cur = 0x80F0;
                    break;
                case 1:
                    *U16Cur = 0x8F00;
                    break;
                case 2:
                    *U16Cur = 0x8F88;
                    break;
                case 3:
                    *U16Cur = Color;
                    break;
                default:
                    *U16Cur = 0x8FFF;
                    break;
                }
                U16Cur++;
            }
        }
        break;
    case AMP_OSD_16BIT_ARGB_4444:
        U16Cur = (UINT16*) Buf;
        for (R = 0; R < 480; R++) {
            for (C = 0; C < 960; C++) {
                switch (((R / 100) + (C / 100)) % 4) {
                case 0:
                    *U16Cur = 0x8FFF;
                    break;
                case 1:
                    *U16Cur = 0x8F00;
                    break;
                case 2:
                    *U16Cur = 0x80F0;
                    break;
                case 3:
                    *U16Cur = Color;
                    break;
                default:
                    *U16Cur = 0x8FFF;
                    break;
                }
                U16Cur++;
            }
        }
        break;
    case AMP_OSD_8BIT_CLUT_MODE:
        U8Cur = (UINT8*) Buf;
        for (R = 0; R < 480; R++) {
            for (C = 0; C < 960; C++) {
                switch (((R / 100) + (C / 100)) % 4) {
                case 0:
                    *U8Cur = 0;
                    break;
                case 1:
                    *U8Cur = 1;
                    break;
                case 2:
                    *U8Cur = 2;
                    break;
                case 3:
                    *U8Cur = Color;
                    break;
                default:
                    *U8Cur = 3;
                    break;
                }
                U8Cur++;
            }
        }
        break;
    default:
        U32Cur = (UINT32*) Buf;
        for (R = 0; R < 480; R++) {
            for (C = 0; C < 960; C++) {
                switch (C / 240) {
                case 0:
                    *U32Cur = 0xFF00FF00;   //G
                    break;
                case 1:
                    *U32Cur = 0xFFFF0000;   //R
                    break;
                case 2:
                    *U32Cur = 0xFF00FF00;   //G
                    break;
                case 3:
                    *U32Cur = 0xFF0000FF;   //B
                    break;
                default:
                    *U32Cur = 0xFFFFFFFF;   //W
                    break;
                }
                U32Cur++;
            }
        }
        break;
    }

    AmbaCache_Clean(Buf, 960*480*4);
    AmbaPrint("AmpUT_Osd_PaintBuf: paint buf:%d ", BufIdx);
    return 0;
}

static void AmpUT_Osd_Usage(struct _AMBA_SHELL_ENV_s_ *env,
                            int argc,
                            char **argv)
{
    AmbaPrint("Please Enter Test Cmd : ");
    AmbaPrint("t osd init [osd bits] [rgb mode]");
    AmbaPrint("t osd start [channel (0/1)");
    AmbaPrint("t osd stop [channel (0/1)]");
    AmbaPrint("t osd setBuf [buf idx(0/1)] [channel (0/1)]");
    AmbaPrint("t osd paintBuf [buf idx(0/1)] [test color]");
    AmbaPrint(
            "t osd setWindow [channel (0/1)] [crop x]  [crop y]  [crop width]  [crop height] [display x]  [display y]  [display width]  [display height]");
}

int AmpUT_OsdTest(struct _AMBA_SHELL_ENV_s_ *env,
                  int argc,
                  char **argv)
{
    AmbaPrint("AmpUT_OsdTest cmd: %s", argv[1]);
    if (strcmp(argv[1], "init") == 0) {
        AmpUT_Osd_init(atoi(argv[2]), atoi(argv[3]));
    } else if (strcmp(argv[1], "start") == 0) {
        AmpUT_Osd_start(argv);
    } else if (strcmp(argv[1], "stop") == 0) {
        AmpUT_Osd_stop(argv);
    } else if (strcmp(argv[1], "setBuf") == 0) {
        AmpUT_Osd_SetBuf(argv);
    } else if (strcmp(argv[1], "paintBuf") == 0) {
        AmpUT_Osd_PaintBuf(argv);
    } else if (strcmp(argv[1], "setWindow") == 0) {
        AmpUT_Osd_SetWindow(argv);
    } else {
        AmpUT_Osd_Usage(env, argc, argv);
    }

    return 0;
}

int AmpUT_OsdInit(void)
{
    // hook command
    AmbaTest_RegisterCommand("osd", AmpUT_OsdTest);

    return AMP_OK;
}
