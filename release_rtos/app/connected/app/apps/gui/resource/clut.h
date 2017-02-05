/**
  * @file src/app/apps/gui/resource/connectedcam/clut.h
  *
  *  CLUT Table - 8 bit and include 16 bit
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
#ifndef APP_CLUT_H_
#define APP_CLUT_H_

__BEGIN_C_PROTO__

/*************************************************************************
 * GUI color definitions
 ************************************************************************/
/** Color info */
/**
* 8-bit color info is index of RGB clut
**/
#define COLOR_8BIT_BLACK        (71)
#define COLOR_8BIT_RED            (175)
#define COLOR_8BIT_GREEN        (208)
#define COLOR_8BIT_BLUE            (80)
#define COLOR_8BIT_MAGENTA        (253)
#define COLOR_8BIT_LIGHTGRAY        (21)
#define COLOR_8BIT_DARKGRAY        (41)
#define COLOR_8BIT_YELLOW        (232)
#define COLOR_8BIT_WHITE        (1)
#define COLOR_8BIT_THUMB_BLUE        (103)
#define COLOR_8BIT_THUMB_GRAY        (32)
#define COLOR_8BIT_TEXT_BORDER        (52)
#define COLOR_8BIT_MENU_BG        (34)
#define COLOR_8BIT_WARNING        (180)
#define COLOR_8BIT_CLR            (0) // Specified transparency color!!

extern UINT8 clut[256 * 3];
extern UINT8 blending[256];

__END_C_PROTO__

#endif /* APP_CLUT_H_ */
