/**
  * @file app/connected/app/system/t_app.c
  *
  * Implementation of Application testing code
  *
  * History:
  *    2013/08/13 - [Martin Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#include <framework/apphmi.h>
#include <framework/appdefines.h>
#include <apps/apps.h>
#include <AmbaKAL.h>
#include <AmbaShell.h>
#include <AmbaTest.h>
#include <AmbaPLL.h>
#include <recorder/ApplibRecorder_ExtendEnc.h>
#include "audio/ApplibAudio_beep.h"
#include "AmbaI2S_Def.h"
#include "AmbaRTSL_PLL.h"
#include "AmbaAudio_CODEC.h"
#include <recorder/StillEnc.h>
#include <AmbaPWC.h>

#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
#include "app_pref.h"
#include "net/NetUtility.h"
#include <transcoder/still_decode/ApplibTranscoder_Thumb_Basic.h>
#endif

static UINT8 TAppYuvStack[(32<<10)];            /**< Stack for Yuvinput */
static UINT8 TAppTriAStack[(32<<10)];
#include <usb/ApplibUsb.h>
#include <framework/appmaintask.h>

/**
 *  @brief A test command to send message
 *
 *  A test command to send message
 *
 *  @param [in] env Amba shell
 *  @param [in] argc Argument count
 *  @param [in] argv Argument vector
 *
 *  @return >=0 success, <0 failure
 */
static int t_app_msg(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    int ReturnValue = 0;
    UINT32 msg = 0, param1 = 0, param2 = 0;
    char *stop;

    if (argc > 2) {
        msg = strtoul(argv[2], &stop, 0);
        param1 = strtoul(argv[3], &stop, 0);
        param2 = strtoul(argv[4], &stop, 0);
        ReturnValue = AppLibComSvcHcmgr_SendMsg(msg, param1, param2);
    } else {
       ReturnValue = AmbaShell_Print(env,
        "Usage: t app msg <msg> <param1> <param2>\n");
    }

    return ReturnValue;
}

/**
 *  @brief A test command to send the key message
 *
 *  A test command to send  the key message
 *
 *  @param [in] env Amba shell
 *  @param [in] argc Argument count
 *  @param [in] argv Argument vector
 *
 *  @return >=0 success, <0 failure
 */
static int t_app_key(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    int ReturnValue = 0;
#define T_APP_KEY_USAGE (-2)

    if (argc > 2) {
        if (strcmp(argv[2], "up") == 0) {
            ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_USER_UP_BUTTON, 0, 0);
        } else if (strcmp(argv[2], "up_rel") == 0) {
            ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_USER_UP_BUTTON_CLR, 0, 0);
        } else if (strcmp(argv[2], "down") == 0) {
            ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_USER_DOWN_BUTTON, 0, 0);
        } else if (strcmp(argv[2], "down_rel") == 0) {
            ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_USER_DOWN_BUTTON_CLR, 0, 0);
        } else if (strcmp(argv[2], "left") == 0) {
            ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_USER_LEFT_BUTTON, 0, 0);
        } else if (strcmp(argv[2], "left_rel") == 0) {
            ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_USER_LEFT_BUTTON_CLR, 0, 0);
        } else if (strcmp(argv[2], "right") == 0) {
            ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_USER_RIGHT_BUTTON, 0, 0);
        } else if (strcmp(argv[2], "right_rel") == 0) {
            ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_USER_RIGHT_BUTTON_CLR, 0, 0);
        } else if (strcmp(argv[2], "zoomin") == 0) {
            ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_USER_ZOOM_IN_BUTTON, 0, 0);
        } else if (strcmp(argv[2], "zoomin_rel") == 0) {
            ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_USER_ZOOM_IN_BUTTON_CLR, 0, 0);
        } else if (strcmp(argv[2], "zoomout") == 0) {
            ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_USER_ZOOM_OUT_BUTTON, 0, 0);
        } else if (strcmp(argv[2], "zoomout_rel") == 0) {
            ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_USER_ZOOM_OUT_BUTTON_CLR, 0, 0);
        } else if (strcmp(argv[2], "set") == 0) {
            ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_USER_SET_BUTTON, 0, 0);
        } else if (strcmp(argv[2], "f4") == 0) {
            ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_USER_F4_BUTTON, 0, 0);
        } else if (strcmp(argv[2], "focus") == 0) {
            ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_USER_SNAP1_BUTTON, 0, 0);
        } else if (strcmp(argv[2], "focus_rel") == 0) {
            ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_USER_SNAP1_BUTTON_CLR, 0, 0);
        } else if (strcmp(argv[2], "shutter") == 0) {
            ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_USER_SNAP2_BUTTON, 0, 0);
        } else if (strcmp(argv[2], "shutter_rel") == 0) {
            ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_USER_SNAP2_BUTTON_CLR, 0, 0);
        } else if (strcmp(argv[2], "record") == 0) {
            ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_USER_RECORD_BUTTON, 0, 0);
        } else if (strcmp(argv[2], "mode") == 0) {
            ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_USER_MODE_BUTTON, 0, 0);
        } else if (strcmp(argv[2], "menu") == 0) {
            ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_USER_MENU_BUTTON, 0, 0);
        } else if (strcmp(argv[2], "power") == 0) {
            ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_USER_POWER_BUTTON, 0, 0);
        } else if (strcmp(argv[2], "power_rel") == 0) {
            ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_USER_POWER_BUTTON_CLR, 0, 0);
        } else if (strcmp(argv[2], "del") == 0) {
            ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_USER_DEL_BUTTON, 0, 0);
        } else {
            ReturnValue = T_APP_KEY_USAGE;
        }
    } else {
        ReturnValue = T_APP_KEY_USAGE;
    }

    if (ReturnValue == T_APP_KEY_USAGE) {
        ReturnValue = AmbaShell_Print(env,
            "Usage: t app key <key>\n"
            "Key list:\n"
            "\t up \n"
            "\t up_rel \n"
            "\t down \n"
            "\t down_rel \n"
            "\t left \n"
            "\t left_rel \n"
            "\t right \n"
            "\t right_rel \n"
            "\t zoomin \n"
            "\t zoomin_rel \n"
            "\t zoomout \n"
            "\t zoomout_re l\n"
            "\t set \n"
            "\t focus \n"
            "\t focus_rel \n"
            "\t shutter \n"
            "\t shutter_rel \n"
            "\t record \n"
            "\t mode \n"
            "\t menu \n"
            "\t power \n"
            "\t del \n");
    }

    return ReturnValue;
}

