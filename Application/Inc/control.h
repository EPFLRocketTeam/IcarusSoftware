/*  Title       : Control
 *  Filename    : control.h
 *  Author      : iacopo sprenger
 *  Date        : 20.01.2021
 *  Version     : 0.1
 *  Description : main program control
 */

#ifndef CONTROL_H
#define CONTROL_H

/**********************
 *  INCLUDES
 **********************/

#include <stdint.h>
#include <servo.h>
#include <can_comm.h>

/**********************
 *  CONSTANTS
 **********************/



/**********************
 *  MACROS
 **********************/





/**********************
 *  TYPEDEFS
 **********************/

typedef enum CONTROL_STATE{
	CS_IDLE = 0x00,
	CS_ABORT  = 0x08,
	CS_ERROR  = 0x09
}CONTROL_STATE_t;




//action scheduling
//
//schedule: higher in the list -> higher priority
typedef enum CONTROL_SCHED{
	CONTROL_SCHED_NOTHING = 0x00,
	CONTROL_SCHED_ABORT,
	CONTROL_SCHED_MOVE_TVC,
	CONTROL_SCHED_RECOVER
}CONTROL_SCHED_t;

typedef struct CONTROL_STATUS {
	CONTROL_STATE_t state;
	uint16_t tvc_psu_voltage;
	int8_t tvc_temperature;
	uint8_t tvc_error;
	int32_t tvc_position;
	int32_t counter;
	uint8_t counter_active;
	uint32_t time;
}CONTROL_STATUS_t;


typedef struct CONTROL_INST{
	CONTROL_STATE_t state;
	uint32_t time;
	uint32_t last_time;
	int32_t counter;
	uint8_t counter_active;
	uint32_t iter;
	SERVO_INST_t * tvc_servo;
	int32_t	tvc_mov_target;
	uint8_t tvc_mov_started;
	CONTROL_SCHED_t sched;
	CAN_msg msg;
	uint8_t needs_recover;
	uint8_t hang_for_recovery;
}CONTROL_INST_t;




/**********************
 *  VARIABLES
 **********************/


/**********************
 *  PROTOTYPES
 **********************/

#ifdef __cplusplus
extern "C"{
#endif

void control_thread(void * arg);

CONTROL_STATE_t control_get_state();

void control_move_tvc(int32_t target);

void control_abort(void);
void control_recover(void);

void control_release();



#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */

#endif /* CONTROL_H */

/* END */












