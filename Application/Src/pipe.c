/*  Title		: Pipe
 *  Filename	: pipe.c
 *	Author		: iacopo sprenger
 *	Date		: 06.05.2021
 *	Version		: 0.1
 *	Description	: data pipe
 */

/**********************
 *	INCLUDES
 **********************/


#include <pipe.h>
#include <cmsis_os.h>


/**********************
 *	CONSTANTS
 **********************/

#define OSAL_SYS_LOCK()		taskENTER_CRITICAL()
#define OSAL_SYS_UNLOCK()	taskEXIT_CRITICAL()



/**********************
 *	MACROS
 **********************/


/**********************
 *	TYPEDEFS
 **********************/



/**********************
 *	VARIABLES
 **********************/


/**********************
 *	PROTOTYPES
 **********************/


/**********************
 *	DECLARATIONS
 **********************/


void pipe_global_init(void) {

}

PIPE_ERROR_t pipe_init(PIPE_INST_t * pipe, uint32_t data_len) {
	static uint32_t pipe_id = 0;
	pipe->id = pipe_id++;
	pipe->data_len = data_len;
	pipe->subscribers_len = 0;
	pipe->mutex = xSemaphoreCreateMutexStatic(&pipe->mutex_buffer);
	return PIPE_SUCCESS;
}

PIPE_ERROR_t pipe_subscribe(PIPE_INST_t * pipe, void (*callback) (void *)) {
	if(pipe->subscribers_len < PIPE_MAX_SUBSCRIBERS) {
		OSAL_SYS_LOCK();
		pipe->subscribers[pipe->subscribers_len++] = callback;
		OSAL_SYS_UNLOCK();
		return PIPE_SUCCESS;
	} else {
		return PIPE_MAX_SUB;
	}
}

PIPE_ERROR_t pipe_publish(PIPE_INST_t * pipe, void * data) {
	if (xSemaphoreTake(pipe->mutex, 0xffff) == pdTRUE) {
		OSAL_SYS_LOCK();
		uint32_t * _data = (uint32_t *) data;
		for(uint32_t i = 0; i < pipe->data_len>>2; i++) {
			((uint32_t *)pipe->data)[i] = _data[i];
		}
		OSAL_SYS_UNLOCK();
		for(uint32_t i = 0; i < pipe->subscribers_len; i++) {
			pipe->subscribers[i](pipe->data);
		}
		return PIPE_SUCCESS;
	} else {
		return PIPE_BUSY;
	}
}


/* END */