/**
 *  @brief A test command to send the jack message
 *
 *  A test command to send  the jack message
 *
 *  @param [in] env Amba shell
 *  @param [in] argc Argument count
 *  @param [in] argv Argument vector
 *
 *  @return >=0 success, <0 failure
 */
static int t_app_jack(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    int ReturnValue = 0;
    UINT32 event = 0;
#define T_APP_JACK_USAGE    (-2)

    if (argc > 3) {
        if (strcmp(argv[3], "in") == 0) {
            event = 1;
        } else if (strcmp(argv[3], "out") == 0) {
            event = 0;
        } else {
            ReturnValue = T_APP_JACK_USAGE;
        }
        if (ReturnValue == 0) {
            if (strcmp(argv[2], "hdmi") == 0) {
                ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_JACK_AV_HDMI(event), 0, 0);
            } else if (strcmp(argv[2], "cs") == 0) {
                ReturnValue = AppLibComSvcHcmgr_SendMsg(HMSG_JACK_AV_CS(event), 0, 0);
            } else {
                ReturnValue = T_APP_JACK_USAGE;
            }
        }
    } else {
        ReturnValue = T_APP_JACK_USAGE;
    }

    if (ReturnValue == T_APP_JACK_USAGE) {
        ReturnValue = AmbaShell_Print(env,
        "Usage: t app jack <jack> <event>\n"
        "Jack list:\n"
        "\thdmi\n"
        "\tcs\n"
        "Event list:\n"
        "\tin\n"
        "\tout\n");
    }

    return ReturnValue;
}

/**
 *  @brief A test command to send the jack message
 *
 *  A test command to send  the jack message
 *
 *  @param [in] env Amba shell
 *  @param [in] argc Argument count
 *  @param [in] argv Argument vector
 *
 *  @return >=0 success, <0 failure
 */

#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
#define IMAGE_BUF_SIZE (2 << 20)
static UINT8 ImageBuf[IMAGE_BUF_SIZE] = {0};

static int t_app_test_idr(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    int ReturnValue = 0;
    int FileId = 0;
    TRANS_STILL_DATA_BUF_s DataBuf = {0};

    #define T_APP_TEST_IRD_USAGE    (-2)

    if (strcmp(argv[3], "init") == 0) {
        AmbaPrintColor(YELLOW, "---------------> t app test idr init");
        AppLibTranscoderThmBasic_Init();
    } else if(strcmp(argv[3], "vid") == 0) {
        AmbaPrintColor(YELLOW, "---------------> t app test idr vid");
         if (argc < 5) {
            ReturnValue = T_APP_TEST_IRD_USAGE;
        } else {
            DataBuf.Buf = ImageBuf;
            DataBuf.BufSize = IMAGE_BUF_SIZE;
            DataBuf.RetDataSize = 0;
            //AppLibTranscoderThmBasic_GetIdrFrame("C:\\DCIM\\140101200\\005924AA.MP4", &DataBuf);
            AppLibTranscoderThmBasic_GetIdrFrame(argv[4], &DataBuf);
        }
    } else if(strcmp(argv[3], "img") == 0) {
        AmbaPrintColor(YELLOW, "---------------> t app test idr img");
        if (argc < 6) {
            ReturnValue = T_APP_TEST_IRD_USAGE;
        } else {
            FileId = atoi(argv[4]);
            if ((FileId >=0) && (FileId <= 2)) {
                DataBuf.Buf = ImageBuf;
                DataBuf.BufSize = IMAGE_BUF_SIZE;
                DataBuf.RetDataSize = 0;
                //AppLibTranscoderThmBasic_GetImage("C:\\DCIM\\140101100\\02111700.JPG", FileId, &DataBuf);
                AppLibTranscoderThmBasic_GetImage(argv[5], FileId, &DataBuf);
                AmbaPrintColor(YELLOW, "DataBuf.RetDataSize = %d",DataBuf.RetDataSize);
            } else {
                AmbaPrint("num should be 0~2");
                ReturnValue = T_APP_TEST_IRD_USAGE;
            }
        }
    } else if(strcmp(argv[3], "uninit") == 0) {
        AmbaPrintColor(YELLOW, "---------------> t app test idr uninit");
        AppLibTranscoderThmBasic_Uninit();
    } else {
        ReturnValue = T_APP_TEST_IRD_USAGE;
    }

    if (ReturnValue == T_APP_TEST_IRD_USAGE) {
        ReturnValue = AmbaShell_Print(env,
        "Usage: t app test idr <option>\n"
        "    Test option list:\n"
        "\t init\n"
        "\t vid <filename> \n"
        "\t img <img source> <filename> \n"
        "\t     <img source> 0:full, 1:thumbnail, 2:screennail\n"
        "\t uninit \n"
        );
    }

    return 0;
}

