/**
 * @file AmpUT_NetCtrl.h
 *
 * Copyright (C) 2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef AMPUT_NETCTRL_H_
#define AMPUT_NETCTRL_H_

#define AMBA_BOSS_MSG_ID   (0x00)
#define AMBA_BOSS_MSG(x)  (((unsigned int)AMBA_BOSS_MSG_ID << 24) + (x))

#define AMBA_GET_SETTING                        AMBA_BOSS_MSG(0x0001)
#define AMBA_SET_SETTING                        AMBA_BOSS_MSG(0x0002)
#define AMBA_GET_ALL_CURRENT_SETTINGS           AMBA_BOSS_MSG(0x0003)
#define AMBA_FORMAT                             AMBA_BOSS_MSG(0x0004)
#define AMBA_GET_SPACE                          AMBA_BOSS_MSG(0x0005)
#define AMBA_GET_NUMB_FILES                     AMBA_BOSS_MSG(0x0006)
#define AMBA_NOTIFICATION                       AMBA_BOSS_MSG(0x0007)
#define AMBA_BURNIN_FW                          AMBA_BOSS_MSG(0x0008)
#define AMBA_GET_SINGLE_SETTING_OPTIONS         AMBA_BOSS_MSG(0x0009)
#define AMBA_PUT_GPS_INFO                       AMBA_BOSS_MSG(0x000a)
#define AMBA_GET_DEVICEINFO                     AMBA_BOSS_MSG(0x000b)
#define AMBA_DIGITAL_ZOOM                       AMBA_BOSS_MSG(0x000e)
#define AMBA_DIGITAL_ZOOM_INFO                  AMBA_BOSS_MSG(0x000f)
#define AMBA_CHANGE_BITRATE                     AMBA_BOSS_MSG(0x0010)
#define AMBA_START_SESSION                      AMBA_BOSS_MSG(0x0101)
#define AMBA_STOP_SESSION                       AMBA_BOSS_MSG(0x0102)
#define AMBA_RESETVF                            AMBA_BOSS_MSG(0x0103)
#define AMBA_STOP_VF                            AMBA_BOSS_MSG(0x0104)
#define AMBA_SET_CLNT_INFO                      AMBA_BOSS_MSG(0x0105)
#define AMBA_RECORD_START                       AMBA_BOSS_MSG(0x0201)
#define AMBA_RECORD_STOP                        AMBA_BOSS_MSG(0x0202)
#define AMBA_GET_RECORD_TIME                    AMBA_BOSS_MSG(0x0203)
#define AMBA_FORCE_SPLIT                        AMBA_BOSS_MSG(0x0204)
#define AMBA_TAKE_PHOTO                         AMBA_BOSS_MSG(0x0301)
#define AMBA_GET_THUMB                          AMBA_BOSS_MSG(0x0401)
#define AMBA_GET_MEDIAINFO                      AMBA_BOSS_MSG(0x0402)
#define AMBA_SET_MEDIA_ATTRIBUTE                AMBA_BOSS_MSG(0x0403)
#define AMBA_DEL_FILE                           AMBA_BOSS_MSG(0x0501)
#define AMBA_LS                                 AMBA_BOSS_MSG(0x0502)
#define AMBA_CD                                 AMBA_BOSS_MSG(0x0503)
#define AMBA_PWD                                AMBA_BOSS_MSG(0x0504)
#define AMBA_GET_FILE                           AMBA_BOSS_MSG(0x0505)
#define AMBA_PUT_FILE                           AMBA_BOSS_MSG(0x0506)
#define AMBA_CANCEL_FILE_XFER                   AMBA_BOSS_MSG(0x0507)
#define AMBA_SET_FILE_ATTRIBUTE                 AMBA_BOSS_MSG(0x0508)
#define AMBA_WIFI_RESTART                       AMBA_BOSS_MSG(0x0601)
#define AMBA_SET_WIFI_SETTING                   AMBA_BOSS_MSG(0x0602)
#define AMBA_GET_WIFI_SETTING                   AMBA_BOSS_MSG(0x0603)
#define AMBA_WIFI_STOP                          AMBA_BOSS_MSG(0x0604)
#define AMBA_WIFI_START                         AMBA_BOSS_MSG(0x0605)
#define AMBA_GET_WIFI_STATUS                    AMBA_BOSS_MSG(0x0606)
#define AMBA_QUERY_SESSION_HOLDER               AMBA_BOSS_MSG(0x0701)

/**
 * RTOS cmd Handler Message Structure Definitions
 */
typedef struct _AMPUT_NETCTRL_CMD_MESSAGE_s_ {
    UINT32 MsgId;                  /**< Message Id.*/
    UINT32 MsgData[2];         /**< Message data.*/
} AMPUT_NETCTRL_CMD_MESSAGE_s;

typedef struct _AMPUT_FILEPATH_s_ {
    char fp_lnx[15];
    char fp[5];
} AMPUT_FILEPATH_s;

#endif /* AMPUT_NETCTRL_H_ */

