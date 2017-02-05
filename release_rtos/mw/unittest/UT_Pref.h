 /**
  * @file mw/unittest/UT_Pref.h
  *
  * Unit test preference API header
  *
  * Copyright (C) 2014, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#ifndef AMPUT_PREF_H_
#define AMPUT_PREF_H_

#include "AmpUnitTest.h"
#include <stdio.h>
#include <string.h>

/**
 * Definition of the item value of UT_PREF_SEAMLESS.\n
 * The values must be defined between 0 and 255.\n
 * Avoid the value 255 because it's the default value of pref if NAND is erased.
 */
typedef enum _UT_PREF_SEAMLESS_e_ {
    UT_PREF_SEAMLESS_DISABLE,   ///< Enable seamless
    UT_PREF_SEAMLESS_ENABLE     ///< Disable seamless
} UT_PREF_SEAMLESS_e;

/**
 * Definition of the item ID.
 */
typedef enum _UT_PREF_e_ {
    UT_PREF_SEAMLESS = 511,
    UT_PREF_NUM,
} UT_PREF_e;

/**
 * Initialize pref.
 *
 * @return 0 - OK, others - NG
 */
extern int UT_Pref_Init(void);

/**
 * Get the value of pref with the specified item ID.\n
 * Note that pref is 255 when NAND is erased before boot.\n
 * User have to handle the case that item value is not defined.
 *
 * @param [in] PrefItem     Item ID
 *
 * @return -1 - NG, others - Item value
 */
extern int UT_Pref_Check(UT_PREF_e PrefItem);

/**
 * Set the value of pref with the specified item ID.
 *
 * @param [in] PrefItem     Item ID
 * @param [in] ItemVal      Item value
 *
 * @return 0 - OK, others - NG
 */
extern int UT_Pref_Set(UT_PREF_e PrefItem, UINT8 ItemVal);



#endif /* AMPUT_PREF_H_ */
