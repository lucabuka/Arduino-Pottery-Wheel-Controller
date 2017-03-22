
#ifndef _POTTERYWHEELCONTROLLER_LOGMODULE_H
#define _POTTERYWHEELCONTROLLER_LOGMODULE_H

/*
 *  SDFAT lib :  pio lib install 1432                                            
 */ 

#include <SPI.h>
#include <SdFat.h>

// LOG types: First "field" in log file's line
#define LOG_SYSTEM_INFO			'I'
#define LOG_SYSTEM_OVERLOAD	'O'
#define LOG_USER_ACTION			'U'
#define LOG_SYSTEM_ACTION		'A'


// Public f()
bool LOG_init(int cs_pin);
bool LOG_open(bool toSerial);
char * LOG_write(char Id, double d, unsigned long l, int i, char * s, bool sync); 
void LOG_printDebugInfo();


// Privates f()
static bool writeLog(char * str);
static int getCntSuff(char * lastCntFile); 
static char * setLogFileName(char * destFname);                                     
static bool writeCntToFile(int cnt);
static void printDirectory(File dir, int numTabs);
static char * ltrimInt(char *str);                                              
static char * rtrimInt(char *str);                                              
static char * trimInt(char *str);  
void fileAsciiDump(char * Fname, int maxChar);

#endif
