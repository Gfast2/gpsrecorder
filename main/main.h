
#ifndef MAIN_MAIN_H_
#define MAIN_MAIN_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <freertos/queue.h>
#include "string.h"
#include "msg_type.h"

QueueHandle_t bme280_queue; // button event communication

// 1 -> loop in bme280 task should going on, 0 -> stop loop ASAP
int loopholder_bme280;
// 1 -> loop in "task_display_info" can going on, 0 -> stop this loop ASAP
int loopholder_display;

// gps information holder
gps_t * gps;

// Temperature task
void task_bme280_normal_mode(void *pvParameters); // TODO: This should be moved to its own unit later

#endif