static int t_app_test_np(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    int ReturnValue = 0;
    int FileId = 1;
    APP_NET_PB_MESSAGE_s Msg = {0};

    #define T_APP_TEST_NP_USAGE    (-2)

    if (argc <= 1) {
        ReturnValue = T_APP_TEST_NP_USAGE;
    } else {
        if (strcmp(argv[3], "open") == 0) {
            if (argc < 5) {
                ReturnValue = T_APP_TEST_NP_USAGE;
            } else {
                FileId = atoi(argv[4]);
                if (FileId > 0) {
                    Msg.MessageID = APPLIB_NETFIFO_PLAYBACK_OPEN;
                    snprintf(Msg.Filename, sizeof(Msg.Filename),"C:\\DCIM\\100MEDIA\\AMBA%04d.MP4", FileId);
                    AppLibNetFifo_PlaybackSendMsg(&Msg, AMBA_KAL_NO_WAIT);
                } else {
                    AmbaPrint("num should be larger than 0");
                    ReturnValue = T_APP_TEST_NP_USAGE;
                }
            }
        } else if (strcmp(argv[3], "play") == 0) {
            Msg.MessageID = APPLIB_NETFIFO_PLAYBACK_PLAY;
            AppLibNetFifo_PlaybackSendMsg(&Msg, AMBA_KAL_NO_WAIT);
        } else if (strcmp(argv[3], "stop") == 0) {
            Msg.MessageID = APPLIB_NETFIFO_PLAYBACK_STOP;
            AppLibNetFifo_PlaybackSendMsg(&Msg, AMBA_KAL_NO_WAIT);
        } else if (strcmp(argv[3], "pause") == 0) {
            Msg.MessageID = APPLIB_NETFIFO_PLAYBACK_PAUSE;
            AppLibNetFifo_PlaybackSendMsg(&Msg, AMBA_KAL_NO_WAIT);
        } else if (strcmp(argv[3], "resume") == 0) {
            Msg.MessageID = APPLIB_NETFIFO_PLAYBACK_RESUME;
            AppLibNetFifo_PlaybackSendMsg(&Msg, AMBA_KAL_NO_WAIT);
        } else if (strcmp(argv[3], "next") == 0) {
            if (argc < 5) {
                ReturnValue = T_APP_TEST_NP_USAGE;
            } else {
                Msg.MessageID = APPLIB_NETFIFO_PLAYBACK_PLAY;
                Msg.MessageData[0] = 0;
                snprintf(Msg.Filename, sizeof(Msg.Filename), "%s", argv[4]);
                ReturnValue = AppLibNetFifo_PlaybackSendMsg(&Msg, AMBA_KAL_NO_WAIT);
                if (ReturnValue != 0) {
                     AmbaPrint("[t_app] <t_app_test_np> AppLibNetFifo_PlaybackSendMsg() fail");
                     ReturnValue = -1;
                }
                AmbaPrint("[t_app] online playback feed next file: %s", Msg.Filename);
            }
        } else {
            ReturnValue = T_APP_TEST_NP_USAGE;
        }
    }

    if (ReturnValue == T_APP_TEST_NP_USAGE) {
        ReturnValue = AmbaShell_Print(env,
        "Usage: t app test np <option>\n"
        "    Test option list:\n"
        "\t open <num>\n"
        "\t play \n"
        "\t stop \n"
        "\t pause \n"
        "\t resume \n"
        "\t next \n"
        );
    }

    return 0;
}

#endif
static int t_app_test_usbdbg(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    int ReturnValue = 0;
#ifdef 	CONFIG_ASD_USB_RS232_ENABLE

    #define T_APP_TEST_USBDBG_USAGE    (-2)

    if (strcmp(argv[3], "start") == 0) {
        AppUserConsole_SetWriteFunc(ApplibUsbCdcAcmMulti_Write);
        AppUserConsole_SetReadFunc(ApplibUsbCdcAcmMulti_Read);
        ApplibUsbCdcAcmMulti_Start();
    } else if (strcmp(argv[3], "2uart") == 0) {
        AppUserConsole_SetWriteFunc((APP_CONSOLE_PUT_CHAR_f)AmbaUART_Write);
        AppUserConsole_SetReadFunc((APP_CONSOLE_GET_CHAR_f)AmbaUART_Read);

    } else if (strcmp(argv[3], "2usb") == 0) {

        AppUserConsole_SetWriteFunc(ApplibUsbCdcAcmMulti_Write);
        AppUserConsole_SetReadFunc(ApplibUsbCdcAcmMulti_Read);
    } else {
        ReturnValue = T_APP_TEST_USBDBG_USAGE;
    }

    if (ReturnValue == T_APP_TEST_USBDBG_USAGE) {
        ReturnValue = AmbaShell_Print(env,
        "Usage: t app test usbdbg <option>\n"
        "    Test option list:\n"
        "\t start \n"
        "\t 2uart \n"
        "\t 2usb \n"
        );
    }
#endif //CONFIG_ASD_USB_RS232_ENABLE
    return 0;
}

