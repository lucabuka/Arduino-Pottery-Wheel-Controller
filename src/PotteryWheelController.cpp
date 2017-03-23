// BOF preprocessor bug prevent - insert me on top of your arduino-code
/****************************************************************************

Pottery Wheel Controller

Arduino Prg for speed control for a pottery wheel powered by a CC motor
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


2017-03-22: Ver 0.9 - Luca

****************************************************************************/

#include "PotteryWheelController.h"


/************************************************************************* 
 *                        Parameters Setup 
 *************************************************************************/ 

#define	LOG_TO_SERIAL  1 	// 0:Log written to SD card 
									// 1:Log written to SD card AND to Serial as well
								
#define	MODE_DEBUG  1		// Some extra startup logs (written to Serial only)

// Default Rotation (CW=LOW or CCW=HIGH)
#define DEFAULT_ROTATION		LOW;

//  PWM
#define PWM_FREQUENCY 20000	// PWM carrier frequency (in Hz) - 20 KHz 
#define MAX_PWM_VAL		250	// PWM for Max motor speed
#define PWM_STEP			 10	// PWM increment/decrement on buttons pressed
#define MIN_PWM_VAL		 40	// PWM for Min motor speed (less than this 
										//	will be set automatically to 0)

// Current (Ampere)  Sensor
#define SENSOR_AMP_MAX_VALUE     14.5 // Max AMP before detecting an Overload 

#define SENSOR_AMP_MAX_OLTIME     450 // Max millis() before OVERLOAD produce
												  // a RESET SPEED

#define SENSOR_AMP_TOTAL_OLTIME  3000 // Max millis() before OVERLOAD produce
												  // a MOTOR STOP

#define PWM_VAL_AFTER_OVERLOAD    150 // After detecting an overload the PWM 
												  // will be automatically reset to this 
												  // value (ONLY if this value is MINOR
												  // than the current speed !)
												  // (set = 0 to stop the motor)

// Type of output from "digital" sensors.
// Set "xxx_ON_STATE" as follows:
//		HIGH: for sensor returning a high state on detection
//		LOW : for sensor returning a low state on detection
#define SENSOR_RPM_ON_STATE	LOW	// Speed Sensor 
#define BUTTON_UP_ON_STATE		LOW	// Button "UP"
#define BUTTON_DOWN_ON_STATE	LOW	// Button "DOWN"

/*********************** END Parameters setup ****************************/ 


// Set debug macro if MODE_DEBUG
#if MODE_DEBUG

#define 	PDEBUGS(s)      Serial.print(s)
#define	PDEBUG(s, x)    { Serial.print(F(s)); Serial.print(x); }
#define	DEBUGX(s, x)   { Serial.print(F(s)); Serial.print(x, HEX); }

#else

#define PDEBUGS(s)
#define	PDEBUG(s, x)
#define	DEBUGX(s, x)

#endif // MODE_DEBUG


// MACRO defines for beep() sounds (buzzer)
#define BEEP_Short					1
#define BEEP_Long						2
#define BEEP_ResetAfterOverload	10
#define BEEP_StopAfterOverload	11


// Vars used in IRS -> Store RAM
volatile int currentSpeedSensorState=0; 
volatile unsigned long  t0 = 0;
volatile unsigned long  t_hb = 0;


int currentSensor_ZeroAnalogRead=510; // "Zero" will be READ/SET on setup()
bool				rotation = DEFAULT_ROTATION;
int				currentPWM;
double			currentAmp = 0;
unsigned long	CurrentRPM = 0;
bool				resetAfterOverload=false;

