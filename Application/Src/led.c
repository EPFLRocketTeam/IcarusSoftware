/*  Title		: Led
 *  Filename	: led.c
 *	Author		: iacopo sprenger
 *	Date		: 20.01.2021
 *	Version		: 0.1
 *	Description	: rgb led control
 */

/**********************
 *	INCLUDES
 **********************/

#include <led.h>
#include <main.h>
#include <tim.h>

/**********************
 *	CONFIGURATION
 **********************/

#define LED_TIM			htim8

#define LED_PORT		LED_GPIO_Port
#define LED_PIN			LED_Pin
/**********************
 *	CONSTANTS
 **********************/

#define LED_MAX			(0xfff)



/**********************
 *	MACROS
 **********************/


/**********************
 *	TYPEDEFS
 **********************/



/**********************
 *	VARIABLES
 **********************/



/**********************
 *	PROTOTYPES
 **********************/



/**********************
 *	DECLARATIONS
 **********************/

void led_init(void) {
	LED_TIM.Instance->ARR = LED_MAX;
	LED_TIM.Instance->CCR1 = 0;
	LED_TIM.Instance->CCR2 = 0;
	LED_TIM.Instance->CCR3 = 0;
	HAL_TIMEx_PWMN_Start(&LED_TIM, TIM_CHANNEL_1);
	HAL_TIMEx_PWMN_Start(&LED_TIM, TIM_CHANNEL_2);
	HAL_TIMEx_PWMN_Start(&LED_TIM, TIM_CHANNEL_3);
}

void led_set_color(uint8_t r, uint8_t g, uint8_t b) {
	LED_TIM.Instance->CCR1 = r;
	LED_TIM.Instance->CCR2 = g;
	LED_TIM.Instance->CCR3 = b;
}

void led_toggle(void) {
	static uint8_t state = 0;

	if(state) {
		LED_PORT->BSRR = LED_PIN;
	} else {
		LED_PORT->BSRR = LED_PIN << 16;
	}

	state = !state;
}

void led_on(void) {
	LED_PORT->BSRR = LED_PIN << 16;
}

void led_off(void) {
	LED_PORT->BSRR = LED_PIN;
}


/* END */