static int t_app_test_bb(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    APPLIB_AUDIO_BEEP_TASK_MSG_s BeepMsg;
    //unsigned char BeepFile[] = "beep48k.bin";
    int ReturnValue = 0;
    int SrcChannelMode = 2;
    UINT32 SrcSampleRate = 0;

    #define T_APP_TEST_BEEP_USAGE    (-2)
    //extern UINT8 BeepShortShutter48K2Ch[8688];

    AppLibAudio_BeepInit();

    if (strcmp(argv[3], "user") == 0) {
        #if 0
        BeepMsg.MessageType = APPLIB_AUDIO_BEEP_TASK_MSG_PLAY_BEEP;
        snprintf(BeepMsg.beepInfo.Fn,sizeof(BeepMsg.beepInfo.Fn),"%s",BeepFile);
        BeepMsg.beepInfo.BeepType = BEEP_FROM_USER;
        BeepMsg.beepInfo.RawPcm = BeepShortShutter48K2Ch;
        BeepMsg.beepInfo.RawSize = 8688;
        BeepMsg.beepInfo.SrcChannelMode = 2;
        BeepMsg.beepInfo.SrcSampleRate = 48000;
        AppLibAudio_SendMsg(&BeepMsg, AMBA_KAL_WAIT_FOREVER);
        #else
        AmbaPrint("[t_app] not support now!");
        #endif
    } else if (strcmp(argv[3], "file") == 0) {
        /* Playback beep sound. */
        if (argc <= 4) {
            ReturnValue = T_APP_TEST_BEEP_USAGE;
        } else {
            BeepMsg.MessageType = APPLIB_AUDIO_BEEP_TASK_MSG_PLAY_BEEP;
            snprintf(BeepMsg.beepInfo.Fn,sizeof(BeepMsg.beepInfo.Fn),"%s",argv[4]);
            BeepMsg.beepInfo.BeepType = BEEP_FROM_FILE;
            BeepMsg.beepInfo.RawPcm = NULL;
            BeepMsg.beepInfo.RawSize = 0;
            BeepMsg.beepInfo.SrcChannelMode = 2;
            BeepMsg.beepInfo.SrcSampleRate = 48000;
            AppLibAudio_SendMsg(&BeepMsg, AMBA_KAL_WAIT_FOREVER);
        }
    } else if (strcmp(argv[3], "file2") == 0) {
        /* Playback beep sound. */
        if (argc <= 6) {
            ReturnValue = T_APP_TEST_BEEP_USAGE;
        } else {
            SrcChannelMode = atoi(argv[5]);
            SrcSampleRate = (UINT32) atoi(argv[6]);
            AmbaPrint("---> SrcChannelMode = %d, SrcSampleRate = %d", SrcChannelMode, SrcSampleRate);

            if (SrcSampleRate == 11025) {
                AmbaRTSL_PllSetAudioClk(11289600);
                // TODO: unmark this when audio team is ready.
                AmbaI2S_SetOverSampingRate(AMBA_I2S_CHANNEL0, AMBA_I2S_CLK_FREQ_1024FS);
                AmbaAudio_CodecFreqConfig(AMBA_AUDIO_CODEC_0, 11025);
            }

            if ((SrcSampleRate != 11025) && (SrcSampleRate != 48000)) {
                AmbaPrint("\n\nWarning: source sampling rate is not 11025 or 48000!!!");
            }

            BeepMsg.MessageType = APPLIB_AUDIO_BEEP_TASK_MSG_PLAY_BEEP;
            snprintf(BeepMsg.beepInfo.Fn,sizeof(BeepMsg.beepInfo.Fn),"%s",argv[4]);
            BeepMsg.beepInfo.BeepType = BEEP_FROM_FILE;
            BeepMsg.beepInfo.RawPcm = NULL;
            BeepMsg.beepInfo.RawSize = 0;
            BeepMsg.beepInfo.SrcChannelMode = SrcChannelMode;
            BeepMsg.beepInfo.SrcSampleRate = SrcSampleRate;
            AppLibAudio_SendMsg(&BeepMsg, AMBA_KAL_WAIT_FOREVER);
        }
    } else {
        ReturnValue = T_APP_TEST_BEEP_USAGE;
    }

    if (ReturnValue == T_APP_TEST_BEEP_USAGE) {
        ReturnValue = AmbaShell_Print(env,
        "Usage: t app test bb <option>\n"
        "    Test option list:\n"
        "\t user\n"
        "\t file <filename>\n"
        "\t file2 <filename> <SrcChannelMode> <SrcSampleRate>\n"
        );
    }

    return 0;
}