void setup()
{

	Serial.begin(115200);

	pinMode(SENSOR_SPEED, INPUT);
	pinMode(SENSOR_AMP_PIN, INPUT);
	pinMode(SPEED_UP_PIN, INPUT);
	pinMode(SPEED_DOWN_PIN, INPUT);
	
	pinMode(LED_ON, OUTPUT);
	pinMode(LED_RPM_SENSOR_ON, OUTPUT);
	pinMode(DIR_PIN, OUTPUT);
	pinMode(PWM_PIN, OUTPUT);
	pinMode(SPEAKER,OUTPUT); 
	
	/******************************************************************/
	// Check the initial state of the RPM sensor 
	// NOTE:
	//   ISR() uses "currentSpeedSensorState" variable to know
	//   if it was called on RISING or FALLING edge.
	currentSpeedSensorState = digitalRead(SENSOR_SPEED);
	
	// Attach ISR
	attachInterrupt(digitalPinToInterrupt(SENSOR_SPEED), SpeedISR, CHANGE);
	
	// PWM Lib
	// initialize all timers except for 0, and sets the frequency for the pin
	PDEBUGS("Initializing Timers for PWM (Timer0 left untouched)...\n");
	InitTimersSafe();
	bool success = SetPinFrequencySafe(PWM_PIN, PWM_FREQUENCY);
	if (!success) {	// On FAILURE, program execution STOPs 
		PDEBUG("ERROR ON SET PWM_PIN to frequency:", PWM_FREQUENCY);
		while (1) {
		beep(BEEP_Long);
		digitalWrite(LED_ON, HIGH);
		delay(250);
		beep(BEEP_Short);
		digitalWrite(LED_ON, LOW);
		delay(250);
		}
	}
	
	pwmWrite(PWM_PIN, 000);
	currentPWM = 0;
	
	// Set value measured on "zero current condition" by current sensor
	currentSensor_ZeroAnalogRead = CurrentSensor_getZeroVal(SENSOR_AMP_PIN);
	// Read current value (Amperes) - should be = 0 (pwm set to 0)
	currentAmp = CurrentSensor_getVal(SENSOR_AMP_PIN,
												currentSensor_ZeroAnalogRead );
	
	PDEBUG("Current Sensor ZERO value (0-1024):", currentSensor_ZeroAnalogRead);
	PDEBUGS("\n");
	PDEBUG("Current Sensor value (Amperes) at speed=0: ", currentAmp);
	PDEBUGS("\n");
	PDEBUG("Speed sensor initial state: :", currentSpeedSensorState);
	PDEBUGS("\n");
	
	PDEBUGS("Initializing SD card...\n");
	if (! LOG_init(SD_CS))  { // Returns 0:OK, 1:Error
		if(MODE_DEBUG) {
			LOG_printDebugInfo(); // Call BEFORE THAN  LOG_init() !!!!
		}
		PDEBUGS("Initializing LOG file...\n");

		LOG_open(true); // True: send log output to Serial.println() as well
	}
	
	Display_init() ;

	Display_readyMsg();
	
	digitalWrite(LED_ON, HIGH);

	beep(BEEP_Short);
}



