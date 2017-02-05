/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: Amba_AeAwbAdj_Control.h
 *
 *  @Copyright      :: Copyright (C) 2013 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Ae/Awb/Adj Algo. control.
 *
 *  @History        ::
 *      Date        Name             Comments
 *      05/06/2006  Jyh-Jiun Li      Created
 *
 \*-------------------------------------------------------------------------------------------------------------------*/
#ifndef AMBA_AEAWBADJ_CONTROL
#define AMBA_AEAWBADJ_CONTROL

/* function prototype */
void Amba_AeAwbAdj_Init(UINT32 chNo,UINT8 initFlg, AMBA_KAL_BYTE_POOL_t *pMMPL);
void Amba_AeAwbAdj_Control(UINT32 chNo);
void Amba_Ae_Ctrl(UINT32 chNo);
void Amba_Awb_Ctrl(UINT32 chNo);
void Amba_Adj_Ctrl(UINT32 chNo);

#endif

