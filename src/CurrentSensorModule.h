
#ifndef _POTTERYWHEELCONTROLLER_CURRENTSENSORMODULE_H
#define _POTTERYWHEELCONTROLLER_CURRENTSENSORMODULE_H

#include <Arduino.h>  

// Smooth function (used by current sensor) 
// filterSamples should  be an odd number, no smaller than 3
#define filterSamples   15  

double CurrentSensor_getVal(int sensor_pin, double zeroVal) ;
int CurrentSensor_getZeroVal(int sensor_pin) ;
static int digitalSmooth(int rawIn, int *sensSmoothArray) ;

#endif
