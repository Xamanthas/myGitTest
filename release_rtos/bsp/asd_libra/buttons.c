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

#define ARD_GPIO_SCAN (1)
#define SCAN_CYC_TIME  (35)

#if ARD_GPIO_SCAN
typedef enum {
	NULL_KEY,
	SHORT_KEY,
	LONG_KEY
}key_type;

typedef struct gpio_btoc_s {
	AMBA_GPIO_PIN_ID_e	gpio_id;	/* Gpio pin ID */
	UINT32	bid_short;	/* Button ID when short press*/
	UINT32	bid_long;	/* Button ID when long press*/	
	AMBA_GPIO_PIN_LEVEL_e	pressed_lv;	/*Gpio pin voltage when pressed */
	AMBA_GPIO_PIN_LEVEL_e	current_lv;	/* Gpio pin current voltage */	
	
	int	pressed_cyc_cnt;	/*The duration of button pressed */
	UINT32	sent_dwn_time;	/*The time that sent button down msg */
	key_type	msg_sent;	/* The latest key type*/
} gpio_btoc_t;

#define GPIO_KEY_NUM	(5)
#define GPIO_KEY_SHORT_THRESHOLD  (100)
#define GPIO_KEY_LONG_THRESHOLD  (1000)
#define GPIO_KEY_CLR_TIME  (100)   //the interval time of CLR msg after DOWN msg be sent.

static gpio_btoc_t G_gpio_btoc[GPIO_KEY_NUM] = {
    { GPIO_KEY_POWER,	F0_BUTTON,	POWER_BUTTON,	AMBA_GPIO_LEVEL_HIGH,   AMBA_GPIO_LEVEL_LOW,	0,	0,	NULL_KEY},    /* power  */
    { GPIO_KEY_RECORD,  RECORD_BUTTON,  RECORD_BUTTON,   AMBA_GPIO_LEVEL_LOW,   AMBA_GPIO_LEVEL_HIGH,    0,  0,  NULL_KEY},    /* record  */
    { GPIO_KEY_UP,   UP_BUTTON,  UP_BUTTON,   AMBA_GPIO_LEVEL_LOW,   AMBA_GPIO_LEVEL_HIGH,    0,  0,  NULL_KEY},    /* Up  */
    { GPIO_KEY_DOWN,   DOWN_BUTTON,  DOWN_BUTTON,   AMBA_GPIO_LEVEL_LOW,   AMBA_GPIO_LEVEL_HIGH,    0,  0,  NULL_KEY},    /* Down  */
    { GPIO_KEY_AV_PWR_EN,   F1_BUTTON,  F1_BUTTON,	AMBA_GPIO_LEVEL_LOW,   AMBA_GPIO_LEVEL_HIGH,	0,	0,	NULL_KEY},	  /* F1	*/	
};
static gpio_btoc_t *G_gpio_btocs =	G_gpio_btoc ;
static UINT32   Timer_cycle_counter = 0;
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
#define ADC_BT_CH_NUM         1

static ADC_BUTTON_s GAdcButton1[] = {
    { MODE_BUTTON,          773,  1173 }, /**< MODE */
    { RIGHT_BUTTON,       1278,  1678 }, /**< RIGHT */
    { SET_BUTTON,         1758, 2158 }, /**< UP */
    { LEFT_BUTTON,      2255, 2655 }, /**<  LEFT*/
    { MENU_BUTTON,   2785, 3185 }, /**< MENU */
    { CHAN_NORM_POS,       0,      0 }
};

extern int AppButtonOp_UpdateStatus(UINT32 buttonId, UINT32 event) __attribute__((weak));
static ADC_BUTTON_s *GAdcButtons[ADC_BT_CH_NUM] = {
    GAdcButton1
};

static INT32 DebounceCounts[ADC_BT_CH_NUM] = {0};
static INT32 DebounceKeys[ADC_BT_CH_NUM] = {-1};
static INT32 LastChanBIds[ADC_BT_CH_NUM] = {-1};

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
    UINT32 AdcValues[ADC_BT_CH_NUM] = {0};
    INT32 CurrentButtonIds[ADC_BT_CH_NUM] = {-1};

    //AmbaPrint("[BSP-button] <A12RefButton_AdcScanHandler> Start");
    /*Get the ADC data.*/
    AmbaADC_DataCollection();

    AdcValues[0] = AmbaAdcData[AMBA_ADC_CHANNEL3];
    //AmbaPrint("adc[3]=%d",AdcValues[0]);
    
