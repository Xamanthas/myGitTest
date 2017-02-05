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
#ifndef APP_CLUT_ARGB32BIT_H_
#define APP_CLUT_ARGB32BIT_H_

__BEGIN_C_PROTO__

/*************************************************************************
 * GUI color definitions
 ************************************************************************/
/** Alpha part */
#define COLOR_ARGB32BIT_ALPHA_MASK        (0xFF000000)
#define COLOR_ARGB32BIT_ALPHA_OFFSET    (24)

/** color part */
#define COLOR_ARGB32BIT_COLOR_MASK        (0x00FFFFFF)
#define COLOR_ARGB32BIT_COLOR_OFFSET    (0)

#define COLOR_ARGB32BIT_BLACK    (0xFF000000)
#define COLOR_ARGB32BIT_RED        (0xFFFF0000)
#define COLOR_ARGB32BIT_GREEN    (0xFF00FF00)
#define COLOR_ARGB32BIT_BLUE    (0xFF0000FF)
#define COLOR_ARGB32BIT_MAGENTA    (0xFFBA00BA)
#define COLOR_ARGB32BIT_LIGHTGRAY    (0xFFC6C6C6)
#define COLOR_ARGB32BIT_DARKGRAY    (0xFF7A7A7A)
#define COLOR_ARGB32BIT_YELLOW    (0xFFFFFF00)
#define COLOR_ARGB32BIT_WHITE    (0xFFFFFFFF)
#define COLOR_ARGB32BIT_THUMB_BLUE    (0xFF3D6294)
#define COLOR_ARGB32BIT_THUMB_GRAY    (0xFF9D9D9D)
#define COLOR_ARGB32BIT_TEXT_BORDER    (0xFF535353)
#define COLOR_ARGB32BIT_MENU_BG    (0xFF959595)
#define COLOR_ARGB32BIT_WARNING    (0xFFD70000)
#define COLOR_ARGB32BIT_CLR        (0x00000000)

__END_C_PROTO__

#endif /* APP_CLUT_ARGB32BIT_H_ */
