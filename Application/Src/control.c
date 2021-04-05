/*  Title		: Control
 *  Filename	: control.c
 *	Author		: iacopo sprenger
 *	Date		: 20.01.2021
 *	Version		: 0.1
 *	Description	: main program control
 */

/**********************
 *	INCLUDES
 **********************/
#include <main.h>
#include <cmsis_os.h>
#include <control.h>
#include <led.h>
#include <storage.h>
#include <can_comm.h>
#include <servo.h>

/**********************
 *	CONFIGURATION
 **********************/

#define CONTROL_HEART_BEAT	50 /* ms */



/**********************
 *	CONSTANTS
 **********************/

#define TIME_TOL (CONTROL_HEART_BEAT/2)


#define TARGET_REACHED_DELAY_CYCLES	(50)

#define SCHED_ALLOWED_WIDTH	(6)

#define CONTROL_SAVE_DELAY	(5000)

/**********************
 *	MACROS
 **********************/

/**********************
 *	TYPEDEFS
 **********************/



/**********************
 *	VARIABLES
 **********************/

static CONTROL_INST_t control;


//Authorisations table
static CONTROL_SCHED_t sched_allowed[][SCHED_ALLOWED_WIDTH] = {
		{CONTROL_SCHED_ABORT, CONTROL_SCHED_MOVE_TVC, CONTROL_SCHED_NOTHING, CONTROL_SCHED_NOTHING, CONTROL_SCHED_NOTHING, CONTROL_SCHED_NOTHING}, 	//IDLE
		{CONTROL_SCHED_ABORT, CONTROL_SCHED_MOVE_TVC, CONTROL_SCHED_NOTHING, CONTROL_SCHED_NOTHING, CONTROL_SCHED_NOTHING, CONTROL_SCHED_NOTHING}, 	//BOOT
		{CONTROL_SCHED_ABORT, CONTROL_SCHED_MOVE_TVC, CONTROL_SCHED_NOTHING, CONTROL_SCHED_NOTHING, CONTROL_SCHED_NOTHING, CONTROL_SCHED_NOTHING}, 	//COMPUTE
		{CONTROL_SCHED_ABORT, CONTROL_SCHED_MOVE_TVC, CONTROL_SCHED_NOTHING, CONTROL_SCHED_NOTHING, CONTROL_SCHED_NOTHING, CONTROL_SCHED_NOTHING}, 	//ABORT
		{CONTROL_SCHED_ABORT, CONTROL_SCHED_MOVE_TVC, CONTROL_SCHED_NOTHING, CONTROL_SCHED_NOTHING, CONTROL_SCHED_NOTHING, CONTROL_SCHED_NOTHING} 	//ERROR
};

/**********************
 *	PROTOTYPES
 **********************/
static void init_control(CONTROL_INST_t * control);
static void control_update(CONTROL_INST_t * control);

// Enter state functions
static void init_idle(CONTROL_INST_t * control);
static void init_boot(CONTROL_INST_t * control);
static void init_compute(CONTROL_INST_t * control);
static void init_abort(CONTROL_INST_t * control);
static void init_error(CONTROL_INST_t * control);

// Main state functions
static void idle(CONTROL_INST_t * control);
static void boot(CONTROL_INST_t * control);
static void compute(CONTROL_INST_t * control);
static void _abort(CONTROL_INST_t * control);
static void error(CONTROL_INST_t * control);

//scheduling

static uint8_t control_sched_should_run(CONTROL_INST_t * control, CONTROL_SCHED_t num);
static void control_sched_done(CONTROL_INST_t * control, CONTROL_SCHED_t num);
static void control_sched_set(CONTROL_INST_t * control, CONTROL_SCHED_t num);


static void (*control_fcn[])(CONTROL_INST_t *) = {
		idle,
		boot,
		compute,
		_abort,
		error
};


/**********************
 *	DECLARATIONS
 **********************/

void control_thread(void * arg) {

	static TickType_t last_wake_time;
	static const TickType_t period = pdMS_TO_TICKS(CONTROL_HEART_BEAT);



	led_init();

	init_control(&control);

	static SERVO_INST_t tvc_servo;

	servo_global_init();

	servo_init(&tvc_servo, 1);

	servo_config(&tvc_servo);

	control.tvc_servo = &tvc_servo;


	init_idle(&control);



	last_wake_time = xTaskGetTickCount();


	for(;;) {


		static uint8_t lol = 0;
		static uint16_t cnt = 0;
		if(cnt++ > 10) {
			lol = !lol;
			cnt = 0;
		}

		if(lol) {
			servo_enable_led(control.tvc_servo, NULL);
		} else {
			servo_disable_led(control.tvc_servo, NULL);
		}


		control_update(&control);


		if(control.state < CS_NUM && control.state > 0) {
			control_fcn[control.state](&control);
		}
		vTaskDelayUntil( &last_wake_time, period );
	}
}


