# Gpsrecorder

## Note for development
- Long press make the Display change to "Next" view
- After make a GPS coordinate snapshot restart the cached taskes
- Test for Version 1.0.0

# Road Map
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