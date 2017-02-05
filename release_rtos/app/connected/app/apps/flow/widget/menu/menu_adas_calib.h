/**
 * system/src/app/apps/flow/widget/menu/menu_adas_calib.h
 *
 * Header of ADAS calibration Menu
 *
 * History:
 *	  2012/05/10 - [James Wang] created file
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __APP_WIDGET_MENU_ADAS_CALIB_H__
#define __APP_WIDGET_MENU_ADAS_CALIB_H__


#include <apps/flow/widget/widgetmgt.h>

__BEGIN_C_PROTO__

/*************************************************************************
 * LDWS calibration Menu definitions
 ************************************************************************/

#define ADAS_CALIB_SELEC_SKY    (0)
#define ADAS_CALIB_SELEC_HOOD (1)

/*************************************************************************
 * ADAS calibration Menu APIs for widget management
 ************************************************************************/
extern WIDGET_ITEM_s* AppAdasCalib_GetWidget(void);

/*************************************************************************
 * Public ADAS calibration Menu Widget APIs
 ************************************************************************/
extern int AppAdasCalib_SetItem(UINT32 tab_id, UINT32 item_id);

__END_C_PROTO__

#endif /* __APP_WIDGET_MENU_ADAS_CALIB_H__ */
