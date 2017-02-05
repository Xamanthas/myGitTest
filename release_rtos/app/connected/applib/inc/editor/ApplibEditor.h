/**
 * @file src/app/connected/applib/inc/editor/ApplibEditor.h
 *
 * Header of Editor
 *
 * History:
 *    2014/01/14 - [Martin Lai] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef APPLIB_EDITOR_H_
#define APPLIB_EDITOR_H_
/**
 * @defgroup Editor
 * @brief Editor related function implementation
 *
 * Implementation of
 * Editor related function
 *
 */

/**
* @defgroup ApplibEditor_Function
* @brief Editor related API
*
*
*/

/**
 * @addtogroup ApplibEditor_Function
 * @ingroup Editor
 * @{
 */
#include <applib.h>

__BEGIN_C_PROTO__

/**
 *  Initialize the editor module
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibEditor_Init(void);

/**
 *  To recover the video clip.
 *
 *  @param [in] filename File name
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibEditor_MovieRecover(char *filename);

/**
 *  @brief  To corp clip to new file
 *
 *  To corp clip to new file
 *
 *  @param [in] nTimeStart clip start time (ms)
 *  @param [in] nTimeEnd clip end time (ms)
 *  @param [in] FileNameIn Input File name
 *  @param [in] FileNameOut output File name
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibEditor_MovieCrop2New(UINT32 nTimeStart, UINT32 nTimeEnd, char *FileNameIn, char *FileNameOut);

/**
 *  @brief  To corp clip to new file
 *
 *  To corp clip to new file
 *
 *  @param [in] FileNameIn1 file 1 to merge
 *  @param [in] FileNameIn2 file 2 to merge
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibEditor_MovieMerge(char *FileNameIn1, char *FileNameIn2);

/**
 *  @brief  release the handler after doing editor
 *
 * release the handler after doing editor
 *
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibEditor_EditComplete(void);

/**
 *  To release the resource when the system finish recovering clip.
 *
 *  @return >=0 success, <0 failure
 */
extern int AppLibEditor_MovieRecoverComplete(void);

__END_C_PROTO__
/**
 * @}
 */
#endif /* APPLIB_EDITOR_H_ */
