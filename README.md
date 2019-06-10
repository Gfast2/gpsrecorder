# Gpsrecorder

Gpsrecorder will help me record a GPS coordinate by short press the button on its front panel. The
long button press will let it change its display/running mode. The possible mode and their use is as
follow:
- _Mode 1:_ Showing Temperature and Humidity
- _Mode 2:_ Showing detailed recorded GPS information


## Note for development
- Long press make the Display change to "Next" view
- After make a GPS coordinate snapshot restart the cached taskes

# Road Map

_For Version 1.2.0_
- Try to map psram into map look-up table
- Wifi access point setup (DHCP Server Included)
- Let Web-Server Access SD Card content when get a get request
- Very Basic HTML Page that content all last 300 Saved coordinates (Limitation is only in HTML tag element limitition)
- Replace function "getFloat" in gps_clock.c by mimia's stock "minmea_tofloat"

_For Version 1.3.0_
- Double click to turn Display Backlight on/off
- Tripple click to turn Wifi Access Point & Webserver on/off
- Solder RTC Curcuit
- Add process to read RTC Module's time, and make it as the new System time. When got time from GPS Module, do time sync to RTC Module
- Checkout if there is still free capacity
---

- Use [Task notifications](https://www.freertos.org/RTOS-task-notifications.html) to emmit task change events (replace semaphore solutions)

## Work log

### Version 1.0.0
- 2019-4-13: Found out SPI bus can not be shared by SD card & my SPI Display at the same time. [Link](https://esp32.com/viewtopic.php?f=2&t=10127)
- ? : Try to stop/restart Display SPI bus when need SD card IO.
- 2019-4-27: Front Panel push button short/long push event
- 2019-5-1: Read GPS Signal in SD Card I/O Task, write it down to the SD Card
- 2019-5-1: Rewrite the communication model between temperature Task & Display Task
- 2019-5-5: For each button push trigger either a short button event or long button event. only one of them
----

### Version 1.0.1
- 2019-5-12: Protect if the sd Card IO Failed. If software can't access SD Card save snapshot, it will terminate the task, but not return the task & crash the software
- 2019-5-12: After trying to save coordinate to sd card, show the save result

### Version 1.1.0
- 2019-5-18: Display mode "Display Speed" (km/h)
- 2019-5-18: Display mode "Time" (Time, Month & Dan are bigger Font, Year are small font)
- 2019-5-22: Display mode "Last two recorded coordinate"
- 2019-5-24: Move snapshot logic into a abstracted event handler (a function)
- 2019-5-24: Move switching next mode logic into a abstracted event handler (a function)
- 2019-5-25: Move Temperature Logic into its own unit
- **Won't do**: Bugger the driver of the Display, make the Display preserve its content during display spi bus unmounted & showing "Switching to next stage" (Because no more remember why its needed)
- 2019-5-30: Clean up code in unit "test_pcd8544.cpp" (abstract display mode start initialization into "initDisp()")
- 2019-6-10: Make a logo for this project (foto see: "/doc/logo_design_20190610_081207.jpg")
- 2019-6-10: Try to fix bug when in _SPEED_ mode, can not make a gps coordinate save (When display speed in size '3' and make font what text black background, there is something works not that great, so as workaround, show text in font size '3' & make it black text & what background)