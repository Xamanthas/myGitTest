/**
  * @file src/app/apps/apps.c
  *
  * Implementation of Applications
  *
  * History:
  *    2013/07/09 - [Martin Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */
#include <apps/apps.h>

int APP_MAIN = -1;
int APP_REC_DSC = -1;
int APP_REC_DV = -1;
int APP_REC_CONNECTED_CAM = -1;
int APP_REC_SPORT_DV = -1;
int APP_REC_SOUND = -1;
int APP_REC_CAR_VIDEO = -1;
int APP_PB_MULTI = -1;
int APP_PB_PHOTO = -1;
int APP_PB_VIDEO = -1;
int APP_PB_SOUND = -1;
int APP_PB_TRANSCODER = -1;
int APP_EDTR_VIDEO = -1;
int APP_EDTR_VIDEO_MERGE = -1;
int APP_THUMB_PICKUP = -1;
int APP_THUMB_MOTION = -1;
int APP_USB_MSC = -1;
int APP_USB_PCCAM = -1;
int APP_USB_AMAGE = -1;
int APP_USB_PICTB = -1;
int APP_USB_PICTB_PRINT = -1;
int APP_USB_STILLIMAGE = -1;
int APP_MISC = -1;
int APP_MISC_CALIBUPDATE = -1;
int APP_MISC_FWUPDATE = -1;
int APP_MISC_MVRECOVER = -1;
int APP_MISC_FORMATCARD = -1;
int APP_MISC_DEFSETTING = -1;
int APP_MISC_POWEROFF = -1;
int APP_MISC_POWERSAVING = -1;
int APP_MISC_WIFISTATUS = -1;
int APP_MISC_WEBUPLOAD = -1;
int APP_TEST_DDR_TUNER = -1;
int APP_TEST_CALIB = -1;
int APP_MISC_QRCONFIG = -1;
int APP_HDMI_TEST  = -1;

/*************************************************************************
 * Main Application
 ************************************************************************/
static int app_main_start(void);
static int app_main_stop(void);
static int app_main_on_message(UINT32 msg, UINT32 param1, UINT32 param2);

APP_APP_s app_main = {
    0,  // Id
    0,  // Tier
    0,  // Parent
    0,  // Previous
    0,  // Child
    APP_AFLAGS_INIT | APP_AFLAGS_START | APP_AFLAGS_READY,  // GFlags
    0,  // Flags
    app_main_start,
    app_main_stop,
    app_main_on_message
};

static int app_main_on_message(UINT32 msg, UINT32 param1, UINT32 param2)
{
    return -1;
}

static int app_main_start(void)
{
    if (!APP_CHECKFLAGS(app_main.GFlags, APP_AFLAGS_INIT)) {
        APP_ADDFLAGS(app_main.GFlags, APP_AFLAGS_INIT);
    }
    if (!APP_CHECKFLAGS(app_main.GFlags, APP_AFLAGS_START)) {
        APP_ADDFLAGS(app_main.GFlags, APP_AFLAGS_START);
    }
    if (!APP_CHECKFLAGS(app_main.GFlags, APP_AFLAGS_READY)) {
        APP_ADDFLAGS(app_main.GFlags, APP_AFLAGS_READY);
    }
    return 0;
}

static int app_main_stop(void)
{
    return 0;
}