static void control_update(CONTROL_INST_t * control) {

	control->last_time = control->time;
	control->time = HAL_GetTick();
	control->iter++;

	if(control->counter_active) {
		control->counter -= (control->time - control->last_time);
	}

	while(can_msgPending()) {
		control->msg = can_readBuffer();

		if(control->msg.id == DATA_ID_COMMAND){

		}
	}

	//read servo parameters
	servo_sync(control->tvc_servo);


	//init error if there is an issue with a motor

	if(control_sched_should_run(control, CONTROL_SCHED_ABORT)) {
		init_abort(control);
		control_sched_done(control, CONTROL_SCHED_ABORT);
	}
}

static void init_control(CONTROL_INST_t * control) {
	control->sched = CONTROL_SCHED_NOTHING;
	control->counter_active = 0;
}

static void init_idle(CONTROL_INST_t * control) {
	control->state = CS_IDLE;
	led_set_color(LED_GREEN);
	storage_disable();
}

static void idle(CONTROL_INST_t * control) {
	if(control_sched_should_run(control, CONTROL_SCHED_MOVE_TVC)) {
		servo_move(control->tvc_servo, control->tvc_mov_target);
		control_sched_done(control, CONTROL_SCHED_MOVE_TVC);
	}

}

static void init_boot(CONTROL_INST_t * control) {
	//global enable
	//to boot the rpi
}

static void boot(CONTROL_INST_t * control) {
	//wait for the cm4 to start answering with concluent status packets
}

static void init_compute(CONTROL_INST_t * control) {
	//start sending data to raspberry pi
}

static void compute(CONTROL_INST_t * control) {

}

static void init_abort(CONTROL_INST_t * control) {
	led_set_color(LED_PINK);
	control->state = CS_ABORT;
	servo_move(control->tvc_servo, 2048); //2048 is the straight position
	control->counter_active=0;
	storage_disable();
}

static void _abort(CONTROL_INST_t * control) {
	if(control_sched_should_run(control, CONTROL_SCHED_RECOVER)) {
		init_idle(control);
		control_sched_done(control, CONTROL_SCHED_RECOVER);
	}
}

static void init_error(CONTROL_INST_t * control) {
	led_set_color(LED_RED);
	control->state = CS_ERROR;
	control->counter_active = 0;
	storage_disable();
}

static void error(CONTROL_INST_t * control) {
	//wait for user release
	if(control_sched_should_run(control, CONTROL_SCHED_RECOVER)) {
		init_idle(control);
		control_sched_done(control, CONTROL_SCHED_RECOVER);
	}
}

CONTROL_STATE_t control_get_state() {
	return control.state;
}


void control_move_tvc(int32_t target) {
	control_sched_set(&control, CONTROL_SCHED_MOVE_TVC);
	control.tvc_mov_target = target;
}

void control_boot(void) {

}

void control_abort() {
	control_sched_set(&control, CONTROL_SCHED_ABORT);
}

void control_recover() {
	control_sched_set(&control, CONTROL_SCHED_RECOVER);
}

CONTROL_STATUS_t control_get_status() {
	CONTROL_STATUS_t status = {0};
	status.tvc_error = control.tvc_servo->error;
	status.tvc_psu_voltage = control.tvc_servo->psu_voltage;
	status.tvc_temperature = control.tvc_servo->temperature;
	status.tvc_position = control.tvc_servo->position;

	return status;
}


static uint8_t control_sched_should_run(CONTROL_INST_t * control, CONTROL_SCHED_t num) {
	return control->sched == num;
}

static void control_sched_done(CONTROL_INST_t * control, CONTROL_SCHED_t num) {
	if(control->sched == num) {
		control->sched = CONTROL_SCHED_NOTHING;
	} else {
		init_error(control);
	}
}

static void control_sched_set(CONTROL_INST_t * control, CONTROL_SCHED_t num) {
	if(control->sched == CONTROL_SCHED_NOTHING) {
		for(uint8_t i = 0; i < SCHED_ALLOWED_WIDTH; i++) {
			if(sched_allowed[control->state][i] == num) {
				control->sched = num;
				return;
			}
		}
	}
}



/* END */


