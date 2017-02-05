/**
 *  @file PrecMux.h
 *
 *  PRE-RECORD mux format header
 *
 *  **History**
 *      |Date       |Name        |Comments       |
 *      |-----------|------------|---------------|
 *      |2013/07/05 |clchan      |Created        |
 *
 *  @copyright 2013 Ambarella Corporation. All rights reserved.
 *                 No part of this file may be reproduced, stored in a retrieval system,
 *                 or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                 recording, or otherwise, without the prior consent of Ambarella Corporation.
 */
#ifndef __PREC_MUX_H__
#define __PREC_MUX_H__

#include <format/Format.h>

/**
 * @defgroup PREC
 * @ingroup Format
 * @brief pre-record mux flow implementation
 *
 * The pre-record mux manager/handler includes the following functions
 * 1. Initiate pre-record mux manager function.
 * 2. Create pre-record mux handler function.
 * 3. Delete pre-record mux handler function.
 *
 */

/**
 * @addtogroup PREC
 * @{
 */

/**
 * Prec muxer initial configure.
 */
typedef struct {
    UINT8 *Buffer;          /**< The buffer of the Prec muxer manager. */
    UINT32 BufferSize;      /**< The buffer size of the Prec muxer manager. */
    UINT8 MaxHdlr;          /**< The max number of the handler of the Prec muxer manager. */
} PREC_MUX_INIT_CFG_s;

/**
 * Prec muxer configure.
 */
typedef struct {
    UINT32 Length;          /**< The length(ms) of the pre-record muxer. */
} PREC_MUX_CFG_s;

/**
 * Get the default configure of pre-record muxer manager.
 * @param [out] config the buffer to get default configure.
 * @return 0 - OK, others - APPLIB_ER_CODE_e
 * @see APPLIB_ER_CODE_e
 */
extern int PrecMux_GetInitDefaultCfg(PREC_MUX_INIT_CFG_s *config);

/**
 * Get the required buffer size
 * @param [in] maxHdlr the number of pre-record muxers
 * @return the required buffer size
 */
extern UINT32 PrecMux_GetRequiredBufferSize(UINT8 maxHdlr);

/**
 * Initiate the core of pre-record muxer manager.
 * @param [in] config the configure for initiating the prec muxer manager.
 * @return 0 - OK, others - APPLIB_ER_CODE_e
 * @see APPLIB_ER_CODE_e
 */
extern int PrecMux_Init(PREC_MUX_INIT_CFG_s *config);

/**
 * Get the default pre-record muxer configure.
 * @param [out] config the buffer to get default configure.
 * @return 0 - OK, others - APPLIB_ER_CODE_e
 * @see APPLIB_ER_CODE_e
 */
extern int PrecMux_GetDefaultCfg(PREC_MUX_CFG_s *config);

/**
 * Create a pre-record mux handler.
 * @param [in] config the configure for creating the prec muxer handler.
 * @param [out] hdlr the double pointer to get the handler.
 * @return 0 - OK, others - APPLIB_ER_CODE_e
 * @see APPLIB_ER_CODE_e
 */
extern int PrecMux_Create(PREC_MUX_CFG_s *config, AMP_MUX_FORMAT_HDLR_s **hdlr);

/**
 * Delete a pre-record muxer.
 * @param [in] hdlr the muxer handler that want to delete.
 * @return 0 - OK, others - APPLIB_ER_CODE_e
 * @see APPLIB_ER_CODE_e
 */
extern int PrecMux_Delete(AMP_MUX_FORMAT_HDLR_s *hdlr);

/**
 * Set the pre-record length
 * @param [in] hdlr the muxer handler
 * @param [in] length the pre-record length (ms)
 * @return 0 - OK, others - APPLIB_ER_CODE_e
 * @see APPLIB_ER_CODE_e
 */
extern int PrecMux_SetPrecLength(AMP_MUX_FORMAT_HDLR_s *hdlr, UINT32 length);

/**
 * @}
 */

#endif