/*****************************************************************************
  main loop
    - check error conditions (Overload)
    - set pwm value on motor controller
******************************************************************************/
float SensorAmpStartHiDefLog = SENSOR_AMP_MAX_VALUE - 2;
void loop()   {

	static int				lastPWM = 0;
	static unsigned long lastRpm = 0;
	static double			lastAmp = 0;
	static bool				logParams = false;

	unsigned long	deltaR;
	double			deltaA;

  	// Check Overload conditions and update global var currentAmp.
  	checkOverload();

	// Change in "Ampere" value ?
	deltaA = abs(currentAmp - lastAmp);
	if((deltaA > 1.5) || 
		((deltaA > 0.3 ) && (currentAmp > SensorAmpStartHiDefLog))) {

		Display_updateAmp(currentAmp, resetAfterOverload);
		lastAmp = currentAmp;
		logParams = true;  // will write to log file
	}

	// Calc "RPM"
	if(t_hb != 0) {  // only on valid "period" t_hb
		CurrentRPM = 60000/t_hb ;
	}
	// Change in "RPM" value ?
	deltaR = abs(CurrentRPM - lastRpm);
	if(deltaR > 2) {
		Display_updateRPM(CurrentRPM);
		lastRpm = CurrentRPM ;
		logParams = true;  // will write to log file
	}

	// Turn LED_RPM_SENSOR_ON Led ON when Hall sensor detect the magnet
	if(currentSpeedSensorState == SENSOR_RPM_ON_STATE){
		digitalWrite(LED_RPM_SENSOR_ON, HIGH);
	} else {
		digitalWrite(LED_RPM_SENSOR_ON, LOW);
	}

	// Read user buttons
	checkUI() ;

	digitalWrite(DIR_PIN, rotation); // Write to DIR pin -> Motor driver

	// Change in "PWM" value ?
	if(currentPWM != lastPWM  ) {
		pwmWrite(PWM_PIN, currentPWM); // Write to PWM pin -> Motor driver
		lastPWM = currentPWM ;
		logParams = true;  // will write to log file

		if (currentPWM == 0) { 
			CurrentRPM = 0; t_hb = 0; currentAmp=0,lastAmp=0; 
			Display_updateAmp(currentAmp, resetAfterOverload);
		}
	}

	if(logParams) {
		LOG_write(LOG_SYSTEM_INFO, lastAmp, lastRpm, lastPWM, 0, true);
	}
	logParams = false;
}


/********************************************************************
 checkOverload()

 Note:
  An overload condition lasting more than "SENSOR_AMP_TOTAL_OLTIME"
  leads to a MOTOR STOP (call to stopMotor() function) 
  AFTER the STOP the routine returning AMP values from the
  sensor -CurrentSensor_getVal()- still returns HIGH current for
  approx 600m Sec (mainly caused by "smoothed" values)
  Because of this we need to add a "&& currentPWM !=0 " to 
  if(overloadDetected()) check - without it the code below
  will RESTART the motor with a PWM=PWM_VAL_AFTER_OVERLOAD !!!
 ********************************************************************/
bool  checkOverload(void) {
	static bool first_loop = 1;
	static uint32_t lastOnTime;
	uint32_t currentTime;
	char errstr[40];
	
	if (overloadDetected() && currentPWM != 0) { // PWM=0 right after stop
		if (first_loop) {
			lastOnTime = millis();
			first_loop = 0;
		}
		currentTime = millis();

		// Load has been above "SENSOR_AMP_MAX_VALUE" Amps for more 
		// than "SENSOR_AMP_MAX_OLTIME" mSec -> Set PWM to cfg value
		if(currentPWM > PWM_VAL_AFTER_OVERLOAD) {
			currentPWM = PWM_VAL_AFTER_OVERLOAD;
		}
		beep(BEEP_ResetAfterOverload);
		resetAfterOverload=true;
	
		strcpy(errstr,"PWM Reset after overload");	
		LOG_write(LOG_SYSTEM_ACTION,currentAmp, 
						CurrentRPM, currentPWM, errstr, true);

		if (currentTime - lastOnTime > SENSOR_AMP_TOTAL_OLTIME) {
			stopMotor();
			strcpy(errstr,"MOTOR OFF after long overload");	
			LOG_write(LOG_SYSTEM_ACTION,currentAmp, 
							CurrentRPM, currentPWM, errstr, true);
			beep(BEEP_StopAfterOverload);
			first_loop=true;
		}
  } else {
    lastOnTime = millis();
  }
	return(0);
}



/////////////////////////////////////////////////////////////////////////////
// overloadDetected()
//  Returns [1] on Overlocad conditions:
//    - Load has been above "SENSOR_AMP_MAX_VALUE" Amps for more
//      than "SENSOR_AMP_MAX_OLTIME" mSec
//  Returns(0) otherwise
////////////////////////////////////////////////////////////////////////////
bool  overloadDetected(void) {
  static bool first_loop = 1;
  static uint32_t lastOnTime;
  uint32_t currentTime;

  currentAmp = CurrentSensor_getVal(SENSOR_AMP_PIN, 
												currentSensor_ZeroAnalogRead);
  if (currentAmp >= SENSOR_AMP_MAX_VALUE) {
    if (first_loop) {
      lastOnTime = millis();
      first_loop = 0;
    }
    currentTime = millis();
    if (currentTime - lastOnTime > SENSOR_AMP_MAX_OLTIME) {
		LOG_write(LOG_SYSTEM_OVERLOAD,currentAmp,CurrentRPM,currentPWM,0,true);

      return (HIGH);
    }
  } else {
    lastOnTime = millis();
  }
  return (LOW);
}


