
#ifndef _POTTERYWHEELCONTROLLER_HARDWARECFG_H
#define _POTTERYWHEELCONTROLLER_HARDWARECFG_H

/**************************************************************************
 * Hardware configuration of Arduino pins
 **************************************************************************/
#define SENSOR_SPEED			2 // RPM: Pin used for Speed sensor - via interrupt
#define SPEAKER				3 // Loudspeaker for alarms - notifications
#define LED_RPM_SENSOR_ON  6 // LED ON when rpm sensor on (hall sensor)

// Pin configured to use Hardware SPI (mandatory to access the SD card)
#define SD_CS			  4 // SD Card - Chip Select (Slave Select)
#define RESET			  7
#define DC				  8 
#define LCD_CS			 10 // LCD - Chip Select (Slave Select)
// MOSI -             11 - Master Out Slave In - default pin = 11
// MISO -             12 - Master Out Slave In - default pin = 12
// SCLK -				 13 - System Clock - default pin = 13

// Motor Controller
#define DIR_PIN		 5 // Connected to DIRECTION pin on Cytron MD10
#define PWM_PIN       9 // PIN PWM output signal for motor controller
                    		 // NOTE: Use Pin 9 or 10 for UNO (Timer 1)

// Input buttons and Current Sensor
#define SENSOR_AMP_PIN A0 // Pin connected to the ACS770 output 
#define LED_ON         A1 // ON-OFF Led- (A1 used as a Digital Input)
#define SPEED_UP_PIN	  A4 // Speed UP button - (A4 used as a Digital Input)
#define SPEED_DOWN_PIN A5 // Speed DOWN button - (A5 used as a Digital Input)

#endif

