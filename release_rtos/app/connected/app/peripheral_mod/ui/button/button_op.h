/**
  * @file src/app/peripheral_mod/ui/button/button_op.h
  *
  * Header of Button Operation - APP level
  *
  * History:
  *    2013/09/09 - [Martin Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#ifndef APP_BUTTON_OP_H_
#define APP_BUTTON_OP_H_

#include <applib.h>

__BEGIN_C_PROTO__

/*************************************************************************
 * Button OP definitons
 ************************************************************************/


/*************************************************************************
 * Button OP APIs - Command entry
 ************************************************************************/
extern int AppButtonOp_Init(void);
extern int AppButtonOp_UpdateStatus(UINT32 buttonId, UINT32 event);

__END_C_PROTO__

#endif /* APP_BUTTON_OP_H_ */