/***************************************************************************

****************************************************************************/
void checkUI(void) {

	static bool userInputReceived=false;
	static bool firstScreenCleanupDone=false;
	static unsigned long bothPressedStartTime = 0;  
	long timeout = 1500L;
	char str[10];
	
	if( userInputReceived &&  ! firstScreenCleanupDone ) {
		Display_erase();
		Display_updateDir(rotation);
		firstScreenCleanupDone = true;
	}

  switch (CheckButtons()) {
	 case 0:
	 	break;
    case 1: // ButtonUP pressed
		beep(BEEP_Short);
	 	currentPWM += PWM_STEP;
		if (currentPWM > MAX_PWM_VAL) currentPWM=MAX_PWM_VAL;
		if (currentPWM < MIN_PWM_VAL) currentPWM=MIN_PWM_VAL; // Jumps up to min
		resetAfterOverload=false;
		userInputReceived=true;

		strcpy(str,"UP");
		LOG_write(LOG_USER_ACTION,currentAmp,CurrentRPM,currentPWM,str,true);
		break;

	 case 2: // Button Down pressed
		beep(BEEP_Short);
	 	currentPWM -= PWM_STEP;
		if (currentPWM < MIN_PWM_VAL) currentPWM=0; // jumps down to 0
		resetAfterOverload=false;
		userInputReceived=true;

		strcpy(str,"Down");
		LOG_write(LOG_USER_ACTION,currentAmp,CurrentRPM,currentPWM,str,true);
		break;

    case 3:  // Both button pressed
		beep(BEEP_Long);
		stopMotor();

		strcpy(str,"Both");
		LOG_write(LOG_USER_ACTION,currentAmp,CurrentRPM,currentPWM,str,true);

		if(bothPressedStartTime == 0){
			bothPressedStartTime = millis();
		} else { 
			// Both buttons pushed a second time right after
			// the first one that stopped the motor -> Dir setup
			if(millis() - bothPressedStartTime	> timeout){

				stopMotor(); // on sw bugs: Avoid dir change with motor ON !!!!

				strcpy(str,"Set DIR");
				LOG_write(LOG_USER_ACTION,currentAmp,CurrentRPM,currentPWM,str,true);

				bothPressedStartTime = 0;

				Display_erase();
				selectRotation();
				Display_erase();
				Display_updateDir(rotation);
			}
		}
		break;	

    default:
	 	// ERROR !!!!
	 	break;
  }


  return;
}


/*****************************************************
  STOP the motor (cycle to have "gentle" a ramp down)
*****************************************************/
void stopMotor(void) {
	for(int i=currentPWM; i>0 ; i-=2) {
		pwmWrite(PWM_PIN, i);
		delay(25);
	}
	pwmWrite(PWM_PIN, 0);
	currentPWM=0 ; t_hb=0; currentAmp=0; 
}


/***************************************************************************
  Allows ONLY 1 "Action" each time a button is pushed.
  If the user wants to repeat, it have to RELASE + PUSH the button again
****************************************************************************/
int CheckButtons(void) {
  int button;
  static int last_button;

  button=readButtons();
  if(button != last_button) {
    last_button = button;
    return(button);
  }
  return(0);
}


