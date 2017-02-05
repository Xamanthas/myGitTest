/**
  * @file src/app/apps/gui/resource/connectedcam/clut_ayuv8888.h
  *
  *  CLUT - Color definitions for AYUV 8888 format
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
#ifndef APP_CLUT_AYUV32BIT_H_
#define APP_CLUT_AYUV32BIT_H_

__BEGIN_C_PROTO__

/*************************************************************************
 * GUI color definitions
 ************************************************************************/
/** Alpha part */
#define COLOR_AYUV32BIT_ALPHA_MASK        (0xFF000000)
#define COLOR_AYUV32BIT_ALPHA_OFFSET    (24)

/** color part */
#define COLOR_AYUV32BIT_COLOR_MASK        (0x00FFFFFF)
#define COLOR_AYUV32BIT_COLOR_OFFSET    (0)

#define COLOR_AYUV32BIT_BLACK    (0xFF008080)
#define COLOR_AYUV32BIT_RED        (0xFF4C54F0)
#define COLOR_AYUV32BIT_GREEN    (0xFF952B15)
#define COLOR_AYUV32BIT_BLUE    (0xFF1DEF67)
#define COLOR_AYUV32BIT_MAGENTA    (0xFF4CBDCD)
#define COLOR_AYUV32BIT_LIGHTGRAY    (0xFFC68080)
#define COLOR_AYUV32BIT_DARKGRAY    (0xFF7A8080)
#define COLOR_AYUV32BIT_YELLOW    (0xFFE11094)
#define COLOR_AYUV32BIT_WHITE    (0xFFEB8080)
#define COLOR_AYUV32BIT_THUMB_BLUE    (0xFF5C9F69)
#define COLOR_AYUV32BIT_THUMB_GRAY    (0xFF9D8080)
#define COLOR_AYUV32BIT_TEXT_BORDER    (0xFF538080)
#define COLOR_AYUV32BIT_MENU_BG    (0xFF958080)
#define COLOR_AYUV32BIT_WARNING    (0xFF405BF0)
#define COLOR_AYUV32BIT_CLR        (0x00008080)

__END_C_PROTO__

#endif /* APP_CLUT_AYUV32BIT_H_ */
