#include <Arduino.h>
#include "LogModule.h"

/*
 *  SDFAT lib :  pio lib install 1432
 */


static char maxLogFileNum = 3; // number of log file (after this -> rotate)
static char cntFile[] = "CNT.TXT"; // File containing current CNT
static char logFileBName[] = "LOG."; // Log file BASE name
static char logFileName[10] ; // String holding the calculated filename
										// ACHTUG!!!
										//  It should be wide enough to contains
										//  string BASE + CNT

static SdFat sd;
static SdFile logFile;
static bool log_ok = false;
static bool log_toSerial = false;
/**************************************************************************
 * Init SD lib
 *************************************************************************/
bool LOG_init(int cs_pin) {
	// Initialize SdFat or print a detailed error message and halt
	// Use half speed like the native library.
	// change to SPI_FULL_SPEED for more performance
	if (!sd.begin(cs_pin, SPI_FULL_SPEED)) {
		return(1);
	}
	log_ok = true;
	return(0);
}

/**************************************************************************
 * Load cntSuff from "cntFile" (or create cntFile)
 * Open Log file for writing
 *************************************************************************/
bool LOG_open(bool toSerial) {
	if (!log_ok){
		return(1);
	}
	if(toSerial) log_toSerial = true;

	setLogFileName(logFileName);

	if (! logFile.open(logFileName, O_RDWR | O_CREAT | O_TRUNC)) {
		Serial.println("Error");
		log_ok = false;
		return(1);
	}

	logFile.sync();

	return(0);

}

/**************************************************************************
   Write log String (CSV record)

   The record will have the following format:
		"Id,d,l,i,optional text..."
	 Examples:
		"I,9.22,48,120," <- "Info" line containg Amp,Rpm,PWM telemetry
		"O,9.22,48,120," <_ "Overload" line with Amp,Rpm,PWM telemetry

 *************************************************************************/
char outRec[40];
char buf[15];
char * LOG_write(char Id,double dv,unsigned long lv,int iv,char * str,bool sync) {
	
	if (!log_ok){
		return(0); // null char *
	}

	// memset(dp,' ',len);
	memset(outRec,0,40);
	memset(buf,0,15);

	// millis() timestamp	
	memset(buf,0,15);
	ultoa	(millis(),buf,10);    
	strcpy(outRec, buf);

	// "ID" char
	* (outRec + strlen(buf)) = ',';
	* (outRec + strlen(buf)+1) = Id;
	strcat(outRec, ","); 

	// dtostrf(val,6,2,str); -> sprintf(str,"%6.2f",val);
	dtostrf(dv,5,2,buf);   // Double Value (usually Amp)
	strcat(outRec, buf); 
	strcat(outRec, ","); 

	memset(buf,0,15);
	ultoa	(lv,buf,10);    
	strcat(outRec, buf); 
	strcat(outRec, ","); 

	memset(buf,0,15);
	itoa	(iv,buf,10);   // Int value (usually PWM)  
	strcat(outRec, buf); 
	strcat(outRec, ","); 

	strcat(outRec, str); // optional string

	// print to FILE
	logFile.println(outRec);

	if(sync){
		logFile.sync();
	}

	if(log_toSerial) { 
		Serial.print("LOG:");	
		Serial.println(outRec);	
	}

	return(outRec);
}





/*************************************************************************
*************************************************************************/
void LOG_printDebugInfo(){

	if (!log_ok){
		return; 
	}
	// List SD card content
	File root = sd.open("/");
	root.ls(	&Serial, LS_SIZE | LS_R, 3 )	;
	root.close();

	// Dump Counter file content
	Serial.print("File:["); Serial.print(cntFile); Serial.println("]"); 
	fileAsciiDump(cntFile, 0); 
	Serial.println("\n");

	// Dump lines from existing log files
	char fname[15];
	char num[11];
	for(int i=0 ; i<maxLogFileNum ; i++){
		strcpy(fname,logFileBName);
		strcat(fname, itoa(i,num,10)); 
		Serial.print("File:["); Serial.print(fname); Serial.println("]"); 

		fileAsciiDump(fname, 0); 
		Serial.println("\n");
	}
}



/*********************************************************************
 *					Private Functions (Module namespace only)
*********************************************************************/

/*
 *  Read CNT from file and returns CNT+1
 *  Save new CNT back to file
 */
static int getCntSuff(char * Fname){
	File file;
	int retCnt = 0;
	if (!file.open(Fname, O_READ)) { // file not found (first run)
		retCnt = 0;
	} else { // read "cnt" string from the file (file contains just one str)
		char line[10];
		int n = file.fgets(line, sizeof(line)) ;
		if(n>0) { // file not empty
			int cnt = atoi(trimInt(line));
			retCnt = ++cnt;
			if(retCnt >= maxLogFileNum) {
				retCnt = 0;
			}
		} else {
			retCnt = 0;
		}
		file.close();
	}
	// Update CNT file content with new CNT value
	writeCntToFile(retCnt);

	return(retCnt);
}

/*
 *  Write into "Fname" the "rotated" LOG file name to be used 
 */
static char * setLogFileName(char * Fname){

	// Get CNT suff for LOG file to be created
	int cnt=getCntSuff(cntFile);

	// Open LOG file for writing
	char num[11];
	strcpy(Fname,logFileBName);
	// Warning: Non std itoa()
	// http://www.nongnu.org/avr-libc/user-manual/group__avr__stdlib.html
	strcat(Fname, itoa(cnt,num,10)); 
	return(Fname);
}

/*
 *  Open Counter File and write the new value in it (Truncate)
 */
static bool writeCntToFile(int cnt){
	File file;
	if (! file.open(cntFile, O_RDWR | O_CREAT | O_TRUNC)) {
		return(1);
	}
	file.println(cnt);
	file.close();
	return(0);
}

/*
 * LEFT trim strings containing INTEGERS
 *		Returns the pointer to the first NUMERIC char in str
 */
static char * ltrimInt(char *str) {
	char	*	s=str-1;
	while(*(++s)){
		if( (*s >= '0') && (*s) <= '9' ) {
			return(s);
		}
	}
	return(str);
}

/*
 * RIGHT trim strings containing INTEGERS
 *		Scan string backwards (from the end to the beginning) and Overwrite 
 *		NON NUMERIC chars with StringTerminator ('\0').
 *		Stops when find a NUMERIC char
*/
static char * rtrimInt(char *str) {
	int	l=strlen(str);
	char	*	s=str+l;

	while( ( *(--s) <= '0') || (*s >= '9' )    ) {
		*s = '\0';
	}
	return(str);
}

/*
 * TRIM strings containing INTEGERS
 *		Returns the pointer to the first NUMERIC char in str
 */
static char * trimInt(char *str) {
	rtrimInt(str);
	ltrimInt(str);
	return(str);
}

/*
 * Write to Serial the first "maxChar" chars of the "Fname" ASCII file
 * maxChar==0 means "whole file"
 */
void fileAsciiDump(char * Fname, int maxChar) {
	int i=0;
	File f;

	if (f.open(Fname, O_READ)) { 
		while (f.available()) {
			if((maxChar!=0) && (i++ > maxChar)){
				f.close();
				return;
			}
			Serial.write(f.read());
		}
		f.close();
	} else {
		Serial.println(" File not found.");
	}
}