static int t_app_test(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    int ReturnValue = 0;
    UINT32 rawaddr;

#define T_APP_TEST_USAGE    (-2)

    if (argc > 2) {
        if (strcmp(argv[2], "chg_res") == 0) {
            int resID = 0;
            resID = atoi(argv[3]);
            AppLibVideoEnc_SetSensorVideoRes(resID);
            AppLibComSvcHcmgr_SendMsg(AMSG_CMD_SET_VIDEO_RES, 0, 0);   //test message

        } else if (strcmp(argv[2], "chg_mode") == 0) {
            int bev = 0;
            bev = atoi(argv[3]);
            AppLibComSvcHcmgr_SendMsg(AMSG_CMD_SET_APP_ENC_MODE, bev, 0);   //test message
        } else if (strcmp(argv[2], "chg_app") == 0) {
            extern int AppUtil_SwitchApp(int appId);
            int bev = 0;
            bev = atoi(argv[3]);

            AppUtil_SwitchApp(bev);
        } else if (strcmp(argv[2], "graph") == 0) {
            if (strcmp(argv[3], "set_debug_level") == 0) {
                int level = atoi(argv[4]);
                switch (level) {
                    case 0x01:
                    case 0x02:
                    case 0x04:
                        GraphicsPrint_SetLevel(level);
                        break;
                    default:
                        AmbaPrint("1: DEBUG_ERR, 2: DEBUG_WARNING, 4: DEBUG_ONLY");
                        break;
                }
            } else if (strcmp(argv[3], "set_debug_module") == 0) {
                int module = atoi(argv[4]);
                if (module == 0) {
                    AmbaPrint("0: DEBUG_MODULE_NONE \n"
                              "1: DEBUG_MODULE_GRAPHICS \n"
                              "2: DEBUG_MODULE_MAINTASK \n"
                              "4: DEBUG_MODULE_SHAPE \n"
                              "8: DEBUG_MODULE_BITMAP \n"
                              "16: DEBUG_MODULE_STING \n"
                              "32: DEBUG_MODULE_FONT \n"
                              "64: DEBUG_MODULE_STAMP \n"
                              "128: DEBUG_MODULE_CANVAS \n"
                              "256: DEBUG_MODULE_RENDER \n"
                              "512: DEBUG_MODULE_UIOBJ \n");
                } else {
                    GraphicsPrint_SetModule(module);
                }
            }else {
                ReturnValue = T_APP_TEST_USAGE;
            }
        } else if (strcmp(argv[2], "save_osd") == 0) {
            int chnId = 0;
            extern void AppLibGraph_SaveAsBMP(UINT32 graphChannelId);
            chnId = atoi(argv[3]);
            AppLibGraph_SaveAsBMP(chnId);
        } else if (strcmp(argv[2], "rawcap") == 0) {
            /** t app test rawcap [cmpr][file_name]*/
            extern void usb_amage_set_capture_type(UINT32 CapType);
            UINT16 RawPitch = 0, RawWidth = 0, RawHeight = 0;
            char fn[64];
            char mode[3] = {'w','b','\0'};
            AMBA_FS_FILE *raw = NULL;
            //AMBA_SENSOR_MODE_INFO_s VinInfo;
            APPLIB_SENSOR_STILLCAP_MODE_CONFIG_s *pMode = NULL;

            if (argc < 5) {
                AmbaPrint("Usage: t app test rawcap [Cmpr][filename]");
                AmbaPrint("             Cmpr:0 : Normal Raw, 1 : Compress Raw.");
                AmbaPrint("             filename: Dump raw file name.     ");
            } else {

                pMode = AppLibSysSensor_GetStillCaptureModeConfig(AppLibStillEnc_GetPhotoPjpegCapMode(), AppLibStillEnc_GetPhotoPjpegConfigId());
                RawWidth =  pMode->CaptureWidth;
                RawHeight = pMode->CaptureHeight;
                RawPitch = (atoi(argv[3]))? \
                    AMP_COMPRESSED_RAW_WIDTH(RawWidth):RawWidth*2;
                RawPitch = ALIGN_32(RawPitch);

                sprintf(fn,"%s", argv[4]);
                usb_amage_set_capture_type(2);/**<Set amage capture type(still raw cap)*/
                AppLibStillEnc_SetRawCaptureCmpr(atoi(argv[3]));
                AppLibStillEnc_CaptureRaw(&rawaddr);
                AmbaKAL_TaskSleep(500);
                raw = AmbaFS_fopen((char const *)fn,(char const *) mode);
                AmbaFS_fwrite((const void *)rawaddr,RawPitch*RawHeight, 1, raw);
                AmbaFS_fclose(raw);
                AmbaPrint("[RAWCAP] Dump Raw finish! Raw Addr = 0x%x RawPitch (%d) RawHeight (%d) RawWidth (%d) filename (%s)",rawaddr,RawPitch,RawHeight,RawWidth,fn);
            }
        } else if (strcmp(argv[2], "videorawcap") == 0) {
            extern void usb_amage_set_capture_type(UINT32 CapType);

            if (argc < 7) {
                AmbaPrint("Usage: t app test videorawcap [RawCompress][Number][EnableFaseCapture][EnableYuv]");
                AmbaPrint("             RawCompress:0 : Normal Raw, 1 : Compress Raw.");
                AmbaPrint("             Number: Capture raw number.     ");
                AmbaPrint("             EnableFaseCapture: Enable fast capture or not      ");
                AmbaPrint("             EnableYuv: dump yuv or not");
            } else {

                usb_amage_set_capture_type(4);/**<Set amage capture type (video raw cap)*/
                AppLibVideoEnc_CaptureRaw(atoi(argv[3]),atoi(argv[4]),atoi(argv[5]),atoi(argv[6]));
            }
        } else if (strcmp(argv[2], "convclut") == 0) {
            extern int convert_ARGB_To_AYUV(void);
            convert_ARGB_To_AYUV();
        } else if (strcmp(argv[2], "freq") == 0) {
            AmbaPrint("-------- System Freq ------------------------");
            AmbaPrint("Cortex:\t\t%d", AmbaPLL_GetCortexClk());
            AmbaPrint("DDR:\t\t%d", AmbaPLL_GetDdrClk());
            AmbaPrint("IDSP:\t\t%d", AmbaPLL_GetIdspClk());
            AmbaPrint("Core:\t\t%d", AmbaPLL_GetCoreClk());
            AmbaPrint("Audio:\t\t%d", AmbaPLL_GetAudioClk());
            AmbaPrint("AXI:\t\t%d", AmbaPLL_GetAxiClk());
            AmbaPrint("AHB:\t\t%d", AmbaPLL_GetAhbClk());
            AmbaPrint("APB:\t\t%d", AmbaPLL_GetApbClk());
            AmbaPrint("-------- Peripheral Freq --------------------");
            AmbaPrint("VoutLCD:\t%d", AmbaPLL_GetVoutLcdClk());
            AmbaPrint("VoutTv:\t\t%d", AmbaPLL_GetVoutTvClk());
            AmbaPrint("Sd:\t\t%d", AmbaPLL_GetSdClk());
            AmbaPrint("Sensor:\t\t%d", AmbaPLL_GetSensorClk());
            AmbaPrint("Hdmi:\t\t%d", AmbaPLL_GetHdmiClk());
            AmbaPrint("Uart:\t\t%d", AmbaPLL_GetUartClk());
            AmbaPrint("---------------------------------------------");
        } else if (strcmp(argv[2], "yuvt") == 0) {
            APPLIB_YUV_TASK_CFG_t Cfg= {0};
            AppLibVideoAnal_FrmHdlr_GetDefCfg(&Cfg);
            Cfg.TaskStack = TAppYuvStack;
            AppLibVideoAnal_FrmHdlr_Init();
        } else if (strcmp(argv[2], "t3t") == 0) {
            APPLIB_TRIA_TASK_CFG_t Cfg= {0};
            AppLibVideoAnal_TriAHdlr_GetDefCfg(&Cfg);
            Cfg.TaskStack = TAppTriAStack;
            AppLibVideoAnal_TriAHdlr_Init();
        } else if (strcmp(argv[2], "fcmd") == 0) {
            int tmp = 0;
            APPLIB_FCMD_CFG_t Config = {0};
            APPLIB_FCMD_PAR_t Params = {0};
            tmp = AppLibVideoAnal_FCMD_GetDef_Setting(&Config, &Params);
            AmbaPrintColor(YELLOW, "--------AppLibVideoAnal_FCMD_GetDef_Setting return = %d", tmp);
            tmp = AppLibVideoAnal_FCMD_Init(APPLIB_FRM_HDLR_2ND_YUV, Config, Params);
            AmbaPrintColor(YELLOW, "--------AppLibVideoAnal_FCMD_Init return = %d", tmp);
            tmp = AppLibVideoAnal_FCMD_Enable();
            AmbaPrintColor(YELLOW, "--------AppLibVideoAnal_FCMD_Enable return = %d", tmp);
        } else if (strcmp(argv[2], "llws") == 0) {
            int tmp = 0;
            APPLIB_LLWS_CFG_t Config = {0};
            APPLIB_LLWS_PAR_t Params = {0};
            AppLibVideoAnal_LLWS_GetDef_Setting( &Config, &Params);
            tmp = AppLibVideoAnal_LLWS_Init(Config, Params);
            AmbaPrintColor(YELLOW, "--------AppLibVideoAnal_LLWS_Init return = %d", tmp);
            tmp = AppLibVideoAnal_LLWS_Enable();
            AmbaPrintColor(YELLOW, "--------AppLibVideoAnal_LLWS_Enable return = %d", tmp);
        } else if (strcmp(argv[2], "mdy") == 0) {
            int tmp = 0;
            APPLIB_MD_CFG_t Config = {0};
            AppLibVideoAnal_MD_GetDef_Setting( &Config);
            tmp = AppLibVideoAnal_MD_Init(APPLIB_FRM_HDLR_2ND_YUV, &Config);
            AmbaPrintColor(YELLOW, "--------AppLibVideoAnal_MD_Init return = %d", tmp);
            tmp = AppLibVideoAnal_MD_Enable();
            AmbaPrintColor(YELLOW, "--------AppLibVideoAnal_MD_Enable return = %d", tmp);
        } else if (strcmp(argv[2], "mdae") == 0) {
            int tmp = 0;
            APPLIB_MD_CFG_t Config = {0};
            Config.Method = APPLIB_MD_AE;
            Config.RoiData[0].Location.X = 0;
            Config.RoiData[0].Location.Y = 0;
            Config.RoiData[0].Location.W = 12;
            Config.RoiData[0].Location.H = 8;
            Config.RoiData[0].MDSensitivity = ADAS_SL_MEDIUM;
            tmp = AppLibVideoAnal_MD_Init(0, &Config);
            AmbaPrintColor(YELLOW, "--------AppLibVideoAnal_MD_Init return = %d", tmp);
            tmp = AppLibVideoAnal_MD_Enable();
            AmbaPrintColor(YELLOW, "--------AppLibVideoAnal_MD_Enable return = %d", tmp);
        } else if (strcmp(argv[2], "defcmd") == 0) {
            int tmp = 0;
            tmp = AppLibVideoAnal_FCMD_Disable();
            AmbaPrintColor(YELLOW, "--------AppLibVideoAnal_FCMD_Disable return = %d", tmp);
            tmp = AppLibVideoAnal_FCMD_DeInit();
            AmbaPrintColor(YELLOW, "--------AppLibVideoAnal_FCMD_DeInit return = %d", tmp);
        } else if (strcmp(argv[2], "demd") == 0) {
            int tmp = 0;
            tmp = AppLibVideoAnal_MD_Disable();
            AmbaPrintColor(YELLOW, "--------AppLibVideoAnal_MD_Disable return = %d", tmp);
            tmp = AppLibVideoAnal_MD_DeInit();
            AmbaPrintColor(YELLOW, "--------AppLibVideoAnal_MD_DeInit return = %d", tmp);
        } else if (strcmp(argv[2], "dellws") == 0) {
            int tmp = 0;
            tmp = AppLibVideoAnal_LLWS_Disable();
            AmbaPrintColor(YELLOW, "--------AppLibVideoAnal_LLWS_Disable return = %d", tmp);
            tmp = AppLibVideoAnal_LLWS_DeInit();
            AmbaPrintColor(YELLOW, "--------AppLibVideoAnal_LLWS_DeInit return = %d", tmp);
        } else if (strcmp(argv[2], "brcshow") == 0) {
            AMP_VIDEOENC_ENCODING_INFO_s EncInfo;
            extern AMP_AVENC_HDLR_s *VideoEncPri;
            extern AMP_AVENC_HDLR_s *VideoEncSec;
            if (VideoEncPri) {
                AmpVideoEnc_GetEncodingInfo(VideoEncPri, &EncInfo);
                AmbaPrint("[VideoEnc][Pri]: Total Frames %d AverBitrate %dkbps TotalBytes %lldbytes", EncInfo.TotalFrames,\
                    EncInfo.AverageBitrate, EncInfo.TotalBytes);
            }
            if (VideoEncSec) {
                AmpVideoEnc_GetEncodingInfo(VideoEncSec, &EncInfo);
                AmbaPrint("[VideoEnc][Sec]: Total Frames %d AverBitrate %dkbps TotalBytes %lldbytes", EncInfo.TotalFrames,\
                    EncInfo.AverageBitrate, EncInfo.TotalBytes);
            }
            if ((strcmp(argv[3],"reset") == 0)) {
                if (VideoEncPri) AmpVideoEnc_ResetEncodingInfo(VideoEncPri);
                if (VideoEncSec) AmpVideoEnc_ResetEncodingInfo(VideoEncSec);
            }
        } else if (strcmp(argv[2], "extenc") == 0) {
            if (strcmp(argv[3], "on") == 0) {
                AppLibExtendEnc_SetEnableStatus(1);
            } else if (strcmp(argv[3], "off") == 0){
                AppLibExtendEnc_SetEnableStatus(0);
            } else if (strcmp(argv[3], "set_freq") == 0) {
                AppLibExtendEnc_SetFrequency(atoi(argv[4]));
            } else if (strcmp(argv[3], "sw") == 0) {
                AppLibExtendEnc_GetEnableStatus();
            } else {
                ReturnValue = T_APP_TEST_USAGE;
            }
        } else if (strcmp(argv[2], "texttrack") == 0) {
            if (strcmp(argv[3], "on") == 0) {
                AppLibFormatMuxMp4_TextTrack_SetEnableStatus(1);
            } else if (strcmp(argv[3], "off") == 0){
                AppLibFormatMuxMp4_TextTrack_SetEnableStatus(0);
            }  else if (strcmp(argv[3], "sw") == 0) {
                AppLibFormatMuxMp4_TextTrack_GetEnableStatus();
            } else {
                ReturnValue = T_APP_TEST_USAGE;
            }
        } else if (strcmp(argv[2], "exif_GPS") == 0) {
            UINT8 latitude_ref[2] = {0x00, 0x4E};
            UINT8 latitude[24] = {0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0x01,
                                  0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x01,
                                  0x00, 0x00, 0x0D, 0x04, 0x00, 0x00, 0x00, 0x64};

            APPLIB_GPS_DATA_s GPSData = {0};
            GPSData.GpsTagId = APPLIB_GPS_TAG_LatitudeRef;
            GPSData.GpsDataType = APPLIB_TAG_TYPE_ASCII;
            GPSData.GpsDataLength = 2;
            GPSData.GpsData = latitude_ref;
            AppLibFormatMuxExif_ConfigGpsTag(&GPSData);
            GPSData.GpsTagId = APPLIB_GPS_TAG_Latitude;
            GPSData.GpsDataType = APPLIB_TAG_TYPE_RATIONAL;
            GPSData.GpsDataLength = 24;
            GPSData.GpsData = latitude;
            AppLibFormatMuxExif_ConfigGpsTag(&GPSData);
        } else if (strcmp(argv[2], "wakeup") == 0) {
            int time = 0;
            time = atoi(argv[3]);
            AmbaPrint("set wake up in %d sec",time);
            AmbaPWC_WakeupByAlarm(time);
            AmbaPWC_ForcePowerDownSequence();
        } else if (strcmp(argv[2], "bb") == 0) {
            t_app_test_bb(env, argc, argv);
        }
        #if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
        else if (strcmp(argv[2], "vf") == 0) {
            if (strcmp(argv[3], "on") == 0) {
                AmbaPrintColor(YELLOW, "---------------> t app test vf on");
                AppLibComSvcHcmgr_SendMsg(HMSG_USER_VF_START, 0, 0);
            } else if (strcmp(argv[3], "off") == 0) {
                AmbaPrintColor(YELLOW, "---------------> t app test vf off");
                AppLibComSvcHcmgr_SendMsg(HMSG_USER_VF_STOP, 0, 0);
            } else if (strcmp(argv[3], "sw") == 0) {
                AmbaPrintColor(YELLOW, "---------------> t app test vf sw");
                AppLibComSvcHcmgr_SendMsg(HMSG_USER_VF_SWITCH_TO_RECORD, 0, 0);
            } else {
                ReturnValue = T_APP_TEST_USAGE;
            }
        } else if (strcmp(argv[2], "rtsp") == 0) {
            if (strcmp(argv[3], "on") == 0) {
                UserSetting->VideoPref.StreamType = STREAM_TYPE_RTSP;
                AppPref_Save();
            } else if (strcmp(argv[3], "off") == 0) {
                UserSetting->VideoPref.StreamType = STREAM_TYPE_OFF;
                AppPref_Save();
            } else if (strcmp(argv[3], "show") == 0) {
                AmbaShell_Print(env,"UserSetting->VideoPref.StreamType : %d\n",UserSetting->VideoPref.StreamType);
            } else {
                ReturnValue = T_APP_TEST_USAGE;
            }
        } else if (strcmp(argv[2], "ps") == 0) {
            AppLibComSvcHcmgr_SendMsg(HMSG_USER_PIRNT_REC_CAP_STATE, 0, 0);
        } else if (strcmp(argv[2], "rec") == 0) {
            if (strcmp(argv[3], "av") == 0) {
                AppLibVideoEnc_SetRecMode(REC_MODE_AV);
                AppLibVideoEnc_PipeChange();
                AppLibNetFifo_SetStreamMode(NET_STREAM_MODE_AV);
                AmpNetUtility_luExecNoResponse("killall AmbaRTSPServer");
                AmpNetUtility_luExecNoResponse("/usr/bin/AmbaRTSPServer --en_audio --en_rtcp");
            } else if (strcmp(argv[3], "v") == 0) {
                AppLibVideoEnc_SetRecMode(REC_MODE_VIDEO_ONLY);
                AppLibVideoEnc_PipeChange();
                AppLibNetFifo_SetStreamMode(NET_STREAM_MODE_VIDEO_ONLY);
                AmpNetUtility_luExecNoResponse("killall AmbaRTSPServer");
                AmpNetUtility_luExecNoResponse("/usr/bin/AmbaRTSPServer");
            } else if (strcmp(argv[3], "show") == 0) {
                AmbaShell_Print(env,"rec mode : %s\n",AppLibVideoEnc_GetRecMode() ? "Video only": "AV");
                AmbaShell_Print(env,"net stream mode : %s\n",AppLibNetFifo_GetStreamMode()? "Video only": "AV");
            }
            else {
                ReturnValue = T_APP_TEST_USAGE;
            }
        } else if (strcmp(argv[2], "idr") == 0) {
            t_app_test_idr(env, argc, argv);
        } else if (strcmp(argv[2], "np") == 0) {
            t_app_test_np(env, argc, argv);
        }
        #endif /* CONFIG_APP_CONNECTED_AMBA_LINK */
		else if (strcmp(argv[2], "usbdbg") == 0) {
			t_app_test_usbdbg(env, argc, argv);
		}
        else {
            ReturnValue = T_APP_TEST_USAGE;
        }
    } else {
        ReturnValue = T_APP_TEST_USAGE;
    }

    if (ReturnValue == T_APP_TEST_USAGE) {
        ReturnValue = AmbaShell_Print(env,
        "Usage: t app test <option>\n"
        "Test option list:\n"
        "\t chg_res \n"
        "\t chg_mode \n"
        "\t chg_app \n"
        "\t graph [set_debug_level | set_debug_module] \n"
        "\t save_osd \n"
        "\t rawcap \n"
        "\t convclut \n"
        "\t freq \n"
        "\t extenc [on | off | set_freq | sw] \n"
        "\t texttrack [on | off | sw] \n"
        "\t wakeup [alarm time(sec)] \n"
        "\t bb \n"
        "\t rawcap [cmpr] [filename] \n"
        #if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
        "\t vf [on | off | sw] \n"
        "\t rtsp [on | off | show] \n"
        "\t rec [av | v | show] \n"
        "\t ps \n"
        "\t idr \n"
        "\t np \n"
        #endif /* CONFIG_APP_CONNECTED_AMBA_LINK */
		"\t usbdbg [start | 2uart | 2usb]\n"
        );
    }

    return ReturnValue;
}

