/**
  * @file src/app/apps/gui/resource/connectedcam/clut_ayuv4444.h
  *
  *  CLUT - Color definitions for AYUV 4444 format
  *
  * History:
  *    2013/09/23 - [Martin Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */
#ifndef APP_CLUT_16BIT_H_
#define APP_CLUT_16BIT_H_

__BEGIN_C_PROTO__

/*************************************************************************
 * GUI color definitions
 ************************************************************************/
/** Alpha part */
#define COLOR_16BIT_ALPHA_MASK        (0xF000)
#define COLOR_16BIT_ALPHA_OFFSET    (12)

/** color part */
#define COLOR_16BIT_COLOR_MASK        (0x0FFF)
#define COLOR_16BIT_COLOR_OFFSET    (0)

#define COLOR_16BIT_BLACK    (0xF188)
#define COLOR_16BIT_RED        (0xF56F)
#define COLOR_16BIT_GREEN    (0xF930)
#define COLOR_16BIT_BLUE    (0xF2F6)
#define COLOR_16BIT_MAGENTA    (0xF5BE)
#define COLOR_16BIT_LIGHTGRAY    (0xFC88)
#define COLOR_16BIT_DARKGRAY    (0xF888)
#define COLOR_16BIT_YELLOW    (0xFE1A)
#define COLOR_16BIT_WHITE    (0xFF88)
#define COLOR_16BIT_THUMB_BLUE    (0xF3B5)
#define COLOR_16BIT_THUMB_GRAY    (0xFA88)
#define COLOR_16BIT_TEXT_BORDER    (0xF588)
#define COLOR_16BIT_MENU_BG    (0xF988)
#define COLOR_16BIT_WARNING    (0xF46F)
#define COLOR_16BIT_CLR        (0x0188)

__END_C_PROTO__

#endif /* APP_CLUT_16BIT_H_ */
