/**
 * @file src/app/connected/applib/inc/applibhmi.h
 *
 * User-defined HMI messages
 *
 * History:
 *    2013/07/05 - [Martin Lai] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include "AmbaCardManager.h"

#ifndef APP_APPLIBHMI_H_
#define APP_APPLIBHMI_H_

/**
*  Defines for Module ID
*/
#define MDL_APPLIB_SYSTEM_ID    0x01
#define MDL_APPLIB_RECORDER_ID    0x02
#define MDL_APPLIB_PLAYER_ID    0x03
#define MDL_APPLIB_AUDIO_ID    0x04
#define MDL_APPLIB_MONITOR_ID    0x05
#define MDL_APPLIB_DCF_ID        0x06
#define MDL_APPLIB_STORAGE_ID    0x07
#define MDL_APPLIB_STREAM_ID    0x08
#define MDL_APPLIB_FORMAT_ID    0x09
#define MDL_APPLIB_EDITOR_ID    0x0A
#define MDL_APPLIB_DISPLAY_ID    0x0B
#define MDL_APPLIB_GRAPHICS_ID    0x0C
#define MDL_APPLIB_IMAGE_ID    0x0D
#define MDL_APPLIB_3A_ID        0x0E
#define MDL_APPLIB_TUNE_ID        0x0F
#define MDL_APPLIB_CALIB_ID    0x11
#define MDL_APPLIB_USB_ID        0x12
#define MDL_APPLIB_COMSVC_ID    0x13
#define MDL_APPLIB_UTILITY_ID    0x14
#define MDL_APPLIB_THIRD_ID    0x15
#define MDL_APPLIB_VA_ID       0x16

/**
* MDL_APP_KEY_ID 0x1B is defined for the message group of netwok control.
**/
#define MDL_APP_NET_ID          0x1B


/**
* MDL_APP_KEY_ID 0x1B is defined for the message group of user
* operation input, such as buttons, IR remote, etc.
**/
#define MDL_APP_KEY_ID          0x1C

/**
* MDL_APP_JACK_ID 0x1C is defined for the message group of
* device peripheral jack, such as vin/vout jack, ain/aout jack,
* card jack, etc.
**/
#define MDL_APP_JACK_ID         0x1D


/**
* MDL_APP_FLOW_ID 0x1E is defined for the message group of
* application flows and test flows, such as app state messages,
* app command messages, and app test messages
**/
#define MDL_APP_FLOW_ID         0x1E

/**
*  Defines for Message Type
*/
#define MSG_TYPE_HMI            0x00
#define MSG_TYPE_CMD            0x01
#define MSG_TYPE_PARAM          0x02
#define MSG_TYPE_ERROR          0x03

#define MSG_ID(mdl_id, msg_type, msg_par)       (((mdl_id & 0x0000001F) << 27) | ((msg_type & 0x00000007) << 24) | (msg_par & 0x00FFFFFF))
#define MSG_MDL_ID(id)          ((id & 0xF8000000)>>27)
#define MSG_TYPE(id)            ((id & 0x07000000)>>24)

#define MSG_NULL_ID             0x00000000

#endif /* APP_APPLIBHMI_H_ */