#if defined(ARD_GPIO_SCAN)
        for(i=0;i<GPIO_KEY_NUM;i++) {
            AMBA_GPIO_PIN_INFO_s pinInfo;     
            AmbaGPIO_GetPinInfo(G_gpio_btocs[i].gpio_id, &pinInfo);
            G_gpio_btocs[i].current_lv = pinInfo.Level;
        }
#endif

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
                //DEBUG_MSG("[BSP-button] key : %x up", LastChanBIds[i]);
                LastChanBIds[i] = -1;
            }
            if (DebounceKeys[i] != CHAN_NORM_POS) {
                LastChanBIds[i] = DebounceKeys[i];
                AppButtonOp_UpdateStatus(LastChanBIds[i], UI_BUTTON_DOWN_EVENT);
                //DEBUG_MSG("[BSP-button] key : %x down", LastChanBIds[i]);
            }
        }
    }

#if defined(ARD_GPIO_SCAN)	
        Timer_cycle_counter++;
        if(Timer_cycle_counter == 0xFFFFFFFF){
            Timer_cycle_counter = 0;
        }
        
	for(i=0;i<GPIO_KEY_NUM;i++) {
		if(G_gpio_btocs[i].current_lv == G_gpio_btocs[i].pressed_lv) {
			G_gpio_btocs[i].pressed_cyc_cnt++;
			if(G_gpio_btocs[i].msg_sent == NULL_KEY) {
				if((G_gpio_btocs[i].pressed_cyc_cnt*SCAN_CYC_TIME) >= GPIO_KEY_LONG_THRESHOLD) {
					G_gpio_btocs[i].msg_sent = LONG_KEY;
					AppButtonOp_UpdateStatus(G_gpio_btocs[i].bid_long, UI_BUTTON_DOWN_EVENT);
					G_gpio_btocs[i].sent_dwn_time = Timer_cycle_counter;
					//DEBUG_MSG("long");
				}
			}
		}else {
			if(G_gpio_btocs[i].msg_sent == NULL_KEY) {
				if((G_gpio_btocs[i].pressed_cyc_cnt*SCAN_CYC_TIME) >= GPIO_KEY_SHORT_THRESHOLD) {
					G_gpio_btocs[i].msg_sent = SHORT_KEY;
					AppButtonOp_UpdateStatus(G_gpio_btocs[i].bid_short, UI_BUTTON_DOWN_EVENT);
					G_gpio_btocs[i].sent_dwn_time = Timer_cycle_counter;
					//DEBUG_MSG("short");
				}
			}
			G_gpio_btocs[i].pressed_cyc_cnt = 0;
			if(G_gpio_btocs[i].msg_sent != NULL_KEY) {
				UINT32 dtime;
				if (Timer_cycle_counter > G_gpio_btocs[i].sent_dwn_time){
					dtime = Timer_cycle_counter - G_gpio_btocs[i].sent_dwn_time;
				}else{
					dtime = 0xFFFFFFFF - G_gpio_btocs[i].sent_dwn_time+ Timer_cycle_counter + 1;
				}
				dtime = dtime*SCAN_CYC_TIME;
				if(dtime >GPIO_KEY_CLR_TIME ) {
					if(G_gpio_btocs[i].msg_sent == LONG_KEY) {
						AppButtonOp_UpdateStatus(G_gpio_btocs[i].bid_long, UI_BUTTON_UP_EVENT);
						//DEBUG_MSG("up L");
					} else if(G_gpio_btocs[i].msg_sent == SHORT_KEY) {
						AppButtonOp_UpdateStatus(G_gpio_btocs[i].bid_short, UI_BUTTON_UP_EVENT);
						//DEBUG_MSG("up S");
					}					
					G_gpio_btocs[i].msg_sent = NULL_KEY;
				}				
			}
		}
	}	
#endif
}

static AMBA_KAL_TIMER_t ButtonTimer = {0};
static UINT32 ButtontTimerId = SCAN_CYC_TIME;

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
    AmbaADC_Init();
    AmbaADC_Start();

    /* Register a system timer for button. */
    ReturnValue = AmbaKAL_TimerCreate(&ButtonTimer, AMBA_KAL_AUTO_START,
                                &A12RefButton_AdcScanHandler, ButtontTimerId,
                                SCAN_CYC_TIME, SCAN_CYC_TIME);
    if (ReturnValue < 0) {
        AmbaPrint("[BSP-button] Register timer fail.");
        K_ASSERT(0);
    }
}


