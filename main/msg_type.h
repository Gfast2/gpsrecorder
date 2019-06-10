#ifndef MAIN_IOC_110_TYPE_H_
#define MAIN_IOC_110_TYPE_H_

#include <time.h>
#include <sys/time.h>

#define true  1
#define false 0

typedef enum {
	LOW=0,
	HIGH
} pinState;

// One GPS Coordinate record line
#define SD2DISPLAY_BUF 60

typedef struct {
	double temperature;
	double pressure;
	double humidity;
	SemaphoreHandle_t semaphore_bme280;
} bme280Info;

// We need millisecond precision,
// timeval is made of two long data. one works as the time_t,
// the other is a long type number with micro second percision
typedef struct {
	uint8_t GPIO_NUM;
	pinState state;
	struct timeval timeStamp;
} pinInfo;

typedef struct {
	pinInfo io[5];
} allIO;

// data from GPS module
typedef struct {
	int hour;
	int minute;
	int second;
	int microseconds;
	int day;
	int month;
	int year;
	float altitude;
	float longitude;
	float latitude;
	float height;
	int fix_quality;
	int dgps_age;
	int satellites_tracked;
	char altitude_units;
	char height_units;
	int msg_nr;
	int total_sats;
	int total_msgs;
	char mode;
	int fix_type;
	int sats[12];
	float pdop;
	float hdop;
	float vdop;
	char status;
	float rms_deviation;
	float semi_major_deviation;
	float semi_minor_deviation;
	float semi_major_orientation;
	float latitude_error_deviation;
	float longitude_error_deviation;
	float altitude_error_deviation;
	float true_track_degrees;
	float magnetic_track_degrees;
	float speed_knots;
	float speed_kph;
	char faa_mode; // https://www.faa.gov/about/office_org/headquarters_offices/ato/service_units/techops/navservices/gnss/library/factsheets/media/RNAV_QFSheet.pdf
	SemaphoreHandle_t semaphore_gps; // semaphore protect multi task accessing it.
} gps_t;

/*
 * Button state
 */
typedef struct {
	uint8_t stateChange;// if or not is this a state change (toggled), 1-changed, 0-unchanged
	uint8_t state;      // 0-low, 1-hight the debounced button state
	uint8_t accumulator;// debouncing parameter
} buttonState;

// life sign should have nearly whole status of this controller
// perhaps as the redundancy for event-based stuff.
typedef struct {
	uint32_t uptime; // unix time format...
	uint32_t msg_number; // counter for how many data have been sent
	uint32_t msg_timestamp; // timestamp of the message
	pinInfo pin[5]; // status of each pin
} lifesign_t;

typedef struct ipConf_T { // TODO: Figure out how to save the into uint32_t or so, and do the conversion
	char mac[18];
	char ip[16];
	char netmask[16];
	char gateway[16];
	char port[6]; // 16-bit integer, so 0-65535
} ipConf;

// configurations of the device
// This comes from NVS. New settings can be read from SD Card. and it can be re-
// ad / write by web-interface.
typedef struct {
	char serialnumber[50];
	char projectname[50];
	char timestamp[23];	// Timestamp of the change. For setting from sd card, should be the timestamp of the lastchange of the config file
	char firmwareversion[20];   // firmware version
	char firmwarebuildtimestamp[30]; // firmware build timestamp
	ipConf targetIp;
	ipConf myIp;
	uint16_t debouncetime[5]; // debounce time for each gpio
} conf_t;

// Mode the system need to handle
enum displayMode {
  TEMPERATUREANDHUMDIDITY = 0,
  GPS_INFO_DETAIL,
  SPEED_INF,
  DATETIME,
  SAVEDCOORDINATE,
};

#endif /* MAIN_IOC_110_TYPE_H_ */
