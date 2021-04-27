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

/**********************
 *  CONSTANTS
 **********************/



/**********************
 *  MACROS
 **********************/


/**********************
 *  TYPEDEFS
 **********************/


/**********************
 *  VARIABLES
 **********************/


/**********************
 *  PROTOTYPES
 **********************/

#ifdef __cplusplus
extern "C"{
#endif

void pipeline_thread(void * arg);

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */

#endif /* PIPELINE_H */

/* END */
