/*  Title		: Pipeline
 *  Filename	: pipeline.c
 *	Author		: iacopo sprenger
 *	Date		: 27.04.2021
 *	Version		: 0.1
 *	Description	: data pipeline
 */

/**********************
 *	INCLUDES
 **********************/


#include <cmsis_os.h>

#include <can_comm.h>

#include <pipeline.h>


/**********************
 *	CONSTANTS
 **********************/

#define PIPELINE_HEART_BEAT	1


/**********************
 *	MACROS
 **********************/


/**********************
 *	TYPEDEFS
 **********************/

typedef enum PIPELINE_CONTROL_FLAGS {
	PIPELINE_CONTROL_THRUST 	= 0b00001,
	PIPELINE_CONTROL_DYN1 		= 0b00010,
	PIPELINE_CONTROL_DYN2 		= 0b00100,
	PIPELINE_CONTROL_DYN3 		= 0b01000,
	PIPELINE_CONTROL_DYN4 		= 0b10000,
	PIPELINE_CONTROL_ALL 		= 0b11111
}PIPELINE_CONTROL_FLAGS_t;

typedef struct PIPELINE_CONTROL_DATA {
	uint32_t timestamp;
	int32_t thrust;
	int32_t dyn1;
	int32_t dyn2;
	int32_t dyn3;
	int32_t dyn4;
}PIPELINE_CONTROL_DATA_t;

typedef enum PIPELINE_SENSORS_FLAGS {
	PIPELINE_SENSORS_ACC_X		= 0b0000001,
	PIPELINE_SENSORS_ACC_Y 		= 0b0000010,
	PIPELINE_SENSORS_ACC_Z 		= 0b0000100,
	PIPELINE_SENSORS_GYRO_X 	= 0b0001000,
	PIPELINE_SENSORS_GYRO_Y 	= 0b0010000,
	PIPELINE_SENSORS_GYRO_Z 	= 0b0100000,
	PIPELINE_SENSORS_ALTI		= 0b1000000,
	PIPELINE_SENSORS_ALL		= 0b1111111
}PIPELINE_SENSORS_FLAGS_t;

typedef struct PIPELINE_SENSORS_DATA {
	uint32_t timestamp;
	int32_t acc_x;
	int32_t acc_y;
	int32_t acc_z;
	int32_t gyro_x;
	int32_t gyro_y;
	int32_t gyro_z;
	int32_t alti;
}PIPELINE_SENSORS_DATA_t;

typedef enum PIPELINE_FEEDBACK_FLAGS {
	PIPELINE_FEEDBACK_THRUST 	= 0b00001,
	PIPELINE_FEEDBACK_DYN1 		= 0b00010,
	PIPELINE_FEEDBACK_DYN2 		= 0b00100,
	PIPELINE_FEEDBACK_DYN3 		= 0b01000,
	PIPELINE_FEEDBACK_DYN4 		= 0b10000,
	PIPELINE_FEEDBACK_ALL 		= 0b11111
}PIPELINE_FEEDBACK_FLAGS_t;

typedef struct PIPELINE_FEEDBACK_DATA {
	uint32_t timestamp;
	int32_t thrust;
	int32_t dyn1;
	int32_t dyn2;
	int32_t dyn3;
	int32_t dyn4;
}PIPELINE_FEEDBACK_DATA_t;

typedef struct PIPELINE_INST {
	CAN_msg msg;
	PIPELINE_CONTROL_FLAGS_t control_flags;
	PIPELINE_CONTROL_DATA_t control_data;
	PIPELINE_SENSORS_FLAGS_t sensors_flags;
	PIPELINE_SENSORS_DATA_t sensors_data;
	PIPELINE_FEEDBACK_FLAGS_t feedback_flags;
	PIPELINE_FEEDBACK_DATA_t feedback_data;

}PIPELINE_INST_t;


/**********************
 *	VARIABLES
 **********************/

static PIPELINE_INST_t pipeline;



/**********************
 *	PROTOTYPES
 **********************/


/**********************
 *	DECLARATIONS
 **********************/

void pipeline_thread(void * arg) {

	static TickType_t last_wake_time;
	static const TickType_t period = pdMS_TO_TICKS(PIPELINE_HEART_BEAT);

	last_wake_time = xTaskGetTickCount();



	for(;;) {

		//Receive all can messages
		while(can_msgPending()) {
			pipeline.msg = can_readBuffer();


			if(pipeline.msg.id == DATA_ID_ALTITUDE){
				pipeline.sensors_data.alti = (int32_t) pipeline.msg.data;
				pipeline.sensors_flags |= PIPELINE_SENSORS_ALTI;

			} else if(pipeline.msg.id == DATA_ID_ACCELERATION_X) {
				pipeline.sensors_data.acc_x = (int32_t) pipeline.msg.data;
				pipeline.sensors_flags |= PIPELINE_SENSORS_ACC_X;

			} else if(pipeline.msg.id == DATA_ID_ACCELERATION_Y) {
				pipeline.sensors_data.acc_y = (int32_t) pipeline.msg.data;
				pipeline.sensors_flags |= PIPELINE_SENSORS_ACC_Y;

			} else if(pipeline.msg.id == DATA_ID_ACCELERATION_Z) {
				pipeline.sensors_data.acc_z = (int32_t) pipeline.msg.data;
				pipeline.sensors_flags |= PIPELINE_SENSORS_ACC_Z;

			} else if(pipeline.msg.id == DATA_ID_GYRO_X) {
				pipeline.sensors_data.gyro_x = (int32_t) pipeline.msg.data;
				pipeline.sensors_flags |= PIPELINE_SENSORS_GYRO_X;

			} else if(pipeline.msg.id == DATA_ID_GYRO_Y) {
				pipeline.sensors_data.gyro_y = (int32_t) pipeline.msg.data;
				pipeline.sensors_flags |= PIPELINE_SENSORS_GYRO_Y;

			} else if(pipeline.msg.id == DATA_ID_GYRO_Z) {
				pipeline.sensors_data.gyro_z = (int32_t) pipeline.msg.data;
				pipeline.sensors_flags |= PIPELINE_SENSORS_GYRO_Z;

			} else if(pipeline.msg.id == DATA_ID_PRESS_2) {
				pipeline.feedback_data.thrust = (int32_t) pipeline.msg.data;
				pipeline.feedback_flags |= PIPELINE_FEEDBACK_THRUST;
			}

			if(pipeline.sensors_flags == PIPELINE_SENSORS_ALL) {
				//SEND TO CM4
			}

		}



		vTaskDelayUntil( &last_wake_time, period );
	}
}



/* END */


