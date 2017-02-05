/**
 * @file src/app/connected/applib/inc/applib.h
 *
 * Header of App Library
 *
 * History:
 *    2013/07/10 - [Martin Lai] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef APPLIB_H_
#define APPLIB_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mw/mw.h>
#include <util.h>
#include <AmbaDataType.h>
#include "applibhmi.h"
#include <w_char.h>


#include "comsvc/ApplibComSvc_AsyncOp.h"
#include "comsvc/ApplibComSvc_Hcmgr.h"
#include "comsvc/ApplibComSvc_MemMgr.h"
#include "comsvc/ApplibComSvc_Timer.h"
#include "comsvc/ApplibComSvc_Message.h"

#include "graphics/UIObj/ApplibGraphics_UIObj.h"


#include "player/decode_utility/ApplibPlayer_Common.h"
#include "player/ApplibPlayer_Message.h"
#include "player/still_decode/AppLibPlayer_Still_Single.h"
#include "player/video_decode/ApplibPlayer_VideoDec.h"
#include "player/still_decode/ApplibPlayer_Thumb_Basic.h"

#include "editor/ApplibEditor.h"
#include "editor/ApplibEditor_Message.h"

#include "recorder/ApplibRecorder_StillEnc.h"
#include "recorder/ApplibRecorder_VideoEnc.h"
#include "recorder/ApplibRecorder_AudioEnc.h"
#include "recorder/ApplibRecorder_Message.h"
#include "recorder/ApplibRecorder_MemMgr.h"



#include "image/ApplibImage.h"

#include "index/ApplibIndex.h"

#include "format/ApplibFormat.h"
#include "format/ApplibFormat_MuxerManager.h"
#include "format/ApplibFormat_Message.h"

#include "monitor/ApplibMonitor.h"
#include "monitor/ApplibMonitor_Message.h"
//#include "monitor/ApplibMonitor_BrcHandler.h"

#include "monitor/ApplibTimerMonitor.h"

#include "display/ApplibDisplay.h"
#include "display/ApplibCsc.h"

#include "graphics/ApplibGraphics.h"

//#include "dcf/ApplibDcf.h"
#include "dcf/ApplibDcfDateTime.h"
#include "dcf/ApplibDcf_Message.h"

#include "storage/ApplibStorage_Message.h"
#include "storage/ApplibStorage_Card.h"
//#include "storage/ApplibStorage_Dmf.h"
#include "storage/ApplibStorage_DmfDateTime.h"
#include "storage/ApplibStorage_AsyncOp.h"

#include "system/ApplibSys_Gyro.h"
//#include "system/ApplibSys_Lcd.h"
#include "system/ApplibSys_Lens.h"
#include "system/ApplibSys_Sensor.h"
#include "system/ApplibSys_Vin.h"
#include "system/ApplibSys_Vout.h"
#include "system/ApplibSys_Gps.h"

#include "usb/ApplibUsb.h"
#include "usb/ApplibUsbHmi.h"

#include "utility/ApplibUtility_SW_Scalar.h"

#include "va/ApplibVideoAnal_Message.h"
#include "va/ApplibVideoAnal_StmpHdlr.h"
#include "va/ApplibVideoAnal_FrmHdlr.h"
#include "va/ApplibVideoAnal_3aHdlr.h"
#include "va/ApplibVideoAnal_FCMD.h"
#include "va/ApplibVideoAnal_LLWS.h"
#include "va/ApplibVideoAnal_ADAS.h"
#include "va/ApplibVideoAnal_MD.h"

#if defined(CONFIG_APP_CONNECTED_AMBA_LINK)
#include "net/ApplibNet.h"
#include "net/ApplibNet_Base.h"
#include "net/ApplibNet_Control.h"
#include "net/ApplibNet_Fifo.h"
#endif

/*************************************************************************
 * Applib Common Definitions
 ************************************************************************/
#define APP_MAX_DIR_SIZE     (64)
#define APP_MAX_FN_SIZE      (64)

#define APPLIB_ADDFLAGS(x, y)          ((x) |= (y))
#define APPLIB_REMOVEFLAGS(x, y)    ((x) &= (~(y)))
#define APPLIB_CHECKFLAGS(x, y)       ((x) & (y))

#if defined(CONFIG_BUILD_VA_ADAS)
#define APP_ADAS //Enable adas
#endif

#endif /* APPLIB_H_ */
