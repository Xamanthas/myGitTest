 /**
  * @file mw/unittest/AmpUT_Display.h
  *
  * Display Unit test header
  *
  * History:
  *    2015/01/06 - [Eric Yen] created file
  *
  * Copyright (C) 2014, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#ifndef AMPUT_DISPLAY_H_
#define AMPUT_DISPLAY_H_

#include <display/Osd.h>
#include <display/Display.h>
#include "AmpUnitTest.h"
#include <stdio.h>
#include <string.h>

/**
 * AmpUT_Display_Init
 * @return 0 - OK, others - AMP_ER_CODE_e
 * @see AMP_ER_CODE_e
 */
extern int AmpUT_Display_Init(void);

/**
 * AmpUT_Display_Start
 * @param[in] Channel - The channel that osd show on
 * @return 0 - OK, others - AMP_ER_CODE_e
 * @see AMP_ER_CODE_e
 */
extern int AmpUT_Display_Start(UINT8 Channel);

/**
 * AmpUT_Display_Stop
 * @param[in] Channel - The channel that osd show on
 * @return 0 - OK, others - AMP_ER_CODE_e
 * @see AMP_ER_CODE_e
 */
extern int AmpUT_Display_Stop(UINT8 Channel);

extern int AmpUT_Display_ConfigGet(UINT8 Channel, AMP_DISP_DEV_CFG_s *pOutDevCfg);
extern int AmpUT_Display_ConfigSet(UINT8 Channel, AMP_DISP_DEV_CFG_s *pOutDevCfg);

/**
 * Create LCD/TV window, if it has been created, just update cfg as new window
 * UT user need to update(call AmpUT_Display_SetWindowCfg) their window setting
 * every time before display start.
 * @param[in] Channel - The channel that osd show on
 * @param[in] Source
 * @param[in] pCropArea
 * @param[in] pTargetAreaOnPlane
 * @return 0 - OK, others - AMP_ER_CODE_e
 * @see AMP_ER_CODE_e
 */
extern int AmpUT_Display_Window_Create(UINT8 Channel,
                                       AMP_DISP_WINDOW_CFG_s *pWindowCfg);

/**
 *
 * @param[in] Channel - The channel that osd show on
 * @return 0 - OK, others - AMP_ER_CODE_e
 * @see AMP_ER_CODE_e
 */
extern int AmpUT_Display_Window_Delete(UINT8 Channel);

extern int AmpUT_Display_Window_Update(UINT8 Channel,
                                AMP_DISP_WINDOW_SRC_e Source,
                                AMP_AREA_s *pCropArea,
                                AMP_AREA_s *pTargetAreaOnPlane);

extern int AmpUT_Display_Act_Window(UINT8 Channel);
extern int AmpUT_Display_DeAct_Window(UINT8 Channel);
extern int AmpUT_Display_GetInfo(UINT8 Channel, AMP_DISP_INFO_s *pOutDispInfo);
extern int AmpUT_Display_GetWindowCfg(UINT8 Channel, AMP_DISP_WINDOW_CFG_s *pOutWindowCfg);
extern int AmpUT_Display_SetWindowCfg(UINT8 Channel, AMP_DISP_WINDOW_CFG_s *pWindowCfg);
extern int AmpUT_Display_Update(UINT8 Channel);
extern int AmpUT_Display_CvbsCtrl(UINT8 Enable);
extern int AmpUT_Display_DispHdlrGet(UINT8 Channel, AMP_DISP_HDLR_s **hdlr);

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
extern int AmpUT_Display_GetDisplayHandler(UINT8 Channel, AMP_DISP_HDLR_s **pOutDispHdlr);





#endif /* AMPUT_DISPLAY_H_ */
