/*  Title       : Pipeline
 *  Filename    : pipeline.h
 *  Author      : iacopo sprenger
 *  Date        : 27.04.2021
 *  Version     : 0.1
 *  Description : data pipeline
 */

#ifndef PIPELINE_H
#define PIPELINE_H



/**********************
 *  INCLUDES
 **********************/

#include <stdint.h>

#include <cm4.h>
#include <control.h>

/**********************
 *  CONSTANTS
 **********************/



/**********************
 *  MACROS
 **********************/


/**********************
 *  TYPEDEFS
 **********************/


typedef enum PIPE_ERROR {
	PIPE_SUCCESS,
	PIPE_ERROR
}PIPE_ERROR_t;

typedef struct PIPE_INST {
	uint32_t id;
	void * data;
	//semaphore
}PIPE_INST_t;



/**********************
 *  VARIABLES
 **********************/


/**********************
 *  PROTOTYPES
 **********************/

#ifdef __cplusplus
extern "C"{
#endif

void pipeline_init(CM4_INST_t * cm4);

void pipeline_thread(void * arg);

void pipeline_send_control(CM4_PAYLOAD_COMMAND_t * cmd);

void pipeline_send_heartbeat(CONTROL_STATE_t state, uint8_t gnc_state, uint32_t time);

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */

#endif /* PIPELINE_H */

/* END */
