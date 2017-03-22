
#ifndef _POTTERYWHEELCONTROLLER_H
#define _POTTERYWHEELCONTROLLER_H

#include <Arduino.h>

#include "HardwareCfg.h"
#include "DisplayModule.h"
#include "LogModule.h"
#include "CurrentSensorModule.h"

#include <PWM.h>  // Lib to fine control PWM frequency


void startUpCfg();
void selectRotation();
void SDInfo();
void printDirectory(File dir, int numTabs) ;

bool  checkOverload(void) ;
bool  overloadDetected(void);

void checkUI(void) ;
int CheckButtons(void);
int readButtons(void);

void stopMotor(void);
void SpeedISR();
void beep(int type);

char * log_WrkParams(char * Id, double Amp, unsigned long Rpm, int Pwm);
void writeLog(char * str, bool sync);

#endif
