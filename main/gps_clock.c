/* Copyright (c) 2017 pcbreflux. All Rights Reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>. *
 */
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
//#include "freertos/heap_regions.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_heap_alloc_caps.h"

#include "gps_clock.h"

#include "driver/uart.h"
#include "minmea.h"
#include "msg_type.h"
#include "main.h"


#define TAG "GPS"

#define GPS_TX_PIN 23 // 17
#define GPS_RX_PIN 18 // 16
#define GPS_UART UART_NUM_1
#define GPS_UART_BUFFER_SIZE 2048

#define MAX_LINE_SIZE 255

void readLine(uart_port_t uart, uint8_t *line) {
  int size;
  uint8_t *ptr = line;
  while(ptr-line < MAX_LINE_SIZE) {
    size = uart_read_bytes(uart, ptr, 1, portMAX_DELAY);
    if (size == 1) {
      if (*ptr == '\n') {
        ptr++;
        *ptr = 0;
        break;
      }
      ptr++;
    }
  }
}


void initUART(uart_port_t uart) {
  ESP_LOGI(TAG, "init UART");

  uart_config_t myUartConfig;
  myUartConfig.baud_rate           = 9600;
  myUartConfig.data_bits           = UART_DATA_8_BITS;
  myUartConfig.parity              = UART_PARITY_DISABLE;
  myUartConfig.stop_bits           = UART_STOP_BITS_1;
  myUartConfig.flow_ctrl           = UART_HW_FLOWCTRL_DISABLE;
  myUartConfig.rx_flow_ctrl_thresh = 122;

  uart_param_config(uart, &myUartConfig);

  uart_set_pin(uart,
          GPS_RX_PIN,         // TX
          GPS_TX_PIN,         // RX
          UART_PIN_NO_CHANGE, // RTS
          UART_PIN_NO_CHANGE  // CTS
);

  uart_driver_install(uart, GPS_UART_BUFFER_SIZE, GPS_UART_BUFFER_SIZE, 0, NULL, 0);
}

void deinitUART(uart_port_t uart) {
	ESP_LOGI(TAG, "deinit UART");

	uart_driver_delete(uart);
}

//float getFloat(uint32_t value, uint32_t scale){
float getFloat(struct minmea_float mf) {
  if (mf.scale == 0) {
    return 0.0f;
  } else {
    return (mf.value / mf.scale);
  }
}

