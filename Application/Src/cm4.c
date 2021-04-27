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
#include <control.h>
#include <pipeline.h>


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

#define COMM_TIMEOUT pdMS_TO_TICKS(10)
#define DRIV_TIMEOUT pdMS_TO_TICKS(200)
#define LONG_TIME 0xffff

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

void cm4_generate_response(CM4_INST_t * cm4);

static void cm4_response_ping(uint8_t * data, uint16_t data_len, uint8_t * resp, uint16_t * resp_len);
static void cm4_response_command(uint8_t * data, uint16_t data_len, uint8_t * resp, uint16_t * resp_len);




static void (*response_fcn[]) (uint8_t *, uint16_t, uint8_t *, uint16_t *) = {
		cm4_response_ping, //0x80
		cm4_response_command // 0x81
};

static uint16_t response_fcn_max = sizeof(response_fcn) / sizeof(void *);


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
		if(cm4->msv2.rx.opcode & 0x80) { //CM4 is master
			cm4_generate_response(cm4);
		} else { //HB is master
			xSemaphoreGive(cm4->rx_sem);
		}

	}
	return tmp;
}

void cm4_generate_response(CM4_INST_t * cm4) {
	static uint8_t send_data[MSV2_MAX_DATA_LEN];
	static uint16_t length = 0;
	static uint16_t bin_length = 0;
	uint8_t opcode = cm4->msv2.rx.opcode;
	opcode &= ~CM4_C2H_PREFIX;
	if(opcode < response_fcn_max) {
		response_fcn[opcode](cm4->msv2.rx.data, cm4->msv2.rx.length, send_data, &length);
		//length is in words
		bin_length = msv2_create_frame(&cm4->msv2, cm4->msv2.rx.opcode, length/2, send_data);
		serial_send(&cm4->ser, msv2_tx_data(&cm4->msv2), bin_length);
	} else {
		send_data[0] = MSV2_CRC_ERROR_LO;
		send_data[1] = MSV2_CRC_ERROR_HI;
		length = 2;
		bin_length = msv2_create_frame(&cm4->msv2, cm4->msv2.rx.opcode, length/2, send_data);
		serial_send(&cm4->ser, msv2_tx_data(&cm4->msv2), bin_length);
	}
}

static void cm4_response_ping(uint8_t * data, uint16_t data_len, uint8_t * resp, uint16_t * resp_len) {
	resp[0] = MSV2_OK_LO;
	resp[1] = MSV2_OK_HI;
	*resp_len = 2;
}

static void cm4_response_command(uint8_t * data, uint16_t data_len, uint8_t * resp, uint16_t * resp_len) {
	CM4_PAYLOAD_COMMAND_t cmd = {0};
	if(data_len == 50) {
		cmd.timestamp = util_decode_u32(data);
		cmd.thrust = util_decode_i32(data+4);

		cmd.dynamixel[0] = util_decode_i32(data+8);
		cmd.dynamixel[1] = util_decode_i32(data+12);
		cmd.dynamixel[2] = util_decode_i32(data+16);
		cmd.dynamixel[3] = util_decode_i32(data+20);

		cmd.position[0] = util_decode_i32(data+24);
		cmd.position[1] = util_decode_i32(data+28);
		cmd.position[2] = util_decode_i32(data+32);

		cmd.speed[0] = util_decode_i32(data+36);
		cmd.speed[1] = util_decode_i32(data+40);
		cmd.speed[2] = util_decode_i32(data+44);

		cmd.state = util_decode_u16(data+48);

		control_set_cmd(cmd);

		pipeline_send_control(&cmd);

		resp[0] = MSV2_OK_LO;
		resp[1] = MSV2_OK_HI;
		*resp_len = 2;
	} else {
		resp[0] = MSV2_ERROR_LO;
		resp[1] = MSV2_ERROR_HI;
		*resp_len = 2;
	}
}



