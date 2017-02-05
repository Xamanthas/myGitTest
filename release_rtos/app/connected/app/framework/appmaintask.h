 /**
  * @file src/app/framework/appmaintask.h
  *
  * Header of DemoApp entry for testing.
  *
  * History:
  *    2013/08/20 - [Martin Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */


#ifndef APPMAINTASK_H_
#define APPMAINTASK_H_

#include <stdio.h>
#include <string.h>
#include <AmbaDataType.h>
#include <AmbaRTSL_GPIO.h>
#include <AmbaKAL.h>
#include "AmbaUART.h"



#define APP_DSP_CMD_PREPARE_TASK_PRIORITY 6
#define APP_DSP_ARM_CMD_TASK_PRIORITY 9
#define APP_DSP_MSG_DISPATCH_TASK_PRIORITY 11
#define APP_DSP_STILLRAW_MONITOR_TASK_PRIORITY 57
#define APP_FIFO_TASK_PRIORITY 25
#define APP_BOOT_MGR_PRIORITY 70
#define APP_CFS_SCHDLR_PRIORITY 73
#define APP_BUTTON_OP_PRIORITY 122
#define APP_IR_BUTTON_OP_PRIORITY 125
#define APP_DCF_PRIORITY 160
#define APP_AMBA_SHELL_PRIORITY 190
#define APP_AMBA_PRINT_PRIORITY 202
#define APP_AMBA_BOSS_PRIORITY 100
typedef int (*APP_CONSOLE_PUT_CHAR_f)(AMBA_UART_CHANNEL_e UartChanNo, int TxDataSize, char *pTxDataBuf, UINT32 TimeOut);
typedef int (*APP_CONSOLE_GET_CHAR_f)(AMBA_UART_CHANNEL_e UartChanNo, int RxDataSize, char *pRxDataBuf, UINT32 TimeOut);
int AppUserConsole_SetWriteFunc(APP_CONSOLE_PUT_CHAR_f Func);
int AppUserConsole_SetReadFunc(APP_CONSOLE_GET_CHAR_f Func);

int AmbaShellEnable(void);

#endif /* APPMAINTASK_H_ */

