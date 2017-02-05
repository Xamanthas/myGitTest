/**
 * @file src/app/apps/flow/disp_param/connectedcam/disp_param_thumb.c
 *
 * Thumbnail Scene display parameters
 *
 * History:
 *    2013/12/05 - [Martin Lai] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <apps/flow/disp_param/disp_param_thumb.h>

/*************************************************************************
 * Motion thumbnail mode (fish-eye: APP_THUMB_MOTION) scene settings
 ************************************************************************/
APPLIB_STILL_DISP_DESC_s basicthm_disp_dchan_3x2_h_normal[6] = {
    { {1593,  2541,  2365,  2459}, 128,  1}, // index 0
    { {4406,  2541,  2365,  2459}, 128,  1}, // index 1
    { {7218,  2541,  2365,  2459}, 128,  1}, // index 2
    { {1593,  5333,  2365,  2459}, 128,  1}, // index 3
    { {4406,  5333,  2365,  2459}, 128,  1}, // index 4
    { {7218,  5333,  2365,  2459}, 128,  1}  // index 5
};

APPLIB_STILL_DISP_DESC_s basicthm_disp_dchan_3x2_h_focus[6] = {
    { {1475,  2418,  2601,  2705}, 128,  1}, // index 0
    { {4288,  2418,  2601,  2705}, 128,  1}, // index 1
    { {7100,  2418,  2601,  2705}, 128,  1}, // index 2
    { {1475,  5210,  2601,  2705}, 128,  1}, // index 3
    { {4288,  5210,  2601,  2705}, 128,  1}, // index 4
    { {7100,  5210,  2601,  2705}, 128,  1}  // index 5
};


APPLIB_STILL_DISP_DESC_s basicthm_disp_fchan_3x2_h[6] = {
    { {1000,  2541,  2365,  2459}, 128,  1}, // index 0
    { {3813,  2541,  2365,  2459}, 128,  1}, // index 1
    { {6625,  2541,  2365,  2459}, 128,  1}, // index 2
    { {1000,  5333,  2365,  2459}, 128,  1}, // index 3
    { {3813,  5333,  2365,  2459}, 128,  1}, // index 4
    { {6625,  5333,  2365,  2459}, 128,  1}  // index 5
};


