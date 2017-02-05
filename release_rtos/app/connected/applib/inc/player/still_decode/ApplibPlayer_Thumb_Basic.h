/**
 * @file src/app/connected/applib/inc/player/still_decode/ApplibPlayer_Thumb_Basic.h
 *
 * The apis provide basic thumbnail for 6 scene on display with focus feature
 *
 * History:
 *    2013/07/29 - [cyweng] Create file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

/**
 * @defgroup ApplibPlayer_Thumb_Basic       ApplibPlayer_Thumb_Basic
 * @brief Decode and display 6 images tiled on screen.
 *
 * Designed for thumbnail mode.                             \n
 * The displayed area of each image is user-defined.        \n
 * There're two sets of user-defined areas to distinguish
 * a focused file form others.
 */

/**
 * @addtogroup ApplibPlayer_Thumb_Basic
 * @ingroup StillDecode
 * @{
 */

#ifndef APPLIB_THUMB_BASIC_H_
#define APPLIB_THUMB_BASIC_H_

#include <applib.h>
#include <player/Decode.h>
#include <player/VideoDec.h>
#include <cfs/AmpCfs.h>
#include <player/decode_utility/ApplibPlayer_Common.h>
/**
 *
 * applib thumb basic table
 *
 */
typedef struct _APPLIB_THUMB_BASIC_TABLE_s_ {
    /** Number of scenes in basic thumbnail mode */
    UINT8 NumScenes;
    /** Display information of each scene */
    APPLIB_STILL_DISP_DESC_s *AreaNormal;
    /** Display information of each scene when it's focused */
    APPLIB_STILL_DISP_DESC_s *AreaFocused;
} APPLIB_THUMB_BASIC_TABLE_s;

/**
 *
 * applib thumb basic file
 *
 */
typedef struct _APPLIB_THUMB_BASIC_FILE_s_ {
    /** filename */
    char Filename[MAX_FILENAME_LENGTH];
    /** if the file is focused */
    UINT8 Focused;
    /** the source of file, indicate to show thumbnail / fullview / screennail */
    UINT32 FileSource;
} APPLIB_THUMB_BASIC_FILE_s;

/**
 * Initialize basic thumbnail, support to display thumbnail on at most 2 window.
 *
 * @return 0 - OK, others - Error
 */
extern int AppLibThmBasic_Init(void);

/**
 * Show black on screen.
 *
 * @return 0 - OK, others - Error
 */
extern int AppLibThmBasic_ClearScreen(void);

/**
 * Show thumbnail on window.
 *
 * @param [in] LocactionInfo    The location table
 * @param [in] NumFiles         Number of file to show
 * @param [in] Files            Array pointer to files to show
 * @param [in] Decoded          The files have been decoded and there's no need to decode them again
 *
 * @return 0 - OK, others - Error
 */
extern int AppLibThmBasic_Show(APPLIB_THUMB_BASIC_TABLE_s *LocactionInfo,
                               UINT8 NumFiles,
                               APPLIB_THUMB_BASIC_FILE_s *Files,
                               UINT8 Decoded);

/**
 * Deinit basic thumbnail.
 *
 * @return 0 - OK, others - Error
 */
extern int AppLibThmBasic_Deinit(void);

#endif /* APPLIB_THUMB_BASIC_H_ */

/**
 * @}
 */     // End of group ApplibPlayer_Thumb_Basic
