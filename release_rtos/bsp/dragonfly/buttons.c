/**
  * @file bsp/a9/a9evk_ginkgo/buttons.c
  *
  * The button setting of evk
  *
  * History:
  *    2013/10/17 - [Martin Lai] created file
  *
  * Copyright (C) 2013, Ambarella, Inc.
  *
  * All rights reserved. No Part of this file may be reproduced, stored
  * in a retrieval system, or transmitted, in any form, or by any means,
  * electronic, mechanical, photocopying, recording, or otherwise,
  * without the prior consent of Ambarella, Inc.
  */

#include <stdio.h>
#include <string.h>
#include "AmbaDataType.h"
#include "AmbaKAL.h"

#include "AmbaRTSL_GPIO.h"
#include "AmbaGPIO.h"
#include "AmbaADC.h"
#include "AmbaIR.h"

#include "AmbaPrintk.h"

#include "buttons.h"

//#define ENABLE_BUTTON_DEBUG_MSG
#ifdef ENABLE_BUTTON_DEBUG_MSG
#define DEBUG_MSG AmbaPrint
#else
#define DEBUG_MSG(...)
#endif


/***************************************************************
* ADC button releated functions
***************************************************************/
typedef struct _ADC_BUTTON_s {
    INT32   ButtonId; /**< Button ID */
    UINT32   LowVolt; /**< (Low range)  voltage */
    UINT32  HighVolt; /**< (High range) voltage */
} ADC_BUTTON_s;

#define CHAN_NORM_POS         0xcafecafe
#define DEBOUNCE_COUNT       3
#define ADC_BT_CH_NUM         2

static ADC_BUTTON_s GAdcButton1[] = {
    { UP_BUTTON,          0,  300 }, /**< UP */
    { DOWN_BUTTON,       500,  800 }, /**< DOWN_ */
    { LEFT_BUTTON,         1080, 1600 }, /**< LEFT */
    { RIGHT_BUTTON,      1800, 2400 }, /**< RIGHT */
    { RECORD_BUTTON,   2800, 3100 }, /**< RECORD */
    { CHAN_NORM_POS,       0,      0 }
};

static ADC_BUTTON_s GAdcButton2[] = {
    { MODE_BUTTON,         1080, 1600 }, /**< MODE */
    { DEL_BUTTON,      2000, 2600 }, /**< DEL */
    { CHAN_NORM_POS,       0,      0 }
};

extern int AppButtonOp_UpdateStatus(UINT32 buttonId, UINT32 event) __attribute__((weak));
static ADC_BUTTON_s *GAdcButtons[ADC_BT_CH_NUM] = {
    GAdcButton1,
    GAdcButton2
};

static INT32 DebounceCounts[ADC_BT_CH_NUM] = {0, 0};
static INT32 DebounceKeys[ADC_BT_CH_NUM] = {-1, -1};
static INT32 LastChanBIds[ADC_BT_CH_NUM] = {-1, -1};

/**
 *  @brief Handler for for watching ADC voltage level changes.
 *
 *  Handler for for watching ADC voltage level changes.
 *
 *  @param [in] tid Timer id
 *
 */
static void A12RefButton_AdcScanHandler(UINT32 tid)
{
    INT32 i = 0, j = 0;
    UINT32 AdcValues[ADC_BT_CH_NUM] = {0, 0};
    INT32 CurrentButtonIds[ADC_BT_CH_NUM] = {-1, -1};

    //AmbaPrint("[BSP-button] <A12RefButton_AdcScanHandler> Start");
    /*Get the ADC data.*/
    AmbaADC_DataCollection();

    AdcValues[0] = AmbaAdcData[1];
    AdcValues[1] = AmbaAdcData[2];

    for (i=0; i<ADC_BT_CH_NUM; i++) {
        for (j=0; GAdcButtons[i][j].ButtonId >= 0; j++) {
            //AmbaPrint("[BSP-button] Adc curr ch[%d] = %d, key[%d]  LowVolt = %d  HighVolt =%d", i, AdcValues[i], j, GAdcButtons[i][j].LowVolt,GAdcButtons[i][j].HighVolt );
            if ((AdcValues[i] >= GAdcButtons[i][j].LowVolt) && (AdcValues[i] <= GAdcButtons[i][j].HighVolt)) {
                CurrentButtonIds[i] = GAdcButtons[i][j].ButtonId;
                //DEBUG_MSG("[BSP-button] key : %x changed", GAdcButtons[i][j].ButtonId);
                break;
            }
        }
        if (GAdcButtons[i][j].ButtonId == CHAN_NORM_POS) {
            CurrentButtonIds[i] = CHAN_NORM_POS;
        }
        if (CurrentButtonIds[i] == -1) {
            continue;
        }
        if (DebounceKeys[i] != CurrentButtonIds[i]) {
            DebounceKeys[i] = CurrentButtonIds[i];
            DebounceCounts[i] = 0;
            continue;
        } else {
            DebounceCounts[i]++;
        }
        if (DebounceCounts[i] == DEBOUNCE_COUNT) {
            if (LastChanBIds[i] != -1) {
                AppButtonOp_UpdateStatus(LastChanBIds[i], UI_BUTTON_UP_EVENT);
                DEBUG_MSG("[BSP-button] key : %x up", LastChanBIds[i]);
                LastChanBIds[i] = -1;
            }
            if (DebounceKeys[i] != CHAN_NORM_POS) {
                LastChanBIds[i] = DebounceKeys[i];
                AppButtonOp_UpdateStatus(LastChanBIds[i], UI_BUTTON_DOWN_EVENT);
                DEBUG_MSG("[BSP-button] key : %x down", LastChanBIds[i]);
            }
        }
    }
}

