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

// This is how to call a function from C in C++
extern "C" {
  void sd_card_task(void *pvParameters);
}

//extern void sd_card_task(void *pvParameters);

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
  display.write('\n');
}

static int min(int a, int b) {
	if (a < b) {
		return a;
	}
	return b;
}
extern "C" {
	void task_test_pcd8544(void *pvParameters);
}

void task_display_info(void *pvParameters) {
	ESP_LOGW(TAG, "task 'task_display_info'started");
	bme280Info bInfo;
//	xTaskCreate(&sd_card_task, "task_sd_card", 8048, NULL, 5, NULL);
	while(*( (int *)pvParameters ) == 1) {
		  if(xQueueReceive(bme280_queue, (void *)(&bInfo), (TickType_t)0) == pdTRUE) {
		  display.clearDisplay();
		  display.setTextSize(1);
		  display.setTextColor(BLACK);
		  print((char *)"Temperature:");
		  display.setTextSize(2);
		  display.setTextColor(WHITE, BLACK);
		  char buffer[12];
		  int ret = snprintf(buffer, sizeof buffer, "%.2fC", bInfo.temperature);
		  print(buffer);
		  display.setTextSize(1);
		  display.setTextColor(BLACK);
		  print((char *)"Humidity:");
		  display.setTextSize(2);
		  display.setTextColor(WHITE, BLACK);
		  ret = snprintf(buffer, sizeof buffer, "%.2f%%", bInfo.humidity);
		  print(buffer);
		  display.display();

//		  ESP_LOGW(tag, "%.2f degC / %.3f hPa / %.3f %%",
//	 					bInfo.temperature,
//	 					bInfo.pressure/100, // Pa -> hPa
//	 					bInfo.humidity);
		  }
	}
 ESP_LOGI(TAG, "Try to stop Display & its SPI Bus!");
 // TODO: Here I should let display.stop() return some result infos, in order to
 // decide if or not do following steps, like if or not should I call SD card task
 display.stop();
 xTaskCreate(&sd_card_task, "sd_card_task", 8048, NULL, 5, NULL);
 ESP_LOGI(TAG, "Display Info Task get deleted");
 vTaskDelete(NULL);
}

void task_test_pcd8544(void *pvParameters)   {
  display.begin();
  display.setContrast(60);
  display.display();
  delay(250); // Splash Photo
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextColor(WHITE, BLACK); // 'inverted' text
  display.setRotation(2);  // rotate 90 degrees counter clockwise, can also use values of 2 and 3 to go further.
  display.setTextSize(2);
  ESP_LOGI(TAG, "Display Initialization Task All done!");

  // trigger another task and let it pull the information later.
  xTaskCreate(&task_display_info, "task_display_info", 8048, pvParameters, 5, NULL);
  ESP_LOGW(TAG, "Display init task ended");
  // TODO: Finish this task
  vTaskDelete(NULL);
}
