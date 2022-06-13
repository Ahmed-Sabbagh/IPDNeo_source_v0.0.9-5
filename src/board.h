/* board.h - Board-specific hooks */

/*
 * Copyright (c) 2020 tecVenture
 *
 */
 
#if defined(CONFIG_BOARD_WALTHERNEOV2)

#define EXT_LED_L1_R_GPIO_PIN     28   /*  */
#define EXT_LED_L1_G_GPIO_PIN     27   /*  */
#define EXT_LED_L1_B_GPIO_PIN     29   /*  */
#define EXT_LED_L2_R_GPIO_PIN     25   /*  */
#define EXT_LED_L2_G_GPIO_PIN     10   /*  */
#define EXT_LED_L2_B_GPIO_PIN     31   /*  */
#define EXT_LED_L3_R_GPIO_PIN     26   /*  */
#define EXT_LED_L3_G_GPIO_PIN     30   /*  */
#define EXT_LED_L3_B_GPIO_PIN     0   /*  */
#define EXT_LED_N_R_GPIO_PIN      20   /*  */
#define EXT_LED_N_G_GPIO_PIN      15   /*  */
#define EXT_LED_N_B_GPIO_PIN      16   /*  */
#define EXT_LED_PE_R_GPIO_PIN     12   /*  */
#define EXT_LED_PE_G_GPIO_PIN     14   /*  */
#define EXT_LED_PE_B_GPIO_PIN     19   /*  */
#define EXT_LED_TEMP_R_GPIO_PIN   11   /*  */
#define EXT_LED_TEMP_G_GPIO_PIN   13   /*  */
#define EXT_LED_TEMP_B_GPIO_PIN   17   /*  */

#elif defined(CONFIG_BOARD_WALTHERNEOV3)

#define EXT_LED_L1_R_GPIO_PIN     29   /*  */
#define EXT_LED_L1_W_GPIO_PIN     28   /*  */
#define EXT_LED_L2_R_GPIO_PIN     11   /*  */
#define EXT_LED_L2_W_GPIO_PIN     10   /*  */
#define EXT_LED_L3_R_GPIO_PIN     26   /*  */
#define EXT_LED_L3_W_GPIO_PIN     25   /*  */
#define EXT_LED_N_R_GPIO_PIN      20   /*  */
#define EXT_LED_N_W_GPIO_PIN      19   /*  */
#define EXT_LED_PE_R_GPIO_PIN     16   /*  */
#define EXT_LED_PE_G_GPIO_PIN     15   /*  */
#define EXT_LED_PE_B_GPIO_PIN     17   /*  */
#define EXT_LED_TEMP_R_GPIO_PIN   13   /*  */
#define EXT_LED_TEMP_G_GPIO_PIN   14   /*  */
#define EXT_LED_TEMP_B_GPIO_PIN   12   /*  */

#elif defined(CONFIG_BOARD_WALTHERNEOV4)

#define EXT_LED_L1_R_GPIO_PIN     29   /*  */
#define EXT_LED_L1_W_GPIO_PIN     27   /*  */
#define EXT_LED_L2_R_GPIO_PIN     11   /*  */
#define EXT_LED_L2_W_GPIO_PIN     10   /*  */
#define EXT_LED_L3_R_GPIO_PIN     22   /*  */
#define EXT_LED_L3_W_GPIO_PIN     23   /*  */
#define EXT_LED_N_R_GPIO_PIN      20   /*  */
#define EXT_LED_N_W_GPIO_PIN      19   /*  */
#define EXT_LED_PE_R_GPIO_PIN     16   /*  */
#define EXT_LED_PE_G_GPIO_PIN     15   /*  */
#define EXT_LED_PE_B_GPIO_PIN     17   /*  */
#define EXT_LED_TEMP_R_GPIO_PIN   13   /*  */
#define EXT_LED_TEMP_G_GPIO_PIN   14   /*  */
#define EXT_LED_TEMP_B_GPIO_PIN   12   /*  */

#else
#error "No Board selected"
#endif


#define WALTHER_PRODUCT_ID			0x0001
#define WALTHER_VERSION_ID			0x0003
