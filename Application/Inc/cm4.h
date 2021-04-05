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

/**********************
 *  CONSTANTS
 **********************/


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

}CM4_INST_t;



/**********************
 *  VARIABLES
 **********************/


/**********************
 *  PROTOTYPES
 **********************/

#ifdef __cplusplus
extern "C"{
#endif


void cm4_init(CM4_INST_t * servo);


SERIAL_RET_t cm4_decode_fcn(void * inst, uint8_t data);



#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */

#endif /* CM4_H */

/* END */
