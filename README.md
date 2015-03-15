# DistanceLight
Arduino Sketch for my ultrasonic parking guide

This is the code to my ultrasonic parking guide (ie a glorified tennisball on a string).
Parts of the code are based on the Arduino Ping sensor example.
- Expects HC-SR04 Ranging Detector Mod Distance Sensor Trigger and Echo pins connected to pins 3 and 6.
- Expects one standard RGB LED (the red green and blue #defines are for those pins on the LED.)
- Expects one momentary normally open button that throws HIGH to buttonPin when pressed.
 

LED will be green at >target distance + 150CM (5 Ft.)
LED gradients yellow to red from Target Distance+150CM to Target Distance
At target distance LED flashes red 5 times then goes to solid red.
Pressing the button and holding for 3 seconds will save the currently measured distance as the target distance.
The LED will turn blue while you're holding it, then flash green 3 times when the setting is saved to the first 4 bytes of the EEPROM.
Continuing to hold the button for another 3 seconds will cause the LED to turn red and the EEPROM will be written out all zeros.
If you put this on a hackduino / fresh ATMEGA chip with a programmer you may need to clear EEPROM with above procedure to get the sensor to behave properly (I had some strange results with a new chip).
The serial is only enabled for debugging, you could remove all of that if you weren't going to make any changes to make the compiled sketch smaller.
Distributed under GPLv3
