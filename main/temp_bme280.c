/*
 * temp_bme280.c
 *
 *  Created on: May 24, 2019
 *      Author: gfast2
 */

#include "driver/i2c.h"
#include "bme280.h"
#include "temp_bme280.h"
#include "main.h"
#include "msg_type.h"
#include "esp_err.h"
#include "esp_log.h"
#include "sdkconfig.h"

#define SDA_PIN 25 // GPIO_NUM_15
#define SCL_PIN 22 // GPIO_NUM_2

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
//              ESP_LOGI(TAG_BME280, "%.2f degC / %.3f hPa / %.3f %%",
//                  bme280_compensate_temperature_double(v_uncomp_temperature_s32),
//                  bme280_compensate_pressure_double(v_uncomp_pressure_s32)/100, // Pa -> hPa
//                  bme280_compensate_humidity_double(v_uncomp_humidity_s32));
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
