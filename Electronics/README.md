The custom electronic is just a very simple interface between Arduino and
the 2 buttons, magnetic sensor (rpm) and the Current Sensor.

The LCD screen (with SD Card) is an "Arduino TFT Screen" (Arduino Robot LCD)
i had available. The program uses the TFT library (included in Arduino IDE)
and hardware SPI to cominicate with Screen and SD card.
  https://www.arduino.cc/en/Guide/TFT

The motor driver used is a Cytron MD10 (any Motor driver with PWM and DIR
inputs will do) 
The Power Supply is a Meanwell LRS-200-24

Just choose Driver+Power Supply powerful enough for your motor (under load)


