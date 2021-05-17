/*  Title       : Pipe
 *  Filename    : pipe.h
 *  Author      : iacopo sprenger
 *  Date        : 06.05.2021
 *  Version     : 0.1
 *  Description : data pipe
 */

#ifndef PIPE_H
#define PIPE_H



/**********************
 *  INCLUDES
 **********************/

#include <stdint.h>
#include <cmsis_os.h>

/**********************
 *  CONSTANTS
 **********************/

#define PIPE_MAX_SUBSCRIBERS	16

/**********************
 *  MACROS
 **********************/


/**********************
 *  TYPEDEFS
 **********************/



typedef enum PIPE_ERROR {
	PIPE_SUCCESS,
	PIPE_BUSY,
	PIPE_MAX_SUB,
	PIPE_ERROR
}PIPE_ERROR_t;

typedef struct PIPE_INST {
	uint32_t id;
	void * data;
	uint32_t data_len;
	void (*subscribers[PIPE_MAX_SUBSCRIBERS]) (void *);
	uint16_t subscribers_len;
	SemaphoreHandle_t mutex;
	StaticSemaphore_t mutex_buffer;
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

void pipe_global_init(void);

PIPE_ERROR_t pipe_init(PIPE_INST_t * pipe, uint32_t data_len);

PIPE_ERROR_t pipe_subscribe(PIPE_INST_t * pipe, void (*callback) (void *));

PIPE_ERROR_t pipe_publish(PIPE_INST_t * pipe, void * data);


#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */

#endif /* PIPE_H */

/* END */
