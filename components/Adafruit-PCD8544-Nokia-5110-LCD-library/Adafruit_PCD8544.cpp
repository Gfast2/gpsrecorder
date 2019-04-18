/*********************************************************************
This is a library for our Monochrome Nokia 5110 LCD Displays

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/338

These displays use SPI to communicate, 4 or 5 pins are required to  
interface

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above, and the splash screen below must be included in any redistribution
*********************************************************************/



#if !defined(_BV)
#define _BV(x) (1 << (x))
#endif // _BV

#include <Adafruit_GFX.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdlib.h>
#include <string.h>

#include "Adafruit_PCD8544.h"
#include "sdkconfig.h"

static char TAG[] = "ADAFRUIT_PCD8544";

// my own test
uint8_t pcd8544_buffer[LCDWIDTH * LCDHEIGHT / 8] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xc0, 0xc0, 0xe0, 0xf0, 0xf0, 0xf8, 0xf8, 0xf8, 0xf8, 0xfc, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfc, 0xfc, 0xfc, 0xf8, 0xfc, 0xfc, 0xf8, 0xf0, 0xe0, 0xe0, 0xe0, 0xc0, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xe0, 0xf8, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xfe, 0xfc, 0xfc, 0xf8, 0xf0, 0xe0, 0xe0, 0xc0, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe8, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0x07, 0x03, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x07, 0x07, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xfe, 0xfc, 0xf8, 0xf8, 0xf0, 0xe0, 0xc0, 0xc0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0xe0, 0xc0, 0xc0, 0xc0, 0x80, 0x80, 0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xf0, 0xf8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0x3f, 0x1f, 0x1f, 0x0f, 0x0f, 0x03, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x0f, 0x3f, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0x3f, 0x3f, 0x1f, 0x0f, 0x07, 0x07, 0x03, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x07, 0x07, 0x0f, 0x0f, 0x1f, 0x1f, 0x1f, 0x3f, 0x3f, 0x3f, 0x7f, 0x7f, 0x3f, 0x7f, 0x7f, 0x7f, 0xff, 0x7f, 0x7f, 0x3f, 0x7f, 0x3f, 0x3f, 0x3f, 0x3f, 0x1f, 0x1f, 0x0f, 0x0f, 0x07, 0x03, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

spi_device_handle_t spi_handle;

// reduces how much is refreshed, which speeds it up!
// originally derived from Steve Evans/JCW's mod but cleaned up and
// optimized
//#define enablePartialUpdate

#ifdef enablePartialUpdate
static uint8_t xUpdateMin, xUpdateMax, yUpdateMin, yUpdateMax;
#endif



static void updateBoundingBox(uint8_t xmin, uint8_t ymin, uint8_t xmax, uint8_t ymax) {
#ifdef enablePartialUpdate
  if (xmin < xUpdateMin) xUpdateMin = xmin;
  if (xmax > xUpdateMax) xUpdateMax = xmax;
  if (ymin < yUpdateMin) yUpdateMin = ymin;
  if (ymax > yUpdateMax) yUpdateMax = ymax;
#endif
}

Adafruit_PCD8544::Adafruit_PCD8544( int8_t SCLK, int8_t DIN, int8_t DC, int8_t CS, int8_t RST ) : Adafruit_GFX(LCDWIDTH, LCDHEIGHT) {
  _din  = DIN;
  _sclk = SCLK;
  _dc   = DC;
  _rst  = RST;
  _cs   = CS;
  ESP_LOGD(TAG, "CS: %d, MOSI: %d, SCLK: %d, DC: %d, RST: %d",
  	_cs, _din, _sclk, _dc, _rst
  );
}

// the most basic function, set a single pixel
void Adafruit_PCD8544::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height)) {
    return;
  }

  int16_t t;
  switch(rotation) {
    case 1:
      t = x;
      x = y;
      y =  LCDHEIGHT - 1 - t;
      break;

    case 2:
      x = LCDWIDTH - 1 - x;
      y = LCDHEIGHT - 1 - y;
      break;

    case 3:
      t = x;
      x = LCDWIDTH - 1 - y;
      y = t;
      break;
  }

  if ((x < 0) || (x >= LCDWIDTH) || (y < 0) || (y >= LCDHEIGHT))
    return;

  // x is which column
  if (color) {
    pcd8544_buffer[x+ (y/8)*LCDWIDTH] |= _BV(y%8);
  }
  else {
    pcd8544_buffer[x+ (y/8)*LCDWIDTH] &= ~_BV(y%8);
  }

  updateBoundingBox(x,y,x,y);
}


// the most basic function, get a single pixel
uint8_t Adafruit_PCD8544::getPixel(int8_t x, int8_t y) {
  if ((x < 0) || (x >= LCDWIDTH) || (y < 0) || (y >= LCDHEIGHT)) {
    return 0;
  }

  return (pcd8544_buffer[x+ (y/8)*LCDWIDTH] >> (y%8)) & 0x1;  
}


