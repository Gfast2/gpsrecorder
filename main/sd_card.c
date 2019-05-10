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
  FILE* f = fopen("/sdcard/gps.txt", "a");
  if (f == NULL) {
      ESP_LOGE(TAG, "Failed to open file for writing");
      return;
  }
//  fprintf(f, "Hello %s!\n", card->cid.name);
  // TODO: Write gps information: time-latitude-longitude
  fprintf(f, "%d-%d-%dT%d:%d:%d:%d@%f,%f\n",
      gps->year, gps->month,gps->day,
      gps->hour, gps->minute,gps->second, gps->microseconds,
      gps->latitude, gps->longitude);
  fclose(f);
  ESP_LOGI(TAG, "gps file updated");

  // Check if destination file exists before renaming
//  struct stat st;
//  if (stat("/sdcard/foo.txt", &st) == 0) {
//      // Delete it if it exists
//      unlink("/sdcard/foo.txt");
//  }
//
//  // Rename original file
//  ESP_LOGI(TAG, "Renaming file");
//  if (rename("/sdcard/hello.txt", "/sdcard/foo.txt") != 0) {
//      ESP_LOGE(TAG, "Rename failed");
//      return;
//  }

  // Open renamed file for reading
  ESP_LOGI(TAG, "Reading file");
  f = fopen("/sdcard/gps.txt", "r");
  if (f == NULL) {
      ESP_LOGE(TAG, "Failed to open file for reading");
      return;
  }
//  char line[64];
//  fgets(line, sizeof(line), f);
//  fclose(f);
  // strip newline
//  char* pos = strchr(line, '\n');
//  if (pos) {
//      *pos = '\0';
//  }

  // Read the last line content
  // Each line will have max. 45 Characters till 2019-5-10, so define a 55 c Buffer
  static const long max_len = 55+ 1;  // define the max length of the line to read
  char buff[max_len + 1];             // define the buffer and allocate the length
  fseek(f, -max_len, SEEK_END);            // set pointer to the end of file minus the length you need. Presumably there can be more than one new line character
  fread(buff, max_len-1, 1, f);            // read the contents of the file starting from where fseek() positioned us
  fclose(f);                               // close the file
  buff[max_len-1] = '\0';                   // close the string
  char *last_newline = strrchr(buff, '\n'); // find last occurrence of newlinw
  char *last_line = last_newline+1;         // jump to it

//  printf("captured: [%s]\n", last_line);    // captured: [472977827]


  ESP_LOGI(TAG, "Read from file: '%s'", last_line);

  // All done, unmount partition and disable SDMMC or SPI peripheral
  esp_vfs_fat_sdmmc_unmount();
  ESP_LOGI(TAG, "Card unmounted");
  ESP_LOGI(TAG, "Now Sd card finish its job!");
  xSemaphoreGive(sdTskEndedSemaphore);
  vTaskDelete(NULL);
}