static int t_app_mem(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    int ReturnValue = 0;
    extern AMBA_KAL_BYTE_POOL_t G_MMPL;
    extern AMBA_KAL_BYTE_POOL_t G_NC_MMPL;
    AmbaPrint("========== MEMORY POOL STATISTICS =========");
//    AmbaPrint("HEAP    : Total remain   = %10d bytes\n\r"
//        "          Max Free Block = %10d bytes\n\r"
//        "          Fragment       = %10d bytes", G_MMPL.tx_byte_pool_start,  G_MMPL.tx_byte_pool_available, G_MMPL.tx_byte_pool_fragments);
//    AmbaPrint("NON-HEAP: Total Remain   = %10d bytes\n\r"
//        "          Max Free Block = %10d bytes\n\r"
//        "          Fragment       = %10d bytes", G_NC_MMPL.tx_byte_pool_start,  G_NC_MMPL.tx_byte_pool_available, G_NC_MMPL.tx_byte_pool_fragments);

    AmbaPrint("    HEAP    : Available size  = %10d bytes = %4d Mbyte\n\r",  G_MMPL.tx_byte_pool_available, G_MMPL.tx_byte_pool_available>>20);
    AmbaPrint("NON-HEAP    : Available size  = %10d bytes = %4d Mbyte\n\r",  G_NC_MMPL.tx_byte_pool_available, G_NC_MMPL.tx_byte_pool_available>>20);


    return ReturnValue;
}