void Adafruit_PCD8544::begin(uint8_t contrast, uint8_t bias) {

	ESP_LOGI(TAG, "test abcd: _sclk: %d, _din: %d", _sclk, _din);
	spi_bus_config_t bus_config;
	bus_config.sclk_io_num   = _sclk; // CLK
	bus_config.mosi_io_num   = _din; // MOSI
	bus_config.miso_io_num   = -1; // MISO
	bus_config.quadwp_io_num = -1; // Not used
	bus_config.quadhd_io_num = -1; // Not used
	ESP_LOGI(TAG, "... Initializing bus.");
	ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &bus_config,0/*1*/)); // spi_bus_free() for "remove"
	spi_device_interface_config_t dev_config;
	dev_config.address_bits     = 0;
	dev_config.command_bits     = 0;
	dev_config.dummy_bits       = 0;
	dev_config.mode             = 0;
	dev_config.duty_cycle_pos   = 0;
	dev_config.cs_ena_posttrans = 0;
	dev_config.cs_ena_pretrans  = 0;
	dev_config.clock_speed_hz   = 800000; // 800KHz
	dev_config.spics_io_num     = _cs;
	dev_config.flags            = 0;
	dev_config.queue_size       = 1;
	dev_config.pre_cb           = NULL;
	dev_config.post_cb          = NULL;
	ESP_LOGI(TAG, "... Adding device bus.");
	ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &dev_config, &spi_handle)); // spi_bus_remove_device() for "remove"

  gpio_set_direction((gpio_num_t)_dc, GPIO_MODE_OUTPUT);
  if (_rst > 0) {
    gpio_set_direction((gpio_num_t)_rst, GPIO_MODE_OUTPUT);
  }

  // toggle RST low to reset
  if (_rst > 0) {
  	gpio_set_level((gpio_num_t)_rst, 0);
  	vTaskDelay(500/portTICK_PERIOD_MS);
  	gpio_set_level((gpio_num_t)_rst, 1);
  }

  // get into the EXTENDED mode!
  command(PCD8544_FUNCTIONSET | PCD8544_EXTENDEDINSTRUCTION );

  // LCD bias select (4 is optimal?)
  command(PCD8544_SETBIAS | bias);

  // set VOP
  if (contrast > 0x7f) {
    contrast = 0x7f;
  }

  command( PCD8544_SETVOP | contrast); // Experimentally determined

  // normal mode
  command(PCD8544_FUNCTIONSET);

  // Set display to Normal
  command(PCD8544_DISPLAYCONTROL | PCD8544_DISPLAYNORMAL);

  // initial display line
  // set page address
  // set column address
  // write display data

  // set up a bounding box for screen updates

  updateBoundingBox(0, 0, LCDWIDTH-1, LCDHEIGHT-1);
  // Push out pcd8544_buffer to the Display (will show the AFI logo)
  display();
}

void Adafruit_PCD8544::stop() {
  // TODO: Remove device from the bus
  ESP_LOGI(TAG, "Try to stop PCD8544 Display");
  int ret = spi_bus_remove_device(spi_handle);
  if(ret != ESP_OK) {
    ESP_LOGE(TAG, "Can not remove PCD8544 Display.");
    // TODO: interrupt this process
    return;
  }

  // TODO: Remove the spi bus
  ret = spi_bus_free(HSPI_HOST);
  if(ret != ESP_OK) {
    ESP_LOGE(TAG, "Can not remove SPI Bus HSPI_HOST.");
    // TODO: interrupt this process
    return;
  }
  ESP_LOGI(TAG, "Stop Display to using SPI bus & free SPI bus HSPI_HOST");
}


inline void Adafruit_PCD8544::spiWrite(uint8_t d) {
	spi_transaction_t trans_desc;
	trans_desc.addr      = 0;
	trans_desc.cmd       = 0;
	trans_desc.flags     = 0;
	trans_desc.length    = 8;
	trans_desc.rxlength  = 0;
	trans_desc.tx_buffer = &d;
	trans_desc.rx_buffer = NULL;

	ESP_ERROR_CHECK(spi_device_transmit(spi_handle, &trans_desc));
}

void Adafruit_PCD8544::command(uint8_t c) {
	gpio_set_level((gpio_num_t)_dc, 0);
  spiWrite(c);
}

void Adafruit_PCD8544::data(uint8_t c) {
	gpio_set_level((gpio_num_t)_dc, 1);
  spiWrite(c);
}

void Adafruit_PCD8544::setContrast(uint8_t val) {
  if (val > 0x7f) {
    val = 0x7f;
  }
  command(PCD8544_FUNCTIONSET | PCD8544_EXTENDEDINSTRUCTION );
  command( PCD8544_SETVOP | val); 
  command(PCD8544_FUNCTIONSET);
}


void Adafruit_PCD8544::display(void) {
  uint8_t col, maxcol, p;
  
  for(p = 0; p < 6; p++) {
#if defined(enablePartialUpdate)
    // check if this page is part of update
    if ( yUpdateMin >= ((p+1)*8) ) {
      continue;   // nope, skip it!
    }
    if (yUpdateMax < p*8) {
      break;
    }
#endif // enablePartialUpdate

    command(PCD8544_SETYADDR | p);


#if defined(enablePartialUpdate)
    col = xUpdateMin;
    maxcol = xUpdateMax;
#else // enablePartialUpdate
    // start at the beginning of the row
    col = 0;
    maxcol = LCDWIDTH-1;
#endif // enablePartialUpdate

    command(PCD8544_SETXADDR | col);

  	gpio_set_level((gpio_num_t)_dc, 1);

    for(; col <= maxcol; col++) {
      spiWrite(pcd8544_buffer[(LCDWIDTH*p)+col]);
    }
  }

  command(PCD8544_SETYADDR );  // no idea why this is necessary but it is to finish the last byte?
#ifdef enablePartialUpdate
  xUpdateMin = LCDWIDTH - 1;
  xUpdateMax = 0;
  yUpdateMin = LCDHEIGHT-1;
  yUpdateMax = 0;
#endif

}

// clear everything
void Adafruit_PCD8544::clearDisplay(void) {
  memset(pcd8544_buffer, 0, LCDWIDTH*LCDHEIGHT/8);
  updateBoundingBox(0, 0, LCDWIDTH-1, LCDHEIGHT-1);
  cursor_y = cursor_x = 0;
}
