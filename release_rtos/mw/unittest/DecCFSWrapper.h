/**
  * @file DecCFSWrapper.h
  *
  * Implementation of video player module in application Library
  *
  * History:
  *    2014/10/6 - [cyweng] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */


#ifndef DECCFSWRAPPER_H_
#define DECCFSWRAPPER_H_

#include <cfs/AmpCfs.h>

extern AMP_CFS_FILE_s* AmpDecCFS_fopen(const char *pFileName, const char *pMode);

#endif /* DECCFSWRAPPER_H_ */
