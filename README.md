# Arduino-Pottery-Wheel-Controller


Arduino Pottery Wheel Controller

Arduino program to control the speed of a pottery wheel powered by a CC motor

Hardware:
 - 1 Buzzer
 - 1 TFT LCD display for RPM, AMP and user CFG of Rotation (CW, CCW)
 - 1 SD Card to LOG data (user inputs, Oveloads, etc)
 - 2 Foot switches used for speed control.
 - Custom home made shield:
	- 1 Current sensor for emergency stop of the motor on OverCurrent
	- 1 HALL sensors (magnetic) used in RPM calculation
	- some electronic to debounce, pull-up, etc. (--see Docs)

The "main" output is:
 - PWM signal feed to the motor controller

 User Interface:
	- Button UP		-> Motor Speed Up
	- Button DOWN	-> Motor Speed Down
	- BOTH buttons -> MOTOR STOP (Ramp down to 0)
		- BOTH buttons pushed again -> Motor "SET DIRECTION" Menu

Overload protection:
	The program REDUCE the speed to a configurable value after 
	an Overload condition last longer "X" mSec.
	If the Overload persist for longer than Y mSec, stops the motor
	(X,Y = compile-time configurable values)



Program compiled/linked via PlatformIO 3.3
It should be easy to compile it using Arduino IDE

2017-03-22: Ver 0.9 - Luca (lucabuka@gmail.com)

# Arduino-Pottery-Wheel-Controller