void gps_clock_task(void *pvParameters) {
  struct timeval tv;
  struct tm mytm;
  uint8_t line[MAX_LINE_SIZE+1];
  uint8_t notimeset=1;

  initUART(GPS_UART);

  while(notimeset) {
    readLine(GPS_UART,line);
    ESP_LOGD(TAG, "%s", line);
    switch(minmea_sentence_id((char *)line, false)) {
    case MINMEA_SENTENCE_RMC:
      ESP_LOGD(TAG, "Sentence - MINMEA_SENTENCE_RMC");
      struct minmea_sentence_rmc senRMC;

      if (minmea_parse_rmc(&senRMC,(char *)line)) {
        ESP_LOGI(TAG, "time: %d:%d:%d %d-%d-%d",
                senRMC.time.hours, senRMC.time.minutes, senRMC.time.seconds,
                2000+senRMC.date.year,senRMC.date.month,senRMC.date.day);
        mytm.tm_hour = senRMC.time.hours;
        mytm.tm_min = senRMC.time.minutes;
        mytm.tm_sec = senRMC.time.seconds;
        mytm.tm_mday = senRMC.date.day;
        mytm.tm_mon = senRMC.date.month-1;
        mytm.tm_year = 100+senRMC.date.year;

        setenv("TZ", "GMT0GMT0", 1);
        tzset();
        time_t t = mktime(&mytm);
        ESP_LOGI(TAG, "time: %ld",t);
        tv.tv_sec = t;
        tv.tv_usec = 0;
        settimeofday(&tv, NULL); \
        notimeset=0;
      } else {
        ESP_LOGD(TAG, "$xxRMC sentence is not parsed\n");
      }
      break;
    case MINMEA_SENTENCE_GGA:
      ESP_LOGD(TAG, "Sentence - MINMEA_SENTENCE_GGA");
      break;
    case MINMEA_SENTENCE_GSV:
      //ESP_LOGI(TAG, "Sentence - MINMEA_SENTENCE_GSV");
      break;
    case MINMEA_INVALID:
      ESP_LOGD(TAG, "Sentence - INVALID");
      break;
    default:
      ESP_LOGD(TAG, "Sentence - other");
      break;
    }
  }

  ESP_LOGI(TAG, "Time set!");
  ESP_LOGI(TAG, "Start to update GPS Info");
  // This loop the gps task will update the GPS information & waiting for the
  // others to accessing it when needed (protect the IO with semaphore)
  gps->semaphore_gps = xSemaphoreCreateBinary();
  xSemaphoreGive(gps->semaphore_gps); // I have to define the semaphore is given clearly
  while (1) {
    readLine(GPS_UART, line);
//    printf("%s", line); // Log the raw data line
    // When there is a new message comes in, mutate this flag:

    // Try to lock gps object's semaphore, waiting this happens for three sec.
    if(xSemaphoreTake(gps->semaphore_gps, 3000/portTICK_RATE_MS) != pdTRUE) {
      ESP_LOGW(TAG, "Wait for other task free gps object accessing failed, wait"\
          " for next round");
      continue;
    }
    switch (minmea_sentence_id((char *) line, false)) {
    case MINMEA_SENTENCE_RMC: // (recommended minimum data for gps )
      ESP_LOGV(TAG, "Sentence - MINMEA_SENTENCE_RMC");
      // TODO: I think about the situation when too long time can not get signal from GPS to update/sync the time rightnow
      struct minmea_sentence_rmc senRMC;
      if (minmea_parse_rmc(&senRMC, (char *) line)) {
          if (senRMC.date.year > 0) {
              gps->year = 2000 + senRMC.date.year;
              gps->month= senRMC.date.month;
              gps->day  = senRMC.date.day;
              gps->hour = senRMC.time.hours;
              gps->minute = senRMC.time.minutes;
              gps->second = senRMC.time.seconds;
          }
          // TODO:
          // Here I should pick up other info
          // speed (knot)
          // coordinate
          // valid ('A' for active, 'V' for void)
      }
      break;
    case MINMEA_SENTENCE_GGA:
//      ESP_LOGV(TAG, "Sentence - MINMEA_SENTENCE_GGA");
      ; // <- This thing make the following line works correctlly
      struct minmea_sentence_gga senGGA;
      if (minmea_parse_gga(&senGGA, (char *) line)) {
        gps->altitude = getFloat(senGGA.altitude);
        gps->longitude = minmea_tocoord(&(senGGA.longitude));
        gps->latitude = minmea_tocoord(&(senGGA.latitude));
//        ESP_LOGI(TAG, "lati.: %f, longi.: %f", gps->latitude, gps->longitude);
        gps->height = getFloat(senGGA.height);
        gps->hdop = getFloat(senGGA.hdop);
        gps->fix_quality = senGGA.fix_quality;
        gps->dgps_age = senGGA.dgps_age;
        gps->satellites_tracked = senGGA.satellites_tracked;
        gps->hour = senGGA.time.hours;
        gps->minute = senGGA.time.minutes;
        gps->second = senGGA.time.seconds;
        gps->microseconds = senGGA.time.microseconds;
        gps->altitude_units = senGGA.altitude_units;
        gps->height_units = senGGA.height_units;
      }
      break;
    case MINMEA_SENTENCE_GSV:
      ESP_LOGV(TAG, "Sentence - MINMEA_SENTENCE_GSV");
      struct minmea_sentence_gsv senGSV;
      if (minmea_parse_gsv(&senGSV, (char *) line)) {
        //              senGSV.sats // TODO: satalite infomation
        gps->msg_nr = senGSV.msg_nr;
        gps->total_sats = senGSV.total_sats;
        gps->total_msgs = senGSV.total_msgs;
      }
      break;
    case MINMEA_SENTENCE_GSA:
      ESP_LOGV(TAG, "Sentence - MINMEA_SENTENCE_GSA");
      struct minmea_sentence_gsa senGSA;
      if (minmea_parse_gsa(&senGSA, (char *) line)) {
          gps->mode = senGSA.mode;
          gps->fix_type = senGSA.fix_type;
          memcpy(&(gps->sats), senGSA.sats, 12 * sizeof(int));
          gps->pdop = getFloat(senGSA.pdop);
          gps->hdop = getFloat(senGSA.hdop);
          gps->vdop = getFloat(senGSA.vdop);
      }
      break;
    case MINMEA_SENTENCE_GLL:
        ESP_LOGV(TAG, "Sentence - MINMEA_SENTENCE_GLL")
        ;
        struct minmea_sentence_gll senGLL;
        if (minmea_parse_gll(&senGLL, (char *) line)) {
            gps->longitude = minmea_tocoord(&(senGLL.longitude));
            gps->latitude = minmea_tocoord(&(senGLL.latitude));
            gps->hour = senGLL.time.hours;
            gps->minute = senGLL.time.minutes;
            gps->second = senGLL.time.seconds;
            gps->microseconds = senGLL.time.microseconds;
            gps->status = senGLL.status;
            gps->mode = senGLL.mode;
        }
        break;
    case MINMEA_SENTENCE_GST: // This sentence did not exist for my navi
      ESP_LOGV(TAG, "Sentence - MINMEA_SENTENCE_GST")
      ;
      struct minmea_sentence_gst senGST;
      if (minmea_parse_gst(&senGST, (char *) line)) {
          gps->hour = senGST.time.hours;
          gps->minute = senGST.time.minutes;
          gps->second = senGST.time.seconds;
          gps->microseconds = senGST.time.microseconds;
          gps->rms_deviation = getFloat(senGST.rms_deviation);
          gps->semi_major_deviation = getFloat(
                  senGST.semi_major_deviation);
          gps->semi_minor_deviation = getFloat(
                  senGST.semi_minor_deviation);
          gps->semi_major_orientation = getFloat(
                  senGST.semi_major_orientation);
          gps->latitude_error_deviation = getFloat(
                  senGST.latitude_error_deviation);
          gps->longitude_error_deviation = getFloat(
                  senGST.longitude_error_deviation);
          gps->altitude_error_deviation = getFloat(
                  senGST.altitude_error_deviation);
      }
      break;
    case MINMEA_SENTENCE_VTG:
      ESP_LOGV(TAG, "Sentence - MINMEA_SENTENCE_VTG");
      struct minmea_sentence_vtg senVTG;
      if (minmea_parse_vtg(&senVTG, (char *) line)) {
          gps->true_track_degrees = getFloat(senVTG.true_track_degrees);
          gps->magnetic_track_degrees = getFloat(
                  senVTG.magnetic_track_degrees);
          gps->speed_knots = getFloat(senVTG.speed_knots);
          gps->speed_kph = getFloat(senVTG.speed_kph);
          gps->faa_mode = senVTG.faa_mode;
      }
      break;
    case MINMEA_INVALID:
      ESP_LOGV(TAG, "Sentence - INVALID");
      break;
    case MINMEA_UNKNOWN:
      ESP_LOGV(TAG, "%s", line);
      ESP_LOGV(TAG, "Sentence - other");
      break;
    default:
      ESP_LOGV(TAG, "Sentence - other");
      break;
    }
    // release gps object's semaphore
    xSemaphoreGive(gps->semaphore_gps);
//      ESP_LOGI(TAG, "HHHHHHHHHHHHHHHHHHHHHHHHHHHH");
//      ESP_LOGI(TAG, "HHH satellites track: %d HHH", gps->satellites_tracked);
//      ESP_LOGI(TAG, "HHH height_units    : %s HHH", &(gps->height_units));
//      ESP_LOGI(TAG, "HHH fix_quality     : %d HHH", gps->fix_quality);
//      ESP_LOGI(TAG, "HHH dgps_age        : %d HHH", gps->dgps_age);
//      ESP_LOGI(TAG, "HHH altitude_units  : %s HHH", &(gps->altitude_units));
//      ESP_LOGI(TAG, "HHHHHHH %d:%d:%d:%d HHHHHHHH", gps->hour, gps->minute,gps->second, gps->microseconds);
//      ESP_LOGI(TAG, "HHHHHHH %d-%d-%d HHHHHHHH", gps->year, gps->month,gps->day);
//      ESP_LOGI(TAG, "HHHHHHHHHHHHHHHHHHHHHHHHHHHH");
    }

	ESP_LOGI(TAG, "Stop to update GPS Info & terminal gps_clock task now.");
	deinitUART(GPS_UART);
	gps->semaphore_gps = NULL;
	vTaskDelete(NULL);
}