/*****************************************************
  Return debounced input from UP and DOWN button
*****************************************************/
int readButtons(void) {
  static int buttonUpState;   	// the current VALID reading from the input pin
  static int buttonDownState;
  static int lastUpState = 0;		// the previous reading from the input pin
  static int lastDownState = 0;
  int tmpUpState = 0;				// the current reading from the input pin
  int tmpDownState = 0;

	// the following variables are long's because the time, measured in 
	// miliseconds, will quickly become a bigger number than can be stored 
	// in an int.
	//
	static unsigned long debounceDelay = 50; // debounce time

	// the last time the output pin was toggled
	static unsigned long lastDebounceUpTime = 0;  
	static unsigned long lastDebounceDownTime = 0;

  // read the state of the switches
   if(digitalRead(SPEED_UP_PIN) == LOW){
		tmpUpState = 1;
   } else {

		tmpUpState = 0;
	}
	if(digitalRead(SPEED_DOWN_PIN) == LOW){
     tmpDownState = 2;
  } else {
	  tmpDownState = 0;
  }
   // check to see if you just pressed the button
   // (i.e. the input went from LOW to a buttonState),  and you've waited
   // long enough since the last press to ignore any noise:

   // "UP" Button
	//////////////
   if (tmpUpState != lastUpState) {
		// If the switch changed, due to noise or pressing:
		// reset the debouncing timer
     lastDebounceUpTime = millis();
   }
   if ((millis() - lastDebounceUpTime) > debounceDelay) {
     // whatever the reading is at, it's been there for longer
     // than the debounce delay, so take it as the actual current state:
     buttonUpState = tmpUpState;
   }
   // save the reading.  Next time through the loop,
   // it'll be the lastButtonState:
   lastUpState = tmpUpState;

	// "DOWN" Button
	//////////////
   if (tmpDownState != lastDownState) {
		// If the switch changed, due to noise or pressing:
		// reset the debouncing timer
     lastDebounceDownTime = millis();
   }
   if ((millis() - lastDebounceDownTime) > debounceDelay) {
     // whatever the reading is at, it's been there for longer
     // than the debounce delay, so take it as the actual current state:
     buttonDownState = tmpDownState;
   }
   // save the reading.  Next time through the loop,
   // it'll be the lastButtonState:
   lastDownState = tmpDownState;

	return( buttonUpState + buttonDownState);

 }



/***************************************************************************
  ISR for SpeedISR sensor (on CHANGE state)
****************************************************************************/
void SpeedISR() {

	unsigned long t;
	// interrupt called when sensor stauts was OFF - Is the RISING edge
	if (currentSpeedSensorState != SENSOR_RPM_ON_STATE) { 
		t = millis();
		t_hb = (t - t0);
		t0 = t;
    currentSpeedSensorState = SENSOR_RPM_ON_STATE;

  } else { // Falling edge
    currentSpeedSensorState = ! SENSOR_RPM_ON_STATE;
  }

}


/*
 *  Manage USER interface for DIR Change
 */
void selectRotation(){

	Display_chooseRotationScreen() ;
	delay(1500); // let user the time to release the buttons...
	// Default Rotation (CW=LOW or CCW=HIGH)
	bool done = false;
	while(!done) { 
		switch (CheckButtons()) {
			case 1: // ButtonUP pressed - CCW (HIGH)
				rotation=HIGH;
				done=true;
				break;
		
			case 2: // Button Down pressed CW (LOW)
				rotation=LOW;
				done=true;
				break;
		}
	}

	beep(BEEP_Short);
	delay(100);
	beep(BEEP_Long);
	delay(200);
	beep(BEEP_Short);
	delay(100);
	beep(BEEP_Long);
	delay(200);
	beep(BEEP_Short);
}


/*
 *	Buzzer
 */

void beep(int type) {
	switch(type){
		case 	BEEP_Short:
			tone(SPEAKER,440,100);		
			break;
		case BEEP_Long:
			tone(SPEAKER,680,600);		
			break;
		case BEEP_ResetAfterOverload:
			tone(SPEAKER,880,2000);		
			break;
		case BEEP_StopAfterOverload:
			tone(SPEAKER,880,5000);		
			break;
	}
	return;
}








