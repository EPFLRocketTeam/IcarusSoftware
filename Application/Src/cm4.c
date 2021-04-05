/*  Title		: CM4
 *  Filename	: cm4.c
 *	Author		: iacopo sprenger
 *	Date		: 04.04.2021
 *	Version		: 0.1
 *	Description	: Communication with a raspberry pi cm4
 *
 *	using msv2 as a master to communicate with the CM4
 *
 *	protocol proposition:
 *		HB sends sensor data
 *		CM4 immediately responds with last gnc computation infos
 *
 *		HB sends status request
 *		CM4 responds with it's internal status (or nothing if not yet completely booted)
 */



/*
 * STARTUP SEQUENCE
 *
 * global EN is always held low on startup
 *
 *
 */

/**********************
 *	INCLUDES
 **********************/

#include <cm4.h>
#include <msv2.h>
#include <serial.h>
#include <main.h>


/**********************
 *	CONFIGURATION
 **********************/

#define CM4_UART	huart6

#define CM4_GLOBAL_EN_PIN 	GLOBAL_EN_Pin
#define CM4_GLOBAL_EN_PORT 	GLOBAL_EN_GPIO_Port

#define CM4_RUN_PG_PIN 		RUN_PG_Pin
#define CM4_RUN_PG_PORT 	RUN_PG_GPIO_Port

/**********************
 *	CONSTANTS
 **********************/


/**********************
 *	MACROS
 **********************/


/**********************
 *	TYPEDEFS
 **********************/



/**********************
 *	VARIABLES
 **********************/


static uint8_t read_run_pg(void);
static void set_global_en(void);
static void clr_global_en(void);


/**********************
 *	PROTOTYPES
 **********************/

SERIAL_RET_t cm4_decode_fcn(void * inst, uint8_t data);

/**********************
 *	DECLARATIONS
 **********************/

void cm4_init(CM4_INST_t * cm4) {
	static uint32_t id_counter = 0;
	cm4->id = id_counter++;
	msv2_init(&cm4->msv2);
	serial_init(&cm4->ser, &CM4_UART, &cm4->msv2, cm4_decode_fcn);
}


SERIAL_RET_t cm4_decode_fcn(void * inst, uint8_t data) {
	MSV2_INST_t * msv2 = (MSV2_INST_t *) inst;
	MSV2_ERROR_t tmp = msv2_decode_fragment(msv2, data);
	if(tmp == MSV2_SUCCESS || tmp == MSV2_WRONG_CRC) {
		//DO something with the reception
	}
	return tmp;
}


static uint8_t read_run_pg(void) {
	return CM4_RUN_PG_PORT->IDR & CM4_RUN_PG_PIN ?1:0;
}

static void set_global_en(void) {
	CM4_GLOBAL_EN_PORT->BSRR = CM4_GLOBAL_EN_PIN;
}

static void clr_global_en(void) {
	CM4_GLOBAL_EN_PORT->BSRR = CM4_GLOBAL_EN_PIN << 16;
}


/* END */