CM4_ERROR_t cm4_send(CM4_INST_t * cm4, uint8_t cmd, uint8_t * data, uint16_t length, uint8_t ** resp_data, uint16_t * resp_len) {
	if (xSemaphoreTake(cm4_busy_sem, DRIV_TIMEOUT) == pdTRUE) {
		uint16_t frame_length = msv2_create_frame(&cm4->msv2, cmd, length/2, data);
		serial_send(&cm4->ser, msv2_tx_data(&cm4->msv2), frame_length);
		if(cm4->rx_sem == NULL) {
			xSemaphoreGive(cm4_busy_sem);
			return CM4_LOCAL_ERROR;
		}
		if(xSemaphoreTake(cm4->rx_sem, COMM_TIMEOUT) == pdTRUE) {
			cm4->garbage_counter = 0;
			if(cm4->msv2.rx.opcode == cmd) {
				if(resp_len != NULL && resp_data != NULL) {
					*resp_len = cm4->msv2.rx.length;
					*resp_data = cm4->msv2.rx.data;
				}
				xSemaphoreGive(cm4_busy_sem);
				return CM4_SUCCESS;
			} else {
				if(resp_len != NULL) {
					*resp_len = 0;
				}
				xSemaphoreGive(cm4_busy_sem);
				return CM4_REMOTE_ERROR;
			}
		} else {
			cm4->garbage_counter++;
			if(cm4->garbage_counter > GARBAGE_THRESHOLD) {
				serial_garbage_clean(&cm4->ser);
				cm4->garbage_counter = 0;
			}
			xSemaphoreGive(cm4_busy_sem);
			return CM4_TIMEOUT;
		}

	} else {
		return CM4_BUSY;
	}

}

CM4_ERROR_t cm4_ping(CM4_INST_t * cm4) {
	CM4_ERROR_t error = 0;
	uint8_t data[] = {0xc5, 0x5c};
	error |= cm4_send(cm4, CM4_H2C_PING, data, 2, NULL, NULL);

	return error;
}


CM4_ERROR_t cm4_send_sensors(CM4_INST_t * cm4, CM4_PAYLOAD_SENSOR_t * sens) {
	CM4_ERROR_t error = 0;
	uint8_t * recv_data;
	uint16_t recv_len = 0;
	uint16_t send_len = 32;
	uint8_t send_data[32];

	util_encode_u32(send_data, sens->timestamp);
	util_encode_i32(send_data+4, sens->acc_x);
	util_encode_i32(send_data+8, sens->acc_y);
	util_encode_i32(send_data+12, sens->acc_z);

	util_encode_i32(send_data+16, sens->gyro_x);
	util_encode_i32(send_data+20, sens->gyro_y);
	util_encode_i32(send_data+24, sens->gyro_z);

	util_encode_i32(send_data+28, sens->baro);

	error |= cm4_send(cm4, CM4_H2C_SENSORS, send_data, send_len , &recv_data, &recv_len);

	//EVENTUAL ACKNOWLEGE

	return error;
}

CM4_ERROR_t cm4_send_feedback(CM4_INST_t * cm4, CM4_PAYLOAD_FEEDBACK_t * feed) {
	CM4_ERROR_t error = 0;
	uint8_t * recv_data;
	uint16_t recv_len = 0;
	uint16_t send_len = 24;
	uint8_t send_data[24];

	util_encode_u32(send_data, feed->timestamp);
	util_encode_i32(send_data+4, feed->cc_pressure);

	util_encode_i32(send_data+8, feed->dynamixel[0]);
	util_encode_i32(send_data+12, feed->dynamixel[1]);
	util_encode_i32(send_data+16, feed->dynamixel[2]);
	util_encode_i32(send_data+20, feed->dynamixel[3]);

	error |= cm4_send(cm4, CM4_H2C_FEEDBACK, send_data, send_len , &recv_data, &recv_len);

	//EVENTUAL ACKNOWLEGE

	return error;
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
	uint8_t data[] = {0x00, 0x00};
	cm4_send(cm4, CM4_H2C_SHUTDOWN, data, 2, NULL, NULL);

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


CM4_ERROR_t cm4_force_shutdown(CM4_INST_t * cm4) {
	hold_boot();
	return CM4_SUCCESS;
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


