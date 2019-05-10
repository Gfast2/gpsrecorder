/*********************************************************************
This is an example sketch for our Monochrome Nokia 5110 LCD Displays

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/338

These displays use SPI to communicate, 4 or 5 pins are required to
interface

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
*********************************************************************/
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Adafruit_GFX.h"
#include "Adafruit_PCD8544.h"
#include <driver/spi_master.h>
#include <esp_log.h>
#include "sdkconfig.h"
#include "msg_type.h"
#include "main.h"

// This is how to call a function from C in C++ or reversed
// https://isocpp.org/wiki/faq/mixing-c-and-cpp#call-cpp
extern "C" {
  void sd_card_task(void *pvParameters);
  void tsk_disp_temp(void *pvParameters);
  void task_disp_gps(void *pvParameters);
}

static char TAG [] = "DISPLAY";

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16

Adafruit_PCD8544 display = Adafruit_PCD8544(
	14, // sclk. HSPI related
	13, // MOSI, HSPI related
	27, // D/C
	15,	// CS,   HSPI related
	26  // RST
);

// a bitmap of a 16x16 fruit icon
static const unsigned char logo16_glcd_bmp[]={
0x30, 0xf0, 0xf0, 0xf0, 0xf0, 0x30, 0xf8, 0xbe, 0x9f, 0xff, 0xf8, 0xc0, 0xc0, 0xc0, 0x80, 0x00, 
0x20, 0x3c, 0x3f, 0x3f, 0x1f, 0x19, 0x1f, 0x7b, 0xfb, 0xfe, 0xfe, 0x07, 0x07, 0x07, 0x03, 0x00, };

// static const unsigned char logo16_glcd_bmp[32] = {0};

static void delay(int val) {
	vTaskDelay(val/portTICK_PERIOD_MS);
}

static void print(char *text) {
  while(*text != 0) {
    display.write(*text);
    text++;
  }
}

static void println(char *text) {
  print(text);
  display.write('\n');
}

static int min(int a, int b) {
	if (a < b) {
		return a;
	}
	return b;
}

void tsk_disp_temp(void *pvParameters) {

  display.begin();
  display.setContrast(60);
  display.display();
  delay(250); // Splash Photo
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextColor(WHITE, BLACK); // 'inverted' text
  display.setRotation(2);  // rotate 90 degrees counter clockwise, can also use values of 2 and 3 to go further.
  ESP_LOGI(TAG, "Display Initialization Task All done!");

	ESP_LOGW(TAG, "task 'tsk_disp_temp' started");
    while(*( (int *)pvParameters ) == 1) {
	  if( bInfo.semaphore_bme280 == NULL) {
        ESP_LOGI(TAG, "Waiting for bme280 sensor info comming.");
        continue;
      }
	  if(xSemaphoreTake(bInfo.semaphore_bme280, 3000/portTICK_RATE_MS) != pdTRUE) {
        ESP_LOGW(TAG, "Wait for other task free bme280 object accessing failed, wait"\
            " for next round");
        continue;
      }
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(BLACK);
      println((char *)"Temperature:");
      display.setTextSize(2);
      display.setTextColor(WHITE, BLACK);
      char buffer[12];
      int ret = snprintf(buffer, sizeof buffer, "%.2fC", bInfo.temperature);
      println(buffer);
      display.setTextSize(1);
      display.setTextColor(BLACK);
      println((char *)"Humidity:");
      display.setTextSize(2);
      display.setTextColor(WHITE, BLACK);
      ret = snprintf(buffer, sizeof buffer, "%.2f%%", bInfo.humidity);
      println(buffer);
      display.display();
      xSemaphoreGive(bInfo.semaphore_bme280);
	}
 ESP_LOGI(TAG, "Try to stop Display & its SPI Bus!");
 display.clearDisplay();
 // Here I should let display.stop() return some result infos, in order to
 // decide if or not do following steps, like if or not should I call SD card task
 display.stop();
// bInfo.semaphore_bme280 = NULL;
// xTaskCreate(&sd_card_task, "sd_card_task", 8048, NULL, 5, NULL);
 ESP_LOGI(TAG, "Display Temperature Task get deleted");
 xSemaphoreGive(taskEndedSemaphoreArr[TEMPERATUREANDHUMDIDITY]);
 vTaskDelete(NULL);
}

// Display GPS Informations
void task_disp_gps(void *pvParameters) {

  display.begin(); // parameter is garbage
  display.setContrast(60);
  display.display();
  delay(250); // Splash Photo
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextColor(WHITE, BLACK); // 'inverted' text
  display.setRotation(2);  // rotate 90 degrees counter clockwise, can also use values of 2 and 3 to go further.
  ESP_LOGI(TAG, "Display Initialization Task All done!");


  ESP_LOGI(TAG, "Display GPS information.");
  // TODO: Available satellites
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(BLACK);
  while(*( (int *)pvParameters ) == 1) {
    if(gps->semaphore_gps == NULL) {
      ESP_LOGI(TAG, "Waiting for GPS info comming.");
      continue;
    }
    if(xSemaphoreTake(gps->semaphore_gps, 3000/portTICK_RATE_MS) != pdTRUE) {
      ESP_LOGW(TAG, "Wait for other task free gps object accessing failed, wait"\
          " for next round");
      continue;
    }
    ESP_LOGI(TAG,"GPS Display update!");
    display.clearDisplay();
    print((char *)"Satellites:");
    char buf [25];
    itoa(gps->satellites_tracked, buf, 10);
    println(buf);
    int ret = snprintf(buf, sizeof buf, "==%d-%d-%d==", gps->year, gps->month, gps->day);
    println(buf);
    ret = snprintf(buf, sizeof buf, "+-%d:%d:%d--+", gps->hour, gps->minute, gps->second); // TODO: Handle ret according to results
    println(buf);
    ret = snprintf(buf, sizeof buf, "A.%.2f%s", gps->altitude, &(gps->altitude_units));
    println(buf);
    char dir [1];
    dir[0] = gps->latitude > 0 ? 'N' : 'S';
    ret = snprintf(buf, sizeof buf, "La.%s%.4f", dir, gps->latitude);
    println(buf);
    dir[0] = gps->longitude > 0 ? 'E' : 'W';
    ret = snprintf(buf, sizeof buf, "Lo.%s%.4f", dir, gps->longitude);
    println(buf);
    display.display();
//    ESP_LOGI(TAG, "lati.: %f, longi.: %f", gps->latitude, gps->longitude);
    xSemaphoreGive(gps->semaphore_gps);
    vTaskDelay(1000/portTICK_RATE_MS);
  }
  display.clearDisplay();
  display.stop();
  ESP_LOGI(TAG, "Display GPS information Task terminated.");
  xSemaphoreGive(taskEndedSemaphoreArr[GPS_INFO_DETAIL]);
  vTaskDelete(NULL);
}
