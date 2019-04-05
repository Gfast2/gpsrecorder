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

//static char tag[] = "mpcd";

extern void task_test_pcd8544(void *ignore);
//extern void task_scroll_text(void *ignore);

// On board LED for Blink is on GPIO 21
#define LCD_LIGHT 5 // LCD Back Light
#define BTN 19      // Button on front panel

void app_main(void)
{
	//xTaskCreatePinnedToCore(&task_scroll_text, "task_simple_tests_pcd8544", 8048, NULL, 5, NULL, 0);
  xTaskCreatePinnedToCore(&task_test_pcd8544, "task_simple_tests_pcd8544", 8048, NULL, 5, NULL, 0);

  // Button read snippet!
//  gpio_pad_select_gpio(BTN);
//  gpio_set_direction(BTN, GPIO_MODE_INPUT);
//  gpio_pad_select_gpio(LCD_LIGHT);
//  gpio_set_direction(LCD_LIGHT, GPIO_MODE_OUTPUT);
//  while(1) {
//	  int result = gpio_get_level(BTN);
//	  printf(". %d\n", result);
//	  vTaskDelay(100 / portTICK_PERIOD_MS);
//  }

  gpio_pad_select_gpio(LCD_LIGHT);
  gpio_set_direction(LCD_LIGHT, GPIO_MODE_OUTPUT);
  while(1) {
	  printf("Turning off the LCD Light\n");
	  gpio_set_level(LCD_LIGHT, 0);
	  vTaskDelay(1000 / portTICK_PERIOD_MS);
	  printf("Turning on the LCD Light\n");
	  gpio_set_level(LCD_LIGHT, 1);
	  vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  printf("Restart the whole damme esp32 again!\n");
}

