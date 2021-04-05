/*  Title		: Debug
 *  Filename	: debug.c
 *	Author		: iacopo sprenger
 *	Date		: 20.01.2021
 *	Version		: 0.1
 *	Description	: debug interface code
 *	What needs to be available:
 *	Access the internal state
 *	Trigger ignition and flight sequence
 *	Trigger calibration
 *	Read sensor data
 *	Read stored data
 *	Configure ignition sequence parameters
 */

/**********************
 *	INCLUDES
 **********************/

#include <debug.h>
#include <usart.h>
#include <control.h>
#include <storage.h>


/**********************
 *	CONSTANTS
 **********************/

#define DEBUG_UART huart3


#define DOWNLOAD_LEN  (4)
#define TVC_MOVE_LEN  (4)



#define ERROR_LO	(0xce)
#define ERROR_HI	(0xec)

#define CRC_ERROR_LO	(0xbe)
#define CRC_ERROR_HI	(0xeb)

#define OK_LO	(0xc5)
#define OK_HI	(0x5c)

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

//debug routines
static void debug_get_status(uint8_t * data, uint16_t data_len, uint8_t * resp, uint16_t * resp_len);
static void debug_boot(uint8_t * data, uint16_t data_len, uint8_t * resp, uint16_t * resp_len);
static void debug_shutdown(uint8_t * data, uint16_t data_len, uint8_t * resp, uint16_t * resp_len);
static void debug_download(uint8_t * data, uint16_t data_len, uint8_t * resp, uint16_t * resp_len);
static void debug_tvc_move(uint8_t * data, uint16_t data_len, uint8_t * resp, uint16_t * resp_len);
static void debug_abort(uint8_t * data, uint16_t data_len, uint8_t * resp, uint16_t * resp_len);
static void debug_recover(uint8_t * data, uint16_t data_len, uint8_t * resp, uint16_t * resp_len);


/**********************
 *	DEBUG FCN ARRAY
 **********************/
static void (*debug_fcn[]) (uint8_t *, uint16_t, uint8_t *, uint16_t *) = {
		debug_get_status,	//0x00
		debug_boot,			//0x01
		debug_shutdown,		//0x02
		debug_download,		//0x03
		debug_tvc_move,		//0x04
		debug_abort,		//0x05
		debug_recover		//0x06
};

static uint16_t debug_fcn_max = sizeof(debug_fcn) / sizeof(void *);

/**********************
 *	DECLARATIONS
 **********************/


//Requires an instance of type debug
SERIAL_RET_t debug_decode_fcn(void * inst, uint8_t data) {
	static uint8_t send_data[MSV2_MAX_DATA_LEN];
	static uint16_t length = 0;
	static uint16_t bin_length = 0;
	DEBUG_INST_t * debug = (DEBUG_INST_t *) inst;
	MSV2_ERROR_t tmp = msv2_decode_fragment(&debug->msv2, data);

	if(tmp == MSV2_SUCCESS) {
		if(debug->msv2.rx.opcode < debug_fcn_max) {

			debug_fcn[debug->msv2.rx.opcode](debug->msv2.rx.data, debug->msv2.rx.length, send_data, &length);
			//length is in words
			bin_length = msv2_create_frame(&debug->msv2, debug->msv2.rx.opcode, length/2, send_data);
			serial_send(&debug->ser, msv2_tx_data(&debug->msv2), bin_length);
		} else {
			send_data[0] = CRC_ERROR_LO;
			send_data[1] = CRC_ERROR_HI;
			length = 2;
			bin_length = msv2_create_frame(&debug->msv2, debug->msv2.rx.opcode, length/2, send_data);
			serial_send(&debug->ser, msv2_tx_data(&debug->msv2), bin_length);
		}
	}

	return tmp;
}

void debug_init(DEBUG_INST_t * debug) {
	static uint32_t id_counter = 0;
	msv2_init(&debug->msv2);
	serial_init(&debug->ser, &DEBUG_UART, debug, debug_decode_fcn);
	debug->id = id_counter++;
}

static void debug_get_status(uint8_t * data, uint16_t data_len, uint8_t * resp, uint16_t * resp_len) {
	CONTROL_STATUS_t status = control_get_status();
	util_encode_u16(resp, status.state);
	util_encode_u16(resp+2, 0); //padding
	util_encode_i32(resp+4, status.counter);
	uint32_t memory = storage_get_used();
	util_encode_u32(resp+8, memory);
	util_encode_i32(resp+12, status.tvc_position);
	util_encode_u16(resp+16, status.tvc_psu_voltage);
	util_encode_u8(resp+18, status.tvc_error);
	util_encode_i8(resp+19, status.tvc_temperature);
	*resp_len = 20;
}

static void debug_boot(uint8_t * data, uint16_t data_len, uint8_t * resp, uint16_t * resp_len) {
	control_boot();
	resp[0] = OK_LO;
	resp[1] = OK_HI;
	*resp_len = 2;
}


static void debug_shutdown(uint8_t * data, uint16_t data_len, uint8_t * resp, uint16_t * resp_len) {
	control_shutdown();
	resp[0] = OK_LO;
	resp[1] = OK_HI;
	*resp_len = 2;
}

static void debug_download(uint8_t * data, uint16_t data_len, uint8_t * resp, uint16_t * resp_len) {
	//downloads 5 samples at a certain location
	if(data_len == DOWNLOAD_LEN) {
		uint32_t location = util_decode_u32(data);
		for(uint8_t i = 0; i < 5; i++) {
			storage_get_sample(location+i, resp+i*32);
		}
		*resp_len = 32*5;
	}
}

static void debug_tvc_move(uint8_t * data, uint16_t data_len, uint8_t * resp, uint16_t * resp_len) {
	if(data_len == TVC_MOVE_LEN) {
		int32_t target = util_decode_i32(data);
		control_move_tvc(target);
		resp[0] = OK_LO;
		resp[1] = OK_HI;
		*resp_len = 2;
	} else {
		resp[0] = ERROR_LO;
		resp[1] = ERROR_HI;
		*resp_len = 2;
	}
}

static void debug_abort(uint8_t * data, uint16_t data_len, uint8_t * resp, uint16_t * resp_len) {
	control_abort();
	resp[0] = OK_LO;
	resp[1] = OK_HI;
	*resp_len = 2;
}

static void debug_recover(uint8_t * data, uint16_t data_len, uint8_t * resp, uint16_t * resp_len) {
	control_recover();
	resp[0] = OK_LO;
	resp[1] = OK_HI;
	*resp_len = 2;
}





/* END */


