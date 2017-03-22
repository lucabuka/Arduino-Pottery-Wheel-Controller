
/****************************************************************************
 * Current Sensor Module
*****************************************************************************/

#include "CurrentSensorModule.h"

// array for holding raw sensor values for Current Sensor
static int currentSensor_SmoothArray [filterSamples];  


#define SENSOR_AMP_SENSITIVITY   0.04
/****************************************************************************
  double getCurrentSensor_Val() - Get current value from Current Sensor

50A Current Sensor: Allegro Microsystem ACS770LCB-050B (50A - Bidirectional)

ACS770 50B (Bidirectional):
Requires a 5V power supply (VCC)
Output voltage (across VIOUT and ground) is proportional to current flowing 
between IP+ e IP-.
The value that links the two measurements is SENSITIVITY ( datasheet): 40mV/a
Using analogRead() to read VIOUT. the returned values will be between 0 
(0V in input) and 1023 (5V in input).
The sensor can measures pos and neg currents (range -50A…50A), so if input 
current is 0, analogRead() will return (1023/2)=511 or so.

The formula to convert units from analogRead() to Ampere will be:

           (analogRead() - analogRead(at 0 Amp) ) * 5.0
  Volts =  --------------------------------------------
                          1023

                Volts
  Current = -------------
             Sensitivity

***************************************************************************/
double CurrentSensor_getVal(int sensor_pin, double zeroVal) {
  int rVal, sRVal;
  double sOutV, sCurrentValue;

  rVal = analogRead(sensor_pin);
  sRVal = digitalSmooth(rVal, currentSensor_SmoothArray);


  sOutV = (sRVal - zeroVal) * 5.0 / 1023.0 ;
  sCurrentValue = sOutV / SENSOR_AMP_SENSITIVITY;
  // abs(value): the motor turns both ways and the current sensor returns 
  // positive or negative values 
  sCurrentValue = abs(sCurrentValue); 
  return (sCurrentValue);
}


//////////////////////////////////////////////////////////////////////////
//
//  Get sensor returned value on ZERO CURRENT situation (motor speed = 0)
//
//////////////////////////////////////////////////////////////////////////
int CurrentSensor_getZeroVal(int sensor_pin) {
  int i, rVal, sRVal;

  for (i = 0; i <= 50; i++) {
    rVal = analogRead(sensor_pin);
    sRVal = digitalSmooth(rVal, currentSensor_SmoothArray);  
    delay(30);
  }
  return (sRVal);

}


/*************************************************************************
  digitalSmooth()
**************************************************************************/
static int digitalSmooth(int rawIn, int *sensSmoothArray) {
  int j, k, temp, top, bottom;
  long total;
  static int i;
  // static int raw[filterSamples];
  static int sorted[filterSamples];
  boolean done;

  i = (i + 1) % filterSamples;  // increment counter and roll over if necc. 
										  // % (modulo operator) rolls over variable
  sensSmoothArray[i] = rawIn;   // input new data into the oldest slot

  for (j = 0; j < filterSamples; j++) { // transfer data array into another 
													 // array for sorting and averaging
    sorted[j] = sensSmoothArray[j];
  }

  done = 0;          // flag to know when we're done sorting
  while (done != 1) {// simple swap sort, sorts numbers from lowest to highest
    done = 1;
    for (j = 0; j < (filterSamples - 1); j++) {
      if (sorted[j] > sorted[j + 1]) {    // numbers are out of order - swap
        temp = sorted[j + 1];
        sorted [j + 1] =  sorted[j] ;
        sorted [j] = temp;
        done = 0;
      }
    }
  }

  // throw out top and bottom 15% of samples - limit to throw out at 
  // least one from top and bottom
  bottom = max(((filterSamples * 15)  / 100), 1);
  
  // the + 1 is to make up for asymmetry caused by integer rounding
  top = min((((filterSamples * 85) / 100) + 1  ), (filterSamples - 1));   
  k = 0;
  total = 0;
  for ( j = bottom; j < top; j++) {
    total += sorted[j];  // total remaining indices
    k++;
  }

  return total / k;    // divide by number of samples
}


