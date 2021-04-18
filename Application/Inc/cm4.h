/*  Title       : CM4
 *  Filename    : cm4.h
 *  Author      : iacopo sprenger
 *  Date        : 04.04.2021
 *  Version     : 0.1
 *  Description : communication with a raspberry pi cm4
 */

#ifndef CM4_H
#define CM4_H



/**********************
 *  INCLUDES
 **********************/

#include <stdint.h>
#include <msv2.h>
#include <serial.h>
#include <cmsis_os.h>

/**********************
 *  CONSTANTS
 **********************/

#define CM4_MASTER_PING			0x00
#define CM4_MASTER_SHUTDOWN		0x01
#define CM4_MASTER_PAYLOAD		0x02
#define CM4_MASTER SENSORS		0x03


//Initiated by CM4
#define CM4_SALVE_PREFIX		0x80

#define CM4_SLAVE_PING			(0x00 | CM4_SLAVE_PREFIX)
#define CM4_SLAVE_COMMAND		(0x01 | CM4_SLAVE_PREFIX)



/**********************
 *  MACROS
 **********************/


/**********************
 *  TYPEDEFS
 **********************/

typedef enum CM4_ERROR {
	CM4_SUCCESS = 0,
	CM4_TIMEOUT = 0b001,
	CM4_REMOTE_ERROR = 0b010,
	CM4_BUSY = 0b100,
	CM4_LOCAL_ERROR = 0b1000
}CM4_ERROR_t;

typedef enum CM4_STATE {
	CM4_POWERED_DOWN,
	CM4_BOOTING,
	CM4_PREPARING,
	CM4_READY,
	CM4_ERROR
}CM4_STATE_t;

typedef struct CM4_INST {
	uint32_t id;
	MSV2_INST_t msv2;
	SERIAL_INST_t ser;
	SemaphoreHandle_t rx_sem;
	StaticSemaphore_t rx_sem_buffer;
	uint16_t garbage_counter;
}CM4_INST_t;

typedef struct CM4_PAYLOAD_SENSOR {
	uint32_t timestamp;
	int32_t acc_x;
	int32_t acc_y;
	int32_t acc_z;
	int32_t gyro_x;
	int32_t gyro_y;
	int32_t gyro_z;
	int32_t baro;
	int32_t alti;
	int32_t cc_pressure;
	int32_t dynamixel[4];
}CM4_PAYLOAD_SENSOR_t;

typedef struct CM4_PAYLOAD_COMMAND {
	uint32_t timestamp;
	int32_t thrust;
	int32_t dynamixel[4];
	int32_t position[3];
	int32_t speed[3];
	uint16_t state;
}CM4_PAYLOAD_COMMAND_t;



/**********************
 *  VARIABLES
 **********************/


/**********************
 *  PROTOTYPES
 **********************/

#ifdef __cplusplus
extern "C"{
#endif

void cm4_global_init(void);
void cm4_init(CM4_INST_t * servo);


SERIAL_RET_t cm4_decode_fcn(void * inst, uint8_t data);

CM4_ERROR_t cm4_send(CM4_INST_t * cm4, uint8_t cmd, uint8_t * data, uint16_t length, uint8_t ** resp_data, uint16_t * resp_len);

CM4_ERROR_t cm4_transaction(CM4_INST_t * cm4, CM4_PAYLOAD_SENSOR_t * sens, CM4_PAYLOAD_COMMAND_t * cmd);

CM4_ERROR_t cm4_boot(CM4_INST_t * cm4);

CM4_ERROR_t cm4_is_ready(CM4_INST_t * cm4, uint8_t * ready);

CM4_ERROR_t cm4_shutdown(CM4_INST_t * cm4);

CM4_ERROR_t cm4_is_shutdown(CM4_INST_t * cm4, uint8_t * shutdown);

CM4_ERROR_t cm4_force_shutdown(CM4_INST_t * cm4);



#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */

#endif /* CM4_H */

/* END */
