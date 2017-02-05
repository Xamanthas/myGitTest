/**
  * @file src/app/ui/ir/irbutton_op.h
  *
  * Header of IR Button Operation - APP level
  *
  * History:
  *    2013/12/24 - [Martin Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#ifndef APP_IR_BUTTON_OP_H_
#define APP_IR_BUTTON_OP_H_

#include <applib.h>

__BEGIN_C_PROTO__

/*************************************************************************
 * Button OP definitons
 ************************************************************************/
typedef enum _IR_PROTOCOL_ID_e_{
    IR_PROTOCOL_PANASONIC = 0
} IR_PROTOCOL_ID_e;


/*************************************************************************
 * Button OP APIs - Task
 ************************************************************************/
extern int AppIRButtonOp_Init(void);

/*************************************************************************
 * Button OP APIs - Command entry
 ************************************************************************/


__END_C_PROTO__

#endif /* APP_IR_BUTTON_OP_H_ */
