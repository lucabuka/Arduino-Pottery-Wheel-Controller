
#ifndef _POTTERYWHEELCONTROLLER_DISPLAYMODULE_H
#define _POTTERYWHEELCONTROLLER_DISPLAYMODULE_H

#include <TFT.h>  // Arduino LCD library
#include <SD.h>
#include "HardwareCfg.h"

void Display_readyMsg();
void Display_init() ;
void Display_erase();
void Display_chooseRotationScreen() ;
void Display_updateDir(bool curDir);
void Display_updateRPM(unsigned long curRPM);
void Display_updateAmp(double curAmp, bool printOverloadSymbol);


#endif