/***************************************************************
* GPIO button releated functions
***************************************************************/
#if ARD_GPIO_SCAN		
    /* Initialize the GPIO button driver.*/
 void A12_ARD_key_gpio_init(void)
 {
     int i;

     /* Initialize the GPIO device. */
     AmbaGPIO_Init();
          
     for(i=0;i<GPIO_KEY_NUM;i++) {            
         AmbaGPIO_ConfigInput(G_gpio_btocs[i].gpio_id);
         if(GPIO_KEY_POWER != G_gpio_btocs[i].gpio_id){
         	AmbaGPIO_PullCtrl(G_gpio_btocs[i].gpio_id,AMBA_GPIO_PULL_UP);
         }
     }
 }
#else
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


static void A12RefButton_GpioUpScanHandler(AMBA_GPIO_PIN_ID_e gpioPinID)
{
    if (gpioPinID == GPIO_KEY_UP) {
        AMBA_GPIO_PIN_INFO_s pinInfo;
        AMBA_GPIO_PIN_LEVEL_e level;

        AmbaGPIO_GetPinInfo(gpioPinID, &pinInfo);
        level = pinInfo.Level;
        DEBUG_MSG("[BSP-button] A12RefButton_GpioScanHandler level = %d",level);

        /* Power: active high */
        if (!level) {
            DEBUG_MSG("[BSP-button] A12RefButton_GpioScanHandler UP");
            AppButtonOp_UpdateStatus(UP_BUTTON, UI_BUTTON_UP_EVENT);
        } else {
            DEBUG_MSG("[BSP-button] A12RefButton_GpioScanHandler Down");
            AppButtonOp_UpdateStatus(UP_BUTTON, UI_BUTTON_DOWN_EVENT);
        }
    } else {
        DEBUG_MSG("[BSP-button] A12RefButton_GpioScanHandler gpioPinID = %d", gpioPinID);
    }
}


static void A12RefButton_GpioDownScanHandler(AMBA_GPIO_PIN_ID_e gpioPinID)
{
    if (gpioPinID == GPIO_KEY_DOWN) {
        AMBA_GPIO_PIN_INFO_s pinInfo;
        AMBA_GPIO_PIN_LEVEL_e level;

        AmbaGPIO_GetPinInfo(gpioPinID, &pinInfo);
        level = pinInfo.Level;
        DEBUG_MSG("[BSP-button] A12RefButton_GpioScanHandler level = %d",level);

        /* Power: active high */
        if (!level) {
            DEBUG_MSG("[BSP-button] A12RefButton_GpioScanHandler UP");
            AppButtonOp_UpdateStatus(DOWN_BUTTON, UI_BUTTON_UP_EVENT);
        } else {
            DEBUG_MSG("[BSP-button] A12RefButton_GpioScanHandler Down");
            AppButtonOp_UpdateStatus(DOWN_BUTTON, UI_BUTTON_DOWN_EVENT);
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
    /* Enable the GPIO about  Up key. */
    AmbaGPIO_ConfigInput(GPIO_KEY_UP);
    AmbaGPIO_IsrHook(GPIO_KEY_UP, GPIO_INT_BOTH_EDGE_TRIGGER, &A12RefButton_GpioUpScanHandler);
    AmbaGPIO_IntEnable(GPIO_KEY_UP);
    /* Enable the GPIO about  Down key. */
    AmbaGPIO_ConfigInput(GPIO_KEY_DOWN);
    AmbaGPIO_IsrHook(GPIO_KEY_DOWN, GPIO_INT_BOTH_EDGE_TRIGGER, &A12RefButton_GpioDownScanHandler);
    AmbaGPIO_IntEnable(GPIO_KEY_DOWN);



}
#endif  


/**
 *  @brief Initialize the A12 buttons driver.
 *
 *  Initialize the A12 buttons driver.
 *
 *  @return >=0 success, <0 failure
 */
void BspRefButton_Init(void)
{
#if ARD_GPIO_SCAN		
    /* GPIO button */
    A12_ARD_key_gpio_init();
#else
    A12RefButton_GpioInit();
#endif    
    A12RefButton_AdcInit();
}

