/*
 * temp_bme280.h
 *
 *  Created on: May 24, 2019
 *      Author: gfast2
 */
#ifndef BME280_H_
#define BME280_H_

void i2c_master_init();
void task_bme280_normal_mode(void *pvParameters);
void task_bme280_forced_mode(void *ignore);

#endif /* BME280_H_ */
