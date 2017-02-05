/**
 *  @file AmpUT_NetEventNotifier.c
 *
 * Copyright (C) 2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <net/NetEventNotifier.h>
#include "AmpUnitTest.h"

#define AMPUT_NETEVENTNOTIFIER_RTOSEVENT(x) (0x00000000|x)
#define AMPUT_NETEVENTNOTIFIER_LINUXEVENT(x) (0x10000000|x)
#define AMPUT_NETEVENTNOTIFIER_RTOSERROR(x) (0xF00000000|x)
#define AMPUT_NETEVENTNOTIFIER_LINUXERROR(x) (0xF10000000|x)

/**
 * RTOS event type
 */
typedef enum _AMPUT_NETEVENTNOTIFIER_RTOSEVENT_e_ {
    AMPUT_NETEVENTNOTIFIER_RTOSEVENT_CARD_INSERT = AMPUT_NETEVENTNOTIFIER_RTOSEVENT(1), /**< card inserted at RTOS. */
    AMPUT_NETEVENTNOTIFIER_RTOSEVENT_CARD_REMOVE, /**< card remoted at RTOS. */
    AMPUT_NETEVENTNOTIFIER_RTOSEVENT_CARD_FORMAT, /**< card formated at RTOS. */
    AMPUT_NETEVENTNOTIFIER_RTOSEVENT_FILE_CREATE, /**< new file created at RTOS. */
    AMPUT_NETEVENTNOTIFIER_RTOSEVENT_FILE_DELETE, /**< file deleted at RTOS. */

    AMPUT_NETEVENTNOTIFIER_RTOSEVENT_GENERAL_ERROR = AMPUT_NETEVENTNOTIFIER_RTOSERROR(1), /**< General Error */
} AMPUT_NETEVENTNOTIFIER_RTOSEVENT_e;

/**
 * Linux event type
 */
typedef enum _AMPUT_NETEVENTNOTIFIER_LINUXEVENT_e_ {
    AMPUT_NETEVENTNOTIFIER_LINUXEVENT_BOOTED = AMPUT_NETEVENTNOTIFIER_LINUXEVENT(1), /**< Linux boot done. */
    AMPUT_NETEVENTNOTIFIER_LINUXEVENT_NETWORK_READY, /**< Network ready */
    AMPUT_NETEVENTNOTIFIER_LINUXEVENT_NETWORK_OFF, /**< Network turned off */

    AMPUT_NETEVENTNOTIFIER_LINUXEVENT_GENERAL_ERROR = AMPUT_NETEVENTNOTIFIER_LINUXERROR(1), /**< General Error */
} AMPUT_NETEVENTNOTIFIER_LINUXEVENT_e;


/**
 * callback function to handle NetEventNotifier event.
 */
static int AmpUT_NetEventNotifier_EventCB(void *hdlr, UINT32 event, void* info)
{
    int rval = 0;
    AMP_NETEVENTNOTIFIER_DATABLK_s *event_param;

    switch(event) {
    case AMPUT_NETEVENTNOTIFIER_LINUXEVENT_BOOTED:
        AmbaPrint("Got Linux Booted!");
        break;
    case AMPUT_NETEVENTNOTIFIER_LINUXEVENT_NETWORK_READY:
        AmbaPrint("Got Linux Network Ready!");
        event_param = (AMP_NETEVENTNOTIFIER_DATABLK_s *)info;
        if(event_param->param_len != sizeof(unsigned int)) {
            AmbaPrint("Invalid event block for Network Ready!");
            rval = -1;
            break;
        } else {
            int *i_ptr;

            i_ptr = (int *)(event_param->param);
            AmbaPrint("Linux Network Ready type is 0x%08x!",*i_ptr);

            switch((*i_ptr)) {
            case 0:
                AmbaPrint("Wifi is configured as AP mode");
                break;
            case 1:
                AmbaPrint("Wifi is configured as STA mode");
                break;
            case 2:
                AmbaPrint("Wifi is configured as P2P mode");
                break;
            default:
                AmbaPrint("Wifi is configured as unknown mode 0x%08x",(*i_ptr));
                break;
            }
        }
        break;
    case AMPUT_NETEVENTNOTIFIER_LINUXEVENT_NETWORK_OFF:
        AmbaPrint("Got Linux Network turned-off!");
        break;
    default:
        AmbaPrint("%s: Unknow Event 0x%08x",__FUNCTION__, event);
        break;
    }

    return rval;
}

/**
 * Unitest initiate function.
 */
int AmpUT_NetEventNotifier_init(void)
{
    int rval = 0;
    AMP_NETEVENTNOTIFIER_INIT_CFG_s init_cfg;

    rval = AmpNetEventNotifier_GetInitDefaultCfg(&init_cfg);
    if (rval < 0) {
        AmbaPrint("%s: fail to GetInitDefaultCfg. %d",__FUNCTION__,rval);
        return -1;
    }

    init_cfg.cbEvent = AmpUT_NetEventNotifier_EventCB;

    rval = AmpNetEventNotifier_init(&init_cfg);
    if (rval < 0) {
        AmbaPrint("%s: fail to AmpNetEventNotifier_init. %d",__FUNCTION__,rval);
        return -1;
    }

    return 0;
}