static AMBA_KAL_TIMER_t ButtonTimer = {0};
static UINT32 ButtontTimerId = 15;

/**
 *  @brief Initialize the A12 adc button driver.
 *
 *  Initialize the A12 adc button driver.
 *
 */
static void A12RefButton_AdcInit(void)
{
    int ReturnValue = -1;

    /* Initialize the ADC device. */
    //AmbaADC_Init();
    //AmbaADC_Start();

    /* Register a system timer for button. */
    ReturnValue = AmbaKAL_TimerCreate(&ButtonTimer, AMBA_KAL_AUTO_START,
                                &A12RefButton_AdcScanHandler, ButtontTimerId,
                                15, 15);
    if (ReturnValue < 0) {
        AmbaPrint("[BSP-button] Register timer fail.");
        K_ASSERT(0);
    }
}


/***************************************************************
* GPIO button releated functions
***************************************************************/


/**
 *  @brief Initialize the Camry GPIO button driver.
 *
 *  Initialize the Camry GPIO button driver.
 *
 */
static void A12RefButton_GpioSetScanHandler(AMBA_GPIO_PIN_ID_e gpioPinID)
{
    if (gpioPinID == GPIO_KEY_SET) {
        AMBA_GPIO_PIN_INFO_s pinInfo;
        AMBA_GPIO_PIN_LEVEL_e level;

        AmbaGPIO_GetPinInfo(gpioPinID, &pinInfo);
        level = pinInfo.Level;
        DEBUG_MSG("[BSP-button] A12RefButton_GpioScanHandler level = %d",level);

        /* Power: active high */
        if (!level) {
            DEBUG_MSG("[BSP-button] A12RefButton_GpioScanHandler UP");
            AppButtonOp_UpdateStatus(SET_BUTTON, UI_BUTTON_UP_EVENT);
        } else {
            DEBUG_MSG("[BSP-button] A12RefButton_GpioScanHandler Down");
            AppButtonOp_UpdateStatus(SET_BUTTON, UI_BUTTON_DOWN_EVENT);
        }
    } else {
        DEBUG_MSG("[BSP-button] A12RefButton_GpioScanHandler gpioPinID = %d", gpioPinID);
    }
}

static void A12RefButton_GpioMenuScanHandler(AMBA_GPIO_PIN_ID_e gpioPinID)
{
    if (gpioPinID == GPIO_KEY_MENU) {
        AMBA_GPIO_PIN_INFO_s pinInfo;
        AMBA_GPIO_PIN_LEVEL_e level;

        AmbaGPIO_GetPinInfo(gpioPinID, &pinInfo);
        level = pinInfo.Level;
        DEBUG_MSG("[BSP-button] A12RefButton_GpioScanHandler level = %d",level);

        /* Power: active high */
        if (!level) {
            DEBUG_MSG("[BSP-button] A12RefButton_GpioScanHandler UP");
            AppButtonOp_UpdateStatus(MENU_BUTTON, UI_BUTTON_UP_EVENT);
        } else {
            DEBUG_MSG("[BSP-button] A12RefButton_GpioScanHandler Down");
            AppButtonOp_UpdateStatus(MENU_BUTTON, UI_BUTTON_DOWN_EVENT);
        }
    } else {
        DEBUG_MSG("[BSP-button] A12RefButton_GpioScanHandler gpioPinID = %d", gpioPinID);
    }
}

