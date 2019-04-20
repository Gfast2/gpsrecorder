/*
  sd card control
*/

#include "sd_card.h"

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "sdkconfig.h"
#include "main.h"

static const char *TAG = "SD_CARD";

// Pin mapping when using SPI mode.
// With this mapping, SD card can be used both in SPI and 1-line SD mode.
#define PIN_NUM_MISO 2
#define PIN_NUM_MOSI 15
#define PIN_NUM_CLK  14
#define PIN_NUM_CS   13

extern void task_test_pcd8544(void *pvParameters);

void sd_card_task(void *pvParameters){
  ESP_LOGI(TAG, "Initializing SD card");
  ESP_LOGI(TAG, "Using SPI peripheral");

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
  slot_config.gpio_miso = (gpio_num_t)PIN_NUM_MISO;
  slot_config.gpio_mosi = (gpio_num_t)PIN_NUM_MOSI;
  slot_config.gpio_sck  = (gpio_num_t)PIN_NUM_CLK;
  slot_config.gpio_cs   = (gpio_num_t)PIN_NUM_CS;
  // This initializes the slot without card detect (CD) and write protect (WP) signals.
  // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.

  // Options for mounting the filesystem.
  // If format_if_mount_failed is set to true, SD card will be partitioned and
  // formatted in case when mounting fails.
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = true,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024
  };

  // Use settings defined above to initialize SD card and mount FAT filesystem.
  // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
  // Please check its source code and implement error recovery when developing
  // production applications.
  sdmmc_card_t* card;
  esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

  if (ret != ESP_OK) {
      if (ret == ESP_FAIL) {
          ESP_LOGE(TAG, "Failed to mount filesystem. "
              "If you want the card to be formatted, set format_if_mount_failed = true.");
      } else {
          ESP_LOGE(TAG, "Failed to initialize the card (%s). "
              "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
      }
      return;
  }

  // Card has been initialized, print its properties
  sdmmc_card_print_info(stdout, card);

  // Use POSIX and C standard library functions to work with files.
  // First create a file.
  ESP_LOGI(TAG, "Opening file");
  FILE* f = fopen("/sdcard/hello.txt", "w");
  if (f == NULL) {
      ESP_LOGE(TAG, "Failed to open file for writing");
      return;
  }
  fprintf(f, "Hello %s!\n", card->cid.name);
  fclose(f);
  ESP_LOGI(TAG, "File written");

  // Check if destination file exists before renaming
  struct stat st;
  if (stat("/sdcard/foo.txt", &st) == 0) {
      // Delete it if it exists
      unlink("/sdcard/foo.txt");
  }

  // Rename original file
  ESP_LOGI(TAG, "Renaming file");
  if (rename("/sdcard/hello.txt", "/sdcard/foo.txt") != 0) {
      ESP_LOGE(TAG, "Rename failed");
      return;
  }

  // Open renamed file for reading
  ESP_LOGI(TAG, "Reading file");
  f = fopen("/sdcard/foo.txt", "r");
  if (f == NULL) {
      ESP_LOGE(TAG, "Failed to open file for reading");
      return;
  }
  char line[64];
  fgets(line, sizeof(line), f);
  fclose(f);
  // strip newline
  char* pos = strchr(line, '\n');
  if (pos) {
      *pos = '\0';
  }
  ESP_LOGI(TAG, "Read from file: '%s'", line);

  // All done, unmount partition and disable SDMMC or SPI peripheral
  esp_vfs_fat_sdmmc_unmount();
  ESP_LOGI(TAG, "Card unmounted");

  // TODO: Here I start the bme280 & display task after each other (again)
  loopholder_display = 1; // Revert these loop holder in order to make them working again
  loopholder_bme280 = 1;
  xTaskCreate(&task_test_pcd8544, "task_pcd8544_display", 8048, &loopholder_display, 5, NULL);
  xTaskCreate(&task_bme280_normal_mode, "bme280_normal_mode",  2048, &loopholder_bme280, 6, NULL);
  vTaskDelete(NULL);
}
