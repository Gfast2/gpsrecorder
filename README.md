# Gpsrecorder

## Note for development

- Try to stop/restart Display SPI bus when need SD card IO.

## Work log

- 2019-4-13: Found out SPI bus can not be shared by SD card & my SPI Display at the same time. [Link](https://esp32.com/viewtopic.php?f=2&t=10127)

----

Starts a FreeRTOS task to print "Hello World"

See the README.md file in the upper level 'examples' directory for more information about examples.
