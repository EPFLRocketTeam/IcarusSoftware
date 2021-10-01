/*  Title		: Storage
 *  Filename	: storage.c
 *	Author		: iacopo sprenger
 *	Date		: 07.02.2021
 *	Version		: 0.1
 *	Description	: storage on the onboard flash memory
 *
 *
 *	TODO: dual headers in case power is cut while a header is beeing written!
 */

/**********************
 *	INCLUDES
 **********************/

#include <storage.h>
#include <control.h>
#include <flash.h>
#include <led.h>

/**********************
 *	CONSTANTS
 **********************/

#define DATA_SIZE		(32)

#define MAGIC_NUMBER	0xCBE0C5E6
#define HEADER_ADDR		0x00000000
#define USED_SS_ADDR	0x00000004

#define SUBSECTOR_SIZE	4096
#define SAMPLES_PER_SS	(SUBSECTOR_SIZE/DATA_SIZE)

#define NEXT_SUBSECTOR (SUBSECTOR_SIZE*used_subsectors)
#define DATA_START		SUBSECTOR_SIZE
#define NB_SUBSECTOR	4096

#define LONG_TIME		0xffff
#define SS_TO_PREP		500

#define STORAGE_AFTER_SAVE 3000


#define STORAGE_HEART_BEAT 2




/**********************
 *	MACROS
 **********************/

#define ADDRESS(i)		(DATA_START + (i)*DATA_SIZE)
#define SUBSECTOR(i)	(ADDRESS(i)>>12)

/**********************
 *	TYPEDEFS
 **********************/


typedef struct  STORAGE_DATA{
	uint16_t sample_id;
	uint8_t hb_state;
	uint8_t cm4_state;
	int32_t pp_thrust;
	int32_t av_alti;
	int32_t tvc_thrust;
	int32_t tvc_alti;
	int32_t tvc_vel;
	uint32_t padding;
	uint32_t time;
}STORAGE_DATA_t;  //MUST BE AN INTEGER DIVISOR OF 4096


typedef struct STORAGE_HEADER{
	uint32_t magic;
	uint32_t used;
	int32_t calib_1;
	int32_t calib_2;
}STORAGE_HEADER_t;

/**********************
 *	VARIABLES
 **********************/


static uint32_t used_subsectors;
static uint32_t data_counter;
static uint8_t record_active;
static uint8_t restart_required;
static int32_t record_should_stop;

static SemaphoreHandle_t storage_sem = NULL;
static StaticSemaphore_t storage_sem_buffer;


/**********************
 *	PROTOTYPES
 **********************/

static STORAGE_DATA_t read_data(uint32_t address);
static void write_header_used(uint32_t used);

static void write_data(STORAGE_DATA_t data);



/**********************
 *	DECLARATIONS
 **********************/

void storage_init() {
	static STORAGE_HEADER_t header;
	flash_init();
	flash_read(HEADER_ADDR, (uint8_t *) &header, sizeof(STORAGE_HEADER_t));
	if(header.magic == MAGIC_NUMBER) {
		used_subsectors = header.used;
		if(used_subsectors > 1) {
			STORAGE_DATA_t data = {0};
			STORAGE_DATA_t last_valid_data = {0};
			uint32_t count = (used_subsectors-2)*SAMPLES_PER_SS;
			data = read_data(count);
			while(data.sample_id == count){
				last_valid_data = data;
				data = read_data(++count);
			}

			data_counter = count;
		} else {
			data_counter = 0;
		}
	} else {
		write_header_used(1);
		data_counter = 0;
	}
	record_active = 0;
	restart_required = 0;
	record_should_stop = 0;
	storage_sem = xSemaphoreCreateBinaryStatic(&storage_sem_buffer);
}


void storage_record_sample() {
	STORAGE_DATA_t data = {0};

	CONTROL_STATUS_t status = control_get_status();
	CM4_PAYLOAD_COMMAND_t cmd = control_get_cmd();
	CM4_PAYLOAD_FEEDBACK_t fdb = control_get_fdb();
	CM4_PAYLOAD_SENSOR_t sens = control_get_sens();

	data.av_alti = sens.alti;
	data.pp_thrust = fdb.cc_pressure;
	data.tvc_alti = cmd.position[2];
	data.tvc_vel = cmd.speed[2];
	data.hb_state = status.state;
	data.cm4_state = cmd.state;
	data.time = status.time;
	data.tvc_thrust = cmd.thrust;


	write_data(data);

}



static void write_header_used(uint32_t used) {
	static STORAGE_HEADER_t header;
	flash_read(HEADER_ADDR, (uint8_t *) &header, sizeof(STORAGE_HEADER_t));
	flash_erase_subsector(HEADER_ADDR);
	header.magic = MAGIC_NUMBER;
	header.used = used;
	flash_write(HEADER_ADDR, (uint8_t *) &header, sizeof(STORAGE_HEADER_t));
	used_subsectors = used;
}



static STORAGE_DATA_t read_data(uint32_t id) {
	static STORAGE_DATA_t data;
	flash_read(ADDRESS(id), (uint8_t *) &data, sizeof(STORAGE_DATA_t));
	return data;
}

static void write_data(STORAGE_DATA_t data) {
	data.sample_id = data_counter;
	uint32_t addr = ADDRESS(data_counter++);
	if(addr % SUBSECTOR_SIZE == 0) {
		write_header_used(used_subsectors + 1);
		flash_erase_subsector(addr);
	}
	flash_write(addr, (uint8_t *) &data, sizeof(STORAGE_DATA_t));
}

uint32_t storage_get_used() {
	return data_counter;
}

void storage_get_sample(uint32_t id, void * dest) {
	*((STORAGE_DATA_t *)dest) = read_data(id);
}

void storage_enable() {
	record_active = 1;
}

void storage_disable() {
	record_should_stop = STORAGE_AFTER_SAVE;
}

void storage_restart() {
	restart_required = 1;
}

void storage_notify() {
	if(storage_sem != NULL) {
		xSemaphoreGive(storage_sem);
	}
}


void storage_thread(void * arg) {



	storage_init();

	static uint32_t time;
	static uint32_t last_time;




	for(;;) {
		last_time = time;
		time = HAL_GetTick();
		if(restart_required) {
			write_header_used(1);
			data_counter = 0;
			restart_required = 0;
		}
		if(record_should_stop) {
			record_should_stop -= time-last_time;;
			if(record_should_stop<=0){
				record_active=0;
				record_should_stop=0;
			}
		}
		if(xSemaphoreTake(storage_sem, 0xffff) == pdTRUE) {
			if(record_active) {
				storage_record_sample();
			}
		}
	}
}



/* END */


