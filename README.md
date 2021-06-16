# cse190wi21_assignment3
Assignment 2: BLE Drop Detection

DESCRIPTION:
Adds in the bluetooth low energy shield to allow bluetooth communication on drop.

Tag Name- A14566071

SETUP:
The files ble.h and ble.cpp have been changed from the original, so keep that in mind when loading my code onto the microcontroller.

OPERATION:
At first, the BLE is put into standby mode.

Once dropped, the BLE will turn on and look to make connections with nearby devices. Once connected, the BLE will send the message:

	"Tag 'A14566071' dropped [TIME] ago"

Where [TIME] will be either the number of seconds (if it has been less than one minute), or number of minutes since the device was FIRST dropped (maxing out at 255 minutes).

The message will be sent over the course of multiple BLE packets as the message is too large. So, on the phone app, you should see something like the following output:

"Tag 'A14566" received
 Notification received from...
"071' droppe" received
 Notification received from...
"d 10 second" received
 Notification received from...
"s ago" received
 Notification received from...


To stop the device sending bluetooth information and reset drop detection, the user should send the single character "s" from the phone app.

The BLE will then be hard-reset and put into standby mode until the next drop is detected, and the process will repeat.