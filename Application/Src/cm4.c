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

#define COMM_TIMEOUT 10

#define GARBAGE_THRESHOLD 10


/**********************
 *	MACROS
 **********************/


/**********************
 *	TYPEDEFS
 **********************/



/**********************
 *	VARIABLES
 **********************/



static SemaphoreHandle_t cm4_busy_sem = NULL;
static StaticSemaphore_t cm4_busy_sem_buffer;





/**********************
 *	PROTOTYPES
 **********************/

static uint8_t is_booted(void);
static void allow_boot(void);
static void hold_boot(void);

SERIAL_RET_t cm4_decode_fcn(void * inst, uint8_t data);

/**********************
 *	DECLARATIONS
 **********************/

void cm4_global_init(void) {
	cm4_busy_sem = xSemaphoreCreateMutexStatic(&cm4_busy_sem_buffer);
}

void cm4_init(CM4_INST_t * cm4) {
	static uint32_t id_counter = 0;
	cm4->id = id_counter++;
	cm4->garbage_counter = 0;
	cm4->rx_sem = xSemaphoreCreateBinaryStatic(&cm4->rx_sem_buffer);
	msv2_init(&cm4->msv2);
	serial_init(&cm4->ser, &CM4_UART, cm4, cm4_decode_fcn);


}


SERIAL_RET_t cm4_decode_fcn(void * inst, uint8_t data) {
	CM4_INST_t * cm4 = (CM4_INST_t *) inst;
	MSV2_ERROR_t tmp = msv2_decode_fragment(&cm4->msv2, data);
	if(tmp == MSV2_SUCCESS || tmp == MSV2_WRONG_CRC) {
		xSemaphoreGive(cm4->rx_sem);
	}
	return tmp;
}



CM4_ERROR_t cm4_send(CM4_INST_t * cm4, uint8_t cmd, uint8_t * data, uint16_t length, uint8_t *resp_data, uint16_t *resp_len) {
	uint16_t frame_length = msv2_create_frame(&cm4->msv2, cmd, length/2, data);
	serial_send(&cm4->ser, msv2_tx_data(&cm4->msv2), frame_length);
	if(xSemaphoreTake(cm4->rx_sem, COMM_TIMEOUT) == pdTRUE) {
		cm4->garbage_counter = 0;
		return CM4_SUCCESS;
	} else {
		cm4->garbage_counter++;
		if(cm4->garbage_counter > GARBAGE_THRESHOLD) {
			serial_garbage_clean(&cm4->ser);
		}
		return CM4_TIMEOUT;
	}

}

CM4_ERROR_t cm4_ping(CM4_INST_t * cm4) {
	CM4_ERROR_t error = 0;
	uint8_t data[] = {0xc5, 0x5c};
	error |= cm4_send(cm4, CM4_PING, data, 2, NULL, NULL);

	return error;
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
	*ready = 0;
	if(is_booted()) {
		if(cm4_ping(cm4) == CM4_SUCCESS) {
			*ready = 1;
		}
	}
	return CM4_SUCCESS;
}

CM4_ERROR_t cm4_shutdown(CM4_INST_t * cm4) {
	//send shutdown command through uart
	uint8_t data[] = {0xc8, 0x8c};
	cm4_send(cm4, CM4_SHUTDOWN, data, 2, NULL, NULL);

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
		*shutdown = 0;
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


