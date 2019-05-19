#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include <driver/spi_master.h>
#include <esp_log.h>

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/task.h"


#include "main.h"
#include "msg_type.h"

#include "sdkconfig.h" // generated by "make menuconfig"

#include "bme280.h"

#include "sd_card.h"

#define SDA_PIN 25 // GPIO_NUM_15
#define SCL_PIN 22 // GPIO_NUM_2

#define TAG "MAIN"
#define TAG_BME280 "BME280"

void i2c_master_init()
{
	i2c_config_t i2c_config = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = SDA_PIN,
		.scl_io_num = SCL_PIN,
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master.clk_speed = 1000000
	};
	i2c_param_config(I2C_NUM_0, &i2c_config);
	i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
}

s8 BME280_I2C_bus_write(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 cnt)
{
	s32 iError = BME280_INIT_VALUE;

	esp_err_t espRc;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);

	i2c_master_write_byte(cmd, reg_addr, true);
	i2c_master_write(cmd, reg_data, cnt, true);
	i2c_master_stop(cmd);

	espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
	if (espRc == ESP_OK) {
		iError = SUCCESS;
	} else {
		iError = FAIL;
	}
	i2c_cmd_link_delete(cmd);

	return (s8)iError;
}

s8 BME280_I2C_bus_read(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 cnt)
{
	s32 iError = BME280_INIT_VALUE;
	esp_err_t espRc;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, reg_addr, true);

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_READ, true);

	if (cnt > 1) {
		i2c_master_read(cmd, reg_data, cnt-1, I2C_MASTER_ACK);
	}
	i2c_master_read_byte(cmd, reg_data+cnt-1, I2C_MASTER_NACK);
	i2c_master_stop(cmd);

	espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
	if (espRc == ESP_OK) {
		iError = SUCCESS;
	} else {
		iError = FAIL;
	}

	i2c_cmd_link_delete(cmd);

	return (s8)iError;
}

void BME280_delay_msek(u32 msek)
{
	vTaskDelay(msek/portTICK_PERIOD_MS);
}

void task_bme280_normal_mode(void *pvParameters)
{
  int tmp = *((int * )pvParameters);
  ESP_LOGI(TAG_BME280, "Parameter is :%d", tmp );
	struct bme280_t bme280 = {
		.bus_write = BME280_I2C_bus_write,
		.bus_read = BME280_I2C_bus_read,
		.dev_addr = BME280_I2C_ADDRESS1,
		.delay_msec = BME280_delay_msek
	};

	s32 com_rslt;
	s32 v_uncomp_pressure_s32;
	s32 v_uncomp_temperature_s32;
	s32 v_uncomp_humidity_s32;

	com_rslt = bme280_init(&bme280);

	com_rslt += bme280_set_oversamp_pressure(BME280_OVERSAMP_16X);
	com_rslt += bme280_set_oversamp_temperature(BME280_OVERSAMP_2X);
	com_rslt += bme280_set_oversamp_humidity(BME280_OVERSAMP_1X);

	com_rslt += bme280_set_standby_durn(BME280_STANDBY_TIME_1_MS);
	com_rslt += bme280_set_filter(BME280_FILTER_COEFF_16);

	com_rslt += bme280_set_power_mode(BME280_NORMAL_MODE);
	if (com_rslt == SUCCESS) {
	  bInfo.semaphore_bme280 = xSemaphoreCreateBinary();
      xSemaphoreGive(bInfo.semaphore_bme280); // I have to define the semaphore is given clearly
      while( (*((int *)pvParameters) ) == 1) {
        vTaskDelay(100/portTICK_PERIOD_MS);

        com_rslt = bme280_read_uncomp_pressure_temperature_humidity(
            &v_uncomp_pressure_s32, &v_uncomp_temperature_s32, &v_uncomp_humidity_s32);

        if (com_rslt == SUCCESS) {
//				ESP_LOGI(TAG_BME280, "%.2f degC / %.3f hPa / %.3f %%",
//					bme280_compensate_temperature_double(v_uncomp_temperature_s32),
//					bme280_compensate_pressure_double(v_uncomp_pressure_s32)/100, // Pa -> hPa
//					bme280_compensate_humidity_double(v_uncomp_humidity_s32));
          if(xSemaphoreTake(bInfo.semaphore_bme280, 3000/portTICK_RATE_MS) != pdTRUE) {
            ESP_LOGW(TAG_BME280, "bme280 task can not get bInfo semaphore more then 3 seconds!");
            continue;
          }
          bInfo.temperature = bme280_compensate_temperature_double(v_uncomp_temperature_s32);
          bInfo.pressure = bme280_compensate_pressure_double(v_uncomp_pressure_s32)/100;
          bInfo.humidity = bme280_compensate_humidity_double(v_uncomp_humidity_s32);
          xSemaphoreGive(bInfo.semaphore_bme280);
        } else {
            ESP_LOGE(TAG_BME280, "measure error. code: %d", com_rslt);
        }
      }
	} else {
		ESP_LOGE(TAG_BME280, "init or setting error. code: %d", com_rslt);
	}

	ESP_LOGI(TAG_BME280, "bme280 Task get terminated");
	vTaskDelete(NULL);
}