/**
 * Send Notify function
 */
static int AmpUT_NetEventNotifier_SendNotify(int argc, char **argv)
{
    UINT32 event;
    UINT8 param[512] = {0};
    UINT32 param_len;
    int rval,i;

    if (argc < 3) {
        AmbaPrint("usage: %s send <EVENT HEX> <PARAM>",argv[0]);
        return -1;
    }

    event = strtoul(argv[2], NULL, 0);
    if (argc == 3) { //no param
        rval = AmpNetEventNotifier_SendNotify(event, NULL, 0);
    } else {
        for (i=3;i<argc;i++) {
            strcat((char *)param, argv[i]);
            strcat((char *)param, " ");
        }
        param_len = strlen((char *)param);
        param[param_len-1] = '\0';
        rval = AmpNetEventNotifier_SendNotify(event, param, param_len);
    }

    return rval;
}

static void usage_Notify(char *p_name)
{
    AmbaPrint("usage: %s notify <EVENT TYPE> <PARAM>",p_name);
    AmbaPrint("<EVENT TYPE>:");
    AmbaPrint("\t \"card_ins\": notify card inserted. PARAM as drive name, such as A, C, or D.");
    AmbaPrint("\t \"card_rm\" : notify card removed. PARAM as drive name, such as A, C, or D.");
    AmbaPrint("\t \"card_fmt\": notify card formated. PARAM as drive name, such as A, C, or D.");
    AmbaPrint("\t \"file_crt\": notify file created. PARAM as filename with full path.");
    AmbaPrint("\t \"file_del\": notify file deleted. PARAM as filename with full path.");
}

/**
 * Send pre-definded Event
 */
static int AmpUT_NetEventNotifier_Notify(int argc, char **argv)
{
    UINT32 event;
    int rval;
    UINT8 param[512] = {0};
    UINT32 param_len;

    if (argc != 4) {
        usage_Notify(argv[0]);
        return -1;
    }

    if (strcmp(argv[2], "card_ins") == 0) {
        event = AMPUT_NETEVENTNOTIFIER_RTOSEVENT_CARD_INSERT;
    } else if (strcmp(argv[2], "card_rm") == 0) {
        event = AMPUT_NETEVENTNOTIFIER_RTOSEVENT_CARD_REMOVE;
    } else if (strcmp(argv[2], "card_fmt") == 0) {
        event = AMPUT_NETEVENTNOTIFIER_RTOSEVENT_CARD_FORMAT;
    } else if (strcmp(argv[2], "file_crt") == 0) {
        event = AMPUT_NETEVENTNOTIFIER_RTOSEVENT_FILE_CREATE;
    } else if (strcmp(argv[2], "file_del") == 0) {
        event = AMPUT_NETEVENTNOTIFIER_RTOSEVENT_FILE_DELETE;
    } else {
        usage_Notify(argv[0]);
        return -1;
    }

    snprintf((char*)param,512,"%s",argv[3]);
    param_len = strlen((char*)param);
    rval = AmpNetEventNotifier_SendNotify(event, param, param_len);

    return rval;
}

static void AmpUT_NetEventNotifier_usage(char *pName)
{
    AmbaPrint("usage: %s <CMD>",pName);
    AmbaPrint("\t init");
    AmbaPrint("\t send <EVENT HEX> [PARAM]");
    AmbaPrint("\t notify <EVENT TYPE> <PARAM>");
    AmbaPrint("\t end\n\n");
}

/**
 * NetEventNotifier test function.
 */
static int AmpUT_NetEventNotifier_Test(struct _AMBA_SHELL_ENV_s_ *env, int argc, char **argv)
{
    int rval = -1;

    if (strcmp(argv[1], "init") == 0) { //initialization
        rval = AmpUT_NetEventNotifier_init();
    } else if (strcmp(argv[1], "send") == 0) {
        rval = AmpUT_NetEventNotifier_SendNotify(argc, argv);
    } else if (strcmp(argv[1], "notify") == 0) {
        rval = AmpUT_NetEventNotifier_Notify(argc, argv);
    } else if (strcmp(argv[1], "end") == 0) {
        AmbaPrint("%s: Not support yet!", argv[1]);
    } else { //show usage
        AmpUT_NetEventNotifier_usage(argv[0]);
    }

    return rval;
}

/**
 * NetEventNotifier test command hooking function.
 */
int AmpUT_NetEventNotifier_TestAdd(void)
{
    // hook command
    AmbaTest_RegisterCommand("netevent", AmpUT_NetEventNotifier_Test);

    return AMP_OK;
}