static void A12RefButton_GpioSnap2ScanHandler(AMBA_GPIO_PIN_ID_e gpioPinID)
{
    if (gpioPinID == GPIO_KEY_SNAP2) {
        AMBA_GPIO_PIN_INFO_s pinInfo;
        AMBA_GPIO_PIN_LEVEL_e level;

        AmbaGPIO_GetPinInfo(gpioPinID, &pinInfo);
        level = pinInfo.Level;
        DEBUG_MSG("[BSP-button] A12RefButton_GpioScanHandler level = %d",level);

        /* Power: active high */
        if (!level) {
            DEBUG_MSG("[BSP-button] A12RefButton_GpioScanHandler UP");
            AppButtonOp_UpdateStatus(SNAP2_BUTTON, UI_BUTTON_UP_EVENT);
        } else {
            DEBUG_MSG("[BSP-button] A12RefButton_GpioScanHandler Down");
            AppButtonOp_UpdateStatus(SNAP2_BUTTON, UI_BUTTON_DOWN_EVENT);
        }
    } else {
        DEBUG_MSG("[BSP-button] A12RefButton_GpioScanHandler gpioPinID = %d", gpioPinID);
    }
}

static void A12RefButton_GpioPowerScanHandler(AMBA_GPIO_PIN_ID_e gpioPinID)
{
    if (gpioPinID == GPIO_KEY_POWER) {
        AMBA_GPIO_PIN_INFO_s pinInfo;
        AMBA_GPIO_PIN_LEVEL_e level;

        AmbaGPIO_GetPinInfo(gpioPinID, &pinInfo);
        level = pinInfo.Level;
        DEBUG_MSG("[BSP-button] A12RefButton_GpioScanHandler level = %d",level);

        /* Power: active high */
        if (!level) {
            DEBUG_MSG("[BSP-button] A12RefButton_GpioScanHandler UP");
            AppButtonOp_UpdateStatus(POWER_BUTTON, UI_BUTTON_UP_EVENT);
        } else {
            DEBUG_MSG("[BSP-button] A12RefButton_GpioScanHandler Down");
            AppButtonOp_UpdateStatus(POWER_BUTTON, UI_BUTTON_DOWN_EVENT);
        }
    } else {
        DEBUG_MSG("[BSP-button] A12RefButton_GpioScanHandler gpioPinID = %d", gpioPinID);
    }
}


/**
 *  @brief Initialize the GPIO button driver.
 *
 *  Initialize the GPIO button driver.
 */
void A12RefButton_GpioInit(void)
{
    /* Initialize the GPIO device. */
    AmbaGPIO_Init();

    /* Enable the GPIO about Ok key. */
    AmbaGPIO_ConfigInput(GPIO_KEY_SET);
    AmbaGPIO_IsrHook(GPIO_KEY_SET, GPIO_INT_BOTH_EDGE_TRIGGER, &A12RefButton_GpioSetScanHandler);
    AmbaGPIO_IntEnable(GPIO_KEY_SET);
    /* Enable the GPIO about Menu key. */
    AmbaGPIO_ConfigInput(GPIO_KEY_MENU);
    AmbaGPIO_IsrHook(GPIO_KEY_MENU, GPIO_INT_BOTH_EDGE_TRIGGER, &A12RefButton_GpioMenuScanHandler);
    AmbaGPIO_IntEnable(GPIO_KEY_MENU);
    /* Enable the GPIO about  key. */
    AmbaGPIO_ConfigInput(GPIO_KEY_SNAP2);
    AmbaGPIO_IsrHook(GPIO_KEY_SNAP2, GPIO_INT_BOTH_EDGE_TRIGGER, &A12RefButton_GpioSnap2ScanHandler);
    AmbaGPIO_IntEnable(GPIO_KEY_SNAP2);
    /* Enable the GPIO about Power key. */
    AmbaGPIO_ConfigInput(GPIO_KEY_POWER);
    AmbaGPIO_IsrHook(GPIO_KEY_POWER, GPIO_INT_BOTH_EDGE_TRIGGER, &A12RefButton_GpioPowerScanHandler);
    AmbaGPIO_IntEnable(GPIO_KEY_POWER);

}


/**
 *  @brief Initialize the A12 buttons driver.
 *
 *  Initialize the A12 buttons driver.
 *
 *  @return >=0 success, <0 failure
 */
void BspRefButton_Init(void)
{
    A12RefButton_GpioInit();
    A12RefButton_AdcInit();
}