/**
 *  @brief Test command of demoapp
 *
 *  Test command of demoapp
 *
 *  @param [in] env Amba shell
 *  @param [in] argc Argument count
 *  @param [in] argv Argument vector
 *
 *  @return >=0 success, <0 failure
 */
static int test_app(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    int ReturnValue = 0;

    if (argc > 1) {
        if (strcmp(argv[1], "msg") == 0) {
            ReturnValue = t_app_msg(env, argc, argv);
            goto _done;
        } else if (strcmp(argv[1], "key") == 0) {
            ReturnValue = t_app_key(env, argc, argv);
            goto _done;
        } else if (strcmp(argv[1], "jack") == 0) {
            ReturnValue = t_app_jack(env, argc, argv);
            goto _done;
        } else if (strcmp(argv[1], "test") == 0) {
            ReturnValue = t_app_test(env, argc, argv);
            goto _done;
        } else if (strcmp(argv[1], "mem") == 0) {
            ReturnValue = t_app_mem(env, argc, argv);
            goto _done;
        }
    }

    ReturnValue = AmbaShell_Print(env,
        "App test command list:\n"
        "\t msg \n"
        "\t key \n"
        "\t jack \n"
        "\t test \n"
        "\t mem \n"
    );

    _done:
    return ReturnValue;
}

/**
 *  @brief Add the test command of demoapp
 *
 *  Add the test command of demoapp
 *
 *  @return >=0 success, <0 failure
 */
int App_TestAdd(void)
{
    // hook command
    AmbaTest_RegisterCommand("app", test_app);

    return AMP_OK;
}