void task_bme280_forced_mode(void *ignore) {
	struct bme280_t bme280 = {
		.bus_write = BME280_I2C_bus_write,
		.bus_read = BME280_I2C_bus_read,
		.dev_addr = BME280_I2C_ADDRESS1,
		.delay_msec = BME280_delay_msek
	};

	s32 com_rslt;
	s32 v_uncomp_pressure_s32;
	s32 v_uncomp_temperature_s32;
	s32 v_uncomp_humidity_s32;

	com_rslt = bme280_init(&bme280);

	com_rslt += bme280_set_oversamp_pressure(BME280_OVERSAMP_1X);
	com_rslt += bme280_set_oversamp_temperature(BME280_OVERSAMP_1X);
	com_rslt += bme280_set_oversamp_humidity(BME280_OVERSAMP_1X);

	com_rslt += bme280_set_filter(BME280_FILTER_COEFF_OFF);
	if (com_rslt == SUCCESS) {
		while(true) {
			com_rslt = bme280_get_forced_uncomp_pressure_temperature_humidity(
				&v_uncomp_pressure_s32, &v_uncomp_temperature_s32, &v_uncomp_humidity_s32);

			if (com_rslt == SUCCESS) {
				ESP_LOGI(TAG_BME280, "%.2f degC / %.3f hPa / %.3f %%",
					bme280_compensate_temperature_double(v_uncomp_temperature_s32),
					bme280_compensate_pressure_double(v_uncomp_pressure_s32)/100, // Pa -> hPa
					bme280_compensate_humidity_double(v_uncomp_humidity_s32));
			} else {
				ESP_LOGE(TAG_BME280, "measure error. code: %d", com_rslt);
			}
		}
	} else {
		ESP_LOGE(TAG_BME280, "init or setting error. code: %d", com_rslt);
	}

	vTaskDelete(NULL);
}

//static char tag[] = "mpcd";

extern void sd_card_task(void *pvParameters);
//extern void task_scroll_text(void *ignore);
extern void gps_clock_task(void *pvParameters);
extern void tsk_disp_temp(void *pvParameters);
extern void task_disp_gps(void *pvParameters);
extern void task_disp_speed(void *pvParameters);
extern void task_disp_timedate(void *pvParameters);
// On board LED for Blink is on GPIO 21
#define LCD_LIGHT 5 // LCD Back Light
#define BTN 19      // Button on front panel

// TODO: Functions define how to stop a certain display mode for preparing to start
// the next, or do other things, like make a gps coordinate snapshot.
// TODO: Stop diplaying temprature/Humidity
void stop_mode_temp(void) {
  ESP_LOGI(TAG, "stop_mode_temp");
  loopholder_bme280 = 0; // Here I stop temperature reading Task
  loopholder_display = 0;// Here I stop Display Task
}

// Stop displaying GPS detailed information
void stop_mode_gps_detail(void) {
  ESP_LOGI(TAG, "stop_mode_gps_detail");
  loopholder_display = 0;// Here I stop Display Task
}

// Stop displaying gps speed
void stop_mode_speed(void) {
  ESP_LOGI(TAG, "stop_mode_speed");
  loopholder_display = 0;
}

void stop_mode_timedate(void) {
  ESP_LOGI(TAG, "stop_mode_datetime");
  loopholder_display = 0;
}

// Start mode display temperature/Humidity
void start_mode_temp(void) {
  ESP_LOGI(TAG, "start_mode_temp");
  loopholder_bme280 = 1;
  loopholder_display = 1;
  xTaskCreate(&task_bme280_normal_mode, "bme280_normal_mode",  2048, &loopholder_bme280, 6, NULL);
  xTaskCreate(&tsk_disp_temp, "task_display_info", 8048, &loopholder_display, 5, NULL);
}

// Start mode display gps information
void start_mode_gps_detail(void) {
  ESP_LOGI(TAG, "start_mode_gps_detail");
  loopholder_display = 1;
  xTaskCreate(&task_disp_gps, "task_disp_gps", 8048, &loopholder_display, 5, NULL);
}

void start_mode_speed(void) {
  ESP_LOGI(TAG, "start_mode_speed");
  loopholder_display = 1;
  xTaskCreate(&task_disp_speed, "task_disp_speed", 8048, &loopholder_display, 5, NULL);
}

void start_mode_timedate(void) {
  ESP_LOGI(TAG, "start_mode_speed");
  loopholder_display = 1;
  xTaskCreate(&task_disp_timedate, "task_disp_timedate", 8048, &loopholder_display, 5, NULL);
}

