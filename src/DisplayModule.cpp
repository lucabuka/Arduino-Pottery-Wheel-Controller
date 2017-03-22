/****************************************************************************
Display Module:

Note: Long and Double to String conversion:
 	On ATMEL micro we can use dtostrf(), ultoa()  a standard avr-libc function
	See: 
		http://www.nongnu.org/avr-libc/user-manual/group__avr__stdlib.html

	sprintf() in arduino libs is a stripped down version 
 	and, for float/double will most likely return a "?" 

	Besides, Strings objects and sprintf are much "heavier" in
	terms of programg size

****************************************************************************/
// BOF preprocessor bug prevent - insert me on top of your arduino-code


#include "DisplayModule.h"


// TFT LCD: create an instance of the library
static TFT myScreen = TFT(LCD_CS, DC, RESET);


void Display_init() {
	myScreen.begin();
	myScreen.background(0, 0, 0);
}

void Display_readyMsg() {
  myScreen.background(0, 0, 0);
  myScreen.setTextSize(3);
  myScreen.stroke(0,255,0);
  myScreen.text("Ready",40,50);
}

void Display_erase(){
  myScreen.background(0, 0, 0);
}


/*************************************************************************
 (Alt + 30) ▲   (Alt + 31) ▼   (Alt + 16) ►   (Alt + 17) ◄ 
*************************************************************************/
void Display_chooseRotationScreen() {

	// ASCII 30: Upwards filled arrow
	// ASCII 31: Downward filled arrow
	char upStr[] = {30,':','C','C','W',0}; 
	char downStr[] = {31,':','C','W',0}; 

	myScreen.stroke(255,255,255);
	myScreen.setTextSize(2);
	myScreen.text("Rotation: ",5,0);
	myScreen.text(upStr,20,40);
	myScreen.text(downStr,20,60);

	delay(2000); // let user the time to release the buttons...

}



/*************************************************************************
 (Alt + 30) ▲   (Alt + 31) ▼   (Alt + 16) ►   (Alt + 17) ◄ 
*************************************************************************/
void Display_updateDir(bool curDir){

	static bool prevDir  ; 
	char sxStr[] = {17, '-', 0}; 
	char dxStr[] = {'-', 16, 0}; 

	static char  dir_x = 120;
	static char	dir_y = 10;


	myScreen.setTextSize(3);

	// Delete prev string (write it with stroke(0,0,0) )
	myScreen.stroke(0,0,0);
	if(prevDir == HIGH)
		myScreen.text(sxStr, dir_x, dir_y);
	else
		myScreen.text(dxStr, dir_x, dir_y);


	// Display new string
	myScreen.stroke(0,255,255);
	if(curDir == HIGH)
		myScreen.text(sxStr, dir_x, dir_y);
	else
		myScreen.text(dxStr, dir_x, dir_y);

	prevDir = curDir;

	return;
}





/*************************************************************************
 Update RPM value on LCD
*************************************************************************/
void Display_updateRPM(unsigned long curRPM){

	static unsigned long prevRPM = 9999 ; 
	char r[5];   // RPM string 

	static char  rpm_x = 20;
	static char	 rpm_y = 40;


	myScreen.setTextSize(7);

	if(prevRPM != 9999) {
		// Delete prev string (write it with stroke(0,0,0) )
		//snprintf(pr, 5, "%lu",prevRPM);
		ultoa	(prevRPM,r,10);     // Rpm

		myScreen.stroke(0,0,0);
		myScreen.text(r, rpm_x, rpm_y);
	}

	// Display new string
	memset(r,0,5);
	ultoa	(curRPM,r,10);     // Rpm

	myScreen.stroke(255,255,255);
	myScreen.text(r,rpm_x, rpm_y);

	prevRPM = curRPM ;

	return;
}

/********************************************************
  Update Amp value on LCD
 ********************************************************/
void Display_updateAmp(double curAmp, bool printOverloadSymbol){

	static double prevAmp = 9999.0 ; 
	static char a[10];   // Amp string 

	static int  amp_x = 20;
	static int  amp_y = 100;
	static int	reset_warn_x = 100;
	static int	reset_warn_y = 90;

	myScreen.setTextSize(2);

	if(prevAmp != 9999.0) {
		// Delete prev string (write it with stroke(0,0,0) )
		dtostrf(prevAmp,5, 2, a);
		myScreen.stroke(0,0,0);
		myScreen.text(a, amp_x, amp_y);
	}

	// Display new string
	memset(a,0,10);
	dtostrf(curAmp,5, 2, a);
	myScreen.stroke(255,255,255);
	myScreen.text(a,amp_x, amp_y);

	prevAmp = curAmp ;

	myScreen.setTextSize(3);
	if(printOverloadSymbol){
		myScreen.stroke(255,255,0);
		myScreen.text("(!)",reset_warn_x, reset_warn_y);
	} else {
		myScreen.stroke(0,0,0);
		myScreen.text("(!)",reset_warn_x, reset_warn_y);
	}

	return;

}




