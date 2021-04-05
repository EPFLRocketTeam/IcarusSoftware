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


static uint8_t is_booted(void);
static void allow_boot(void);
static void hold_boot(void);


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


CM4_ERROR_t cm4_transaction(CM4_INST_t * cm4, CM4_PAYLOAD_COMMAND_t * cmd, CM4_PAYLOAD_SENSOR_t * sens) {

	return CM4_SUCCESS;
}

CM4_ERROR_t cm4_boot(CM4_INST_t * cm4) {
	allow_boot();
	return CM4_SUCCESS;
}

CM4_ERROR_t cm4_is_ready(CM4_INST_t * cm4, uint8_t * ready) {
	if(ready == NULL){
		return CM4_LOCAL_ERROR;
	}
	if(is_booted()) {
		//check wheter the CM4 answers
		//if answers
		*ready = 1;
		return CM4_SUCCESS;

	} else {
		*ready = 0;
		return CM4_SUCCESS;
	}
}

CM4_ERROR_t cm4_shutdown(CM4_INST_t * cm4) {
	//send shutdown command through uart
	return CM4_SUCCESS;
}

CM4_ERROR_t cm4_is_shutdown(CM4_INST_t * cm4, uint8_t * shutdown) {
	if(shutdown == NULL){
		return CM4_LOCAL_ERROR;
	}
	if(!is_booted()) {
		//check wheter the CM4 answers
		//if answers
		hold_boot();
		*shutdown = 1;
		return CM4_SUCCESS;
	} else {
		*shutdown = 1;
		return CM4_SUCCESS;
	}
}




static uint8_t is_booted(void) {
	return CM4_RUN_PG_PORT->IDR & CM4_RUN_PG_PIN ?1:0;
}

static void allow_boot(void) {
	CM4_GLOBAL_EN_PORT->BSRR = CM4_GLOBAL_EN_PIN;
}

static void hold_boot(void) {
	CM4_GLOBAL_EN_PORT->BSRR = CM4_GLOBAL_EN_PIN << 16;
}


/* END */