void app_main(void)
{
	//xTaskCreatePinnedToCore(&task_scroll_text, "task_simple_tests_pcd8544", 8048, NULL, 5, NULL, 0);
  i2c_master_init();
//  TaskHandle_t xHandle_bme280;
  loopholder_bme280 = 1;
  loopholder_display = 1;
  coordinateSaveSucceed = true;
  snapshotFlag = false;
  gps = pvPortMalloc(sizeof(gps_t)); // define this gps object
  // Tipp: xTaskCreate( vTaskCode, "NAME", STACK_SIZE, NULL, tskIDLE_PRIORITY, &xHandle );
  xTaskCreate(&task_bme280_normal_mode, "bme280_normal_mode",  2048, &loopholder_bme280, 6, NULL);
  xTaskCreate(gps_clock_task, "task_gps", 4096, NULL, 5, NULL);
  xTaskCreate(&tsk_disp_temp, "tsk_disp_temp", 8048, &loopholder_display, 5, NULL);
  dMode = TEMPERATUREANDHUMDIDITY;

  gpio_pad_select_gpio(LCD_LIGHT);
  gpio_set_direction(LCD_LIGHT, GPIO_MODE_OUTPUT);
  gpio_set_level(LCD_LIGHT, 1);

  for(int i=0; i<sizeof(taskEndedSemaphoreArr)/sizeof(taskEndedSemaphoreArr[0]);
      i++) {
    taskEndedSemaphoreArr[i] = xSemaphoreCreateBinary();
  }
  sdTskEndedSemaphore = xSemaphoreCreateBinary();

  stopCertainMode[0] = &stop_mode_temp;
  stopCertainMode[1] = &stop_mode_gps_detail;
  stopCertainMode[2] = &stop_mode_speed;
  stopCertainMode[3] = &stop_mode_timedate;
  startCertainMode[0] = &start_mode_temp;
  startCertainMode[1] = &start_mode_gps_detail;
  startCertainMode[2] = &start_mode_speed;
  startCertainMode[3] = &start_mode_timedate;

  // TODO: Would it better, if we wanna pack this in "btn_task"?
  // Button read
  gpio_pad_select_gpio(BTN);
  gpio_set_direction(BTN, GPIO_MODE_INPUT);
  gpio_pad_select_gpio(LCD_LIGHT);
  gpio_set_direction(LCD_LIGHT, GPIO_MODE_OUTPUT);
  int btn_old = 0;
  int btn_now = 0;
  uint32_t btn_long_press_old = 0;
  // Flag marking if btn press a long press to preventing long press trigger a
  // short btn press event either.
  int longPressFlag = false;
  while(1) {
    btn_now = gpio_get_level(BTN);
    if(btn_now != btn_old){
//      ESP_LOGI(TAG, "Get New State");
      if(btn_now == 1){ // btn pressed down
        ESP_LOGI(TAG, "BTN get pressed down.");
        btn_long_press_old = xTaskGetTickCount();
        longPressFlag = false;
      } else { // btn released
        ESP_LOGI(TAG, "BTN get released.");
        // When the button get released in less then 1 second, do the gps coordinate snapshot
        if(abs(xTaskGetTickCount()-btn_long_press_old) < 1000/portTICK_PERIOD_MS
            && !longPressFlag){ // time to make a gps snapshot
          ESP_LOGI(TAG, "Triggered a snapshot");
          snapshotFlag = true;
          // Trigger a snapshot.
          // Stop the right now mode (Kill tasks that should be stopped)
          (*stopCertainMode[dMode])();
          if(xSemaphoreTake(taskEndedSemaphoreArr[dMode], 3000/portTICK_RATE_MS) != pdTRUE) {
            ESP_LOGW(TAG, "Wait for other task finish there job timeout!");
          }
          xTaskCreate(&sd_card_task, "sd_card_tsk",  4096, NULL, 6, NULL);
          // Trigger the SD Card task
          if(xSemaphoreTake(sdTskEndedSemaphore, 3000/portTICK_RATE_MS) != pdTRUE) {
            ESP_LOGW(TAG, "Wait for sd card I/O task finish its job timeout!");
          }
          // Restart the old task where we stopped
          (*startCertainMode[dMode])();
        }
      }
      btn_old = btn_now;
      btn_long_press_old = xTaskGetTickCount();
    }
    else if(btn_now == 1) { // Check if this is a long press btn event
//      ESP_LOGI(TAG, "newTick: %lu, oldTick: %lu, check resolution: %lu millisec.",
//          (unsigned long)xTaskGetTickCount(), (unsigned long)btn_long_press_old,
//          (unsigned long)(1000/portTICK_PERIOD_MS)
//          );

      // When the button get released after more then 1 second, then go to the next display mode.
      if(abs(xTaskGetTickCount()-btn_long_press_old) > 1000/portTICK_PERIOD_MS){
        ESP_LOGI(TAG, "long btn press get triggered");
        (*stopCertainMode[dMode])();
        if(xSemaphoreTake(taskEndedSemaphoreArr[dMode], 3000/portTICK_RATE_MS) != pdTRUE) {
          ESP_LOGW(TAG, "Wait for other task finish there job timeout!");
        }
        if(dMode == 3) {
          dMode = 0;
        } else {
          dMode++;
        }
        (*startCertainMode[dMode])();
        btn_long_press_old=xTaskGetTickCount(); // Update timer preparing trigger the next long btn press event
        longPressFlag = true;
      }
    }
    else {
//      ESP_LOGI(TAG, "Button stays unpressed.");
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  printf("the damme esp32's main reach end!\n");
}
