
#ifndef MAIN_MAIN_H_
#define MAIN_MAIN_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <freertos/queue.h>
#include "string.h"
#include "msg_type.h"

bme280Info bInfo; // temperature, air pressure & humidity info

// 1 -> loop in bme280 task should going on, 0 -> stop loop ASAP
int loopholder_bme280;
// 1 -> loop in "task_display_info" (and other display tasks in general) going
// on, 0 -> stop this loop ASAP
int loopholder_display;

// gps information holder
gps_t * gps;

// binarySemaphore Array to flagging if certain task ended.
SemaphoreHandle_t taskEndedSemaphoreArr [2]; // xSemaphoreCreateBinary();
SemaphoreHandle_t sdTskEndedSemaphore;

// Function pointer array defining "stop right now mode"
// fun_ptr is a pointer to function fun()
//void (*fun_ptr)(int) = &fun;
void(*stopCertainMode[2])(void); // length is the same as "enum displayMode"

// Function pointer array defining "start certain mode"
void(*startCertainMode[2])(void); // length is the same as "enum displayMode"

// Which display mode we are in right now
int dMode;

// Save SD Card failed or succeed, boolean
int coordinateSaveSucceed;

// trigger a snapshot flag, when trigger a snapshot, flip this to true
int snapshotFlag;

// Temperature task
void task_bme280_normal_mode(void *pvParameters); // TODO: This should be moved to its own unit later

#endif
