/**
 *******************************************************************************
 *******************************************************************************
 *
 *	License :
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *     any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *******************************************************************************
 *******************************************************************************
 *
 *
 *    @file   FSM.h
 *    @author gilou
 *    @date   19 févr. 2018
 *    @brief  The FSM is the finish state machine mechanism.
 *
 *    This is the Final State Machine organization
 *
 */


#include "Arduino.h"
#include "FSM.h"

// hardware include
#include "Rtc_Pcf8563.h"
#include "SD.h"

// sensor include
#include "Anemometer.h"
#include "Windvane.h"

Anemometer anemo1(0);	// Anemo1 is collected in the timer0
Anemometer anemo2(1);	// Anemo2 is collected in the timer1
Windvane vane(6);		// The windvane is connected to the ADC6

/******************************************************************************
 * Constructor and destructor definition
 */
FSM::FSM() {


}	// FSM constructor


FSM::~FSM() {}	// FSM destructor

/******************************************************************************
 * Public flags
 */
bool FSM::flag_configRequest = false;			/**< initialize the flag_configRequest to false */
bool FSM::flag_frequenciesReady = false;		/**< initialize the flag_frequenciesReady to false */
bool FSM::flag_measure = false;					/**< initialize the flag_measure to false */

/******************************************************************************
 * State machine mechanic methods
 */
/// todo bug in the FSM, if no anemo, never the averages are calculated... loop around st_measure
// this method initialize the FSM
void FSM::init(){
	m_eeprom_addr = 0;				// data are store from the 0 adress
	nextState = &FSM::st_SETUP;		// the first state is : st_setup

	load_param();					// load FSM param from EEPROM

	rtc.getDateTime();				// upload date and time from the RTC

	anemo1.load_param();
	anemo2.load_param();

	vane.load_param();

	power1.load_param();
	power2.load_param();

	sd_init = false;

}

void FSM::timingControl(){
	// update local variables
	rtc.getTime();

	if(secondOld!=rtc.getSecond()){				// is it a new second ?
		second_counter++;						// Use a second counter to compare with the measurePeriode

		if((second_counter%measurePeriode)==0)
		{
			//Serial.print(measurePeriode); Serial.println("new measure");
			flag_measure = true;				// if it's true, we can do a new measure
			second_counter=0;					// reset the counter
		}
	secondOld = rtc.getSecond();		// update timestamp_old
	}

	// get anemo flag value
	flag_frequenciesReady = anemo1.flag_anemo();
	//Serial.print(anemo1.flag_anemo());
}

/******************************************************************************
 * Configuration management
 ******************************************************************************/
void FSM::menu(){
	Serial.println("Configuration menu :");
	Serial.println("	$1 - FSM");
	Serial.println("	$2 - Date/Time");
	Serial.println("	$3 - Anemo1");
	Serial.println("	$4 - Anemo2");
	Serial.println("	$5 - Windvane");
	Serial.println("	$6 - Power1");
	Serial.println("	$7 - Power2");

	Serial.println("	$9 - Output configuration");
}

void FSM::printConfig(){
	Serial.println("FSM config stuff :");
	Serial.print("	*11 measure_sample_conf = ");	Serial.print(measureSampleConf);	Serial.println("		0: no measure,1: 10s average,2:1min average,3:10min average.");
	Serial.print("	*12 node id = ");	Serial.print(node_id);	Serial.println("		permit identify each datalogger (0 - 255).");
	Serial.print("	*19 Reset configuration - reinitialize the FSM and each sensors configuration.");
}

void FSM::printDateTime(){
	Serial.println("Date/Time config :");
	Serial.print("	*21 Time : ");	Serial.println(rtc.formatTime());
	Serial.print("	*22 Date : ");	Serial.println(rtc.formatDate());
}

bool FSM::config(char *stringConfig){
	uint8_t item = stringConfig[2]-'0';	// convert item in char

	double arg_f = atof(stringConfig + 4);	// convert the second part, the value in double to cover all possibilities.
	unsigned char arg_uc = (unsigned char)arg_f;
	switch (item) {
		case 1:	// choose measurement periode parameter
			switch (arg_uc) {
			case 0:	// no measurement, use at the first wake up
				measureSampleConf = 0;measureMax = 0;measurePeriode = 0;
				update_param();	measure = 0; second_counter = 0;
				break;
			case 1:	// Config 1 : 2 measures in 10 secondes
				measureSampleConf = 1;measureMax = 2;measurePeriode = 5;
				update_param();	measure = 0; second_counter = 0;
				break;
			case 2:	// Config 2 : 4 measures in 1 minute
				measureSampleConf = 2;measureMax = 4;measurePeriode = 15;
				update_param();	measure = 0; second_counter = 0;
				break;
			case 3:	// Config 3 : 10 measures in 10 minutes
				measureSampleConf = 3;measureMax = 10;measurePeriode = 60;
				update_param();	measure = 0; second_counter = 0;
				break;
			default:
				Serial.println("Not a correct value");
				break;
			}
		break;
		case 2:	// Set the Node id number
			node_id = arg_uc;
			update_param();
			break;
		case 9:	// Reset each param,add here each sensors initialise_param method
			initialize_param();		// initialize the FSM
			anemo1.initialize_param();
			anemo2.initialize_param();
			vane.initialize_param();
			power1.initialize_param();
			power2.initialize_param();
			break;
		default:
			Serial.print("Bad request : ");Serial.println(item);
	}

	return 0;
}

void FSM::configDT(char *stringConfig){
	// valid string are :
	//	*21=hh:mm:ss
	//	*22=mm/dd/yyyy
	//	else bad request
	uint8_t item = stringConfig[2]-'0';	// convert item in char

	switch (item) {
		case 1:	//	*21=hh:mm:ss
			if(stringConfig[6]==':'&&stringConfig[9]==':'){	// test time separator
				char hours[3]={stringConfig[4],stringConfig[5],'\0'};
				char mins[3]={stringConfig[7],stringConfig[8],'\0'};
				char secs[3]={stringConfig[10],stringConfig[11],'\0'};

				rtc.setDateTime(rtc.getDay(), rtc.getWeekday(), rtc.getMonth(), 0,  rtc.getYear(), atoi(hours), atoi(mins), atoi(secs));
			}
			else Serial.print("Bad value : type *21=hh:mm:ss");
			break;
		case 2://	*22=mm/dd/yyyy
			if(stringConfig[6]=='/'&&stringConfig[9]=='/'){	// test date separator
				char months[3]={stringConfig[4],stringConfig[5],'\0'};
				char days[3]={stringConfig[7],stringConfig[8],'\0'};
				char years[3]={stringConfig[12],stringConfig[13],'\0'};

				rtc.setDateTime(atoi(days), rtc.getWeekday(), atoi(months), 0, atoi(years), rtc.getHour(), rtc.getMinute(), rtc.getSecond());
			}
			else Serial.print("Bad value : type *22=mm/dd/yyyy");
			break;
		default:
			Serial.print("Bad request : ");Serial.print(item);
			break;
	}
}

void FSM::configOutput(char *stringConfig){
	uint8_t item = stringConfig[2]-'0';	// convert item in char

	double arg_f = atof(stringConfig + 4);	// convert the second part, the value in double to cover all possibilities.
	unsigned char arg_uc = (unsigned char)arg_f;
	switch (item) {
		case 1:	// enable or disable write data on Serial
			if(arg_uc==0)serial_enable = false;	// disable
			else serial_enable = true;				// enable
			update_param();
		break;
		case 2:	// enable or disable write data on SD card
			if(arg_uc==0)sd_enable = false;	// disable
			else sd_enable = true;				// enable
			update_param();
			break;
//			case 3:	// Set offset value
//				m_offset = arg_f;
//				update_param();
//				break;
		default:
			Serial.print("Bad request : ");Serial.println(item);
	}
}

void FSM::printOutput(){
	Serial.println("Output config :");
	Serial.print("	*91 Serial enable : ");	Serial.println(serial_enable);
	Serial.print("	*92 Sd card enable : ");	Serial.println(sd_enable);


}

/******************************************************************************
 * State list declaration
 */
void FSM::st_SETUP(){
#ifdef DEBUG_FSM
	Serial.println("st_SETUP");
#endif

	// by default the transition is ev_waiting
	ev_isWaiting();
}

void FSM::st_CONFIG(){

#ifdef DEBUG_FSM
	Serial.println("st_CONFIG");
#endif

	if(flag_configRequest==true){
		Serial.println(serialString);

		switch (serialString[0]) {
			case '$':
				// menu config
				switch (serialString[1]) {
					case '$':
						isInConfig = 1;		// set config menu index to main
						menu();
						break;
					case '1':
						printConfig();
						break;
					case '2':
						printDateTime();
						break;
					case '3':
						anemo1.print_config();
						break;
					case '4':
						anemo2.print_config();
						break;
					case '5':
						vane.print_config();
						break;
					case '6':
						power1.print_config();
						break;
					case '7':
						power2.print_config();
						break;

					case '9':
					printOutput();
					break;
					case 'q':
						Serial.println("Config Done and start measurement.");
						isInConfig = 0;	// go out menu and config
						measure = 0; second_counter = 0;	// reset measure
						break;
					default:
						Serial.println("Bad request");
						break;
				}
				break;
			case '*':
				// Sub Config sender
				switch (serialString[1]) {
					case '1':
						config(serialString);
						printConfig();
						break;
					case '2':
						configDT(serialString);
						printDateTime();
						break;
					case '3':
						anemo1.config(serialString);
						anemo1.print_config();
						break;
					case '4':
						anemo2.config(serialString);
						anemo2.print_config();
						break;
					case '5':
						vane.config(serialString);
						vane.print_config();
						break;
					case '6':
						power1.config(serialString);
						power1.print_config();
						break;
					case '7':
						power2.config(serialString);
						power2.print_config();
						break;

					case '9':
						configOutput(serialString);
						printOutput();
						break;
					default:
						Serial.println("Bad request");
						break;
				}
				break;
			default:
				Serial.println("Bad request");
				break;
		}
		StrIndex=0;
		flag_configRequest = false;

		if(!(serialString[0]=='$' && serialString[1]=='q' ))	//if request is not quit
				Serial.println("Main menu : $$ or Quit and start measurement : $q");
	}


	// Transition test ?
	if(flag_configRequest || isInConfig>0) ev_configRequest();
	else ev_isWaiting();	// by default the transition is ev_waiting

}

void FSM::st_SLEEP(){
#ifdef DEBUG_FSM
	//Serial.println("st_SLEEP");
#endif


	// Transition test ?
	if(flag_configRequest) ev_configRequest();
	else if(flag_measure) {
		ev_measure();
	}
	else if(flag_frequenciesReady)ev_frequenciesReady();
	else ev_isWaiting();	// by default the transition is ev_waiting
}

void FSM::st_MEASURE(){

#ifdef DEBUG_FSM
	Serial.println("st_MEASURE");
#endif
	digitalWrite(LED_BUILTIN,HIGH);			// led on

	anemo1.start();	// start anemo1 and 2

	vane.read_value(measure);				// read the windvane value

	power1.read_value(measure, 4, 300);		// read power value
	power2.read_value(measure, 4, 300);


	digitalWrite(LED_BUILTIN,LOW);			// led off

	// reset the flag
	flag_measure = false;
	measure++;				// increase measure

	// Transition test ?
	if(flag_configRequest) ev_configRequest();
	else if(flag_frequenciesReady)ev_frequenciesReady();
	else ev_isWaiting();	// by default the transition is ev_waiting
}

void FSM::st_READ_FREQUENCIES(){
#ifdef DEBUG_FSM
	Serial.println("st_READ_FREQUENCIES");
#endif

	anemo1.read_value(measure);
	anemo2.read_value(measure);

	flag_frequenciesReady = false;

	// Transition test ?
	if(flag_configRequest) ev_configRequest();
	else ev_testCounter();
}

void FSM::st_CALC_AVERAGES(){
#ifdef DEBUG_FSM
	Serial.println("st_CALC_AVERAGES");
#endif

	// test measure number, if equal measureMax, it's time to made average
	bool isMeasureMax = false;
	if(measure == measureMax){
		isMeasureMax = true;

		anemo1.calc_average(measureMax);
		anemo2.calc_average(measureMax);
		vane.calc_average(measureMax);
		power1.calc_average(measureMax);
		power2.calc_average(measureMax);

		timestamp = rtc.getTimestamp();	// save average's timestamp
		measure = 0;	// restart a new sequence

		anemo1.clear(measureMax);
		anemo2.clear(measureMax);
		vane.clear(measureMax);
		power1.clear(measureMax);
		power2.clear(measureMax);
	}

	// Transition test ?
	if(flag_configRequest) ev_configRequest();
	else if(isMeasureMax)ev_transmitting();
	else ev_isWaiting();
}

void FSM::st_OUTPUT(){
#ifdef DEBUG_FSM
	Serial.println("st_OUTPUT");
#endif
	bool isTransmitting = true;

	if(serial_enable==true){
		char dataString[50];

		// print on Serial (uart 0)
		Serial.print(timestamp); Serial.print("	");
		Serial.print(node_id); Serial.print("	");
		Serial.print(anemo1.get_average()); Serial.print("	");
		Serial.print(anemo2.get_average()); Serial.print("	");
		Serial.print(vane.get_average());	Serial.print("	");
		Serial.print(power1.get_average(dataString));	Serial.print("	");
		Serial.print(power2.get_average(dataString));	Serial.print("	");

		Serial.println();
	}

	if(sd_enable==true){
		// todo improve with SD card detection, SD_CD on pin 3
		char tempString[50];
		char dataString[200];

		strcpy(dataString, ltoa(timestamp, tempString, 10)); strcat(dataString,"	");
		strcat(dataString,itoa(node_id, tempString, 10)); strcat(dataString,"	");
		strcat(dataString,dtostrf(anemo1.get_average(), 1, 1, tempString)); strcat(dataString,"	");
		strcat(dataString,dtostrf(anemo2.get_average(), 1, 1, tempString)); strcat(dataString,"	");
		strcat(dataString,itoa(vane.get_average(), tempString, 10)); strcat(dataString,"	");
		strcat(dataString,power1.get_average(tempString)); strcat(dataString,"	");
		strcat(dataString,power2.get_average(tempString)); strcat(dataString,"	");

		if(sd_init==false){	// if sd is not initialize, do it
			Serial.print("Initializing SD card...");
			// make sure that the default chip select pin is set to
			// output, even if you don't use it:
			pinMode(10, OUTPUT);

			// see if the card is present and can be initialized:
			if (!SD.begin(SD_CS)) {
			Serial.println("Card failed, or not present");
			// don't do anything more:
			sd_init=false;
			}
			else {
				Serial.println("card initialized.");
				sd_init = true;
			}
		}

		if(sd_init==true)	// sd card is initialize, write on
		{
			// set file name from month and day
			char fileName[13]; char tempConv[6];
			strcpy(fileName,itoa(rtc.getMonth(),tempConv,10));
			strcat(fileName,itoa(rtc.getDay(),tempConv,10));
			strcat(fileName,".txt");

			// open the file. note that only one file can be open at a time,
			// so you have to close this one before opening another.
			File dataFile = SD.open(fileName, FILE_WRITE);

			// if the file is available, write to it:
			if (dataFile) {
			dataFile.println(dataString);
			dataFile.close();
			// print to the serial port too:
			//Serial.println(dataString);
			}
			// if the file isn't open, pop up an error:
			else {
			Serial.println("error opening datalog.txt");
			}
		}

	}

	isTransmitting = false;

	// Transition test ?
	if(isTransmitting) ev_transmitting();
	else ev_isWaiting();
}

/******************************************************************************
 * Eeprom management
 */
// Load saved config data from the eeprom
void FSM::load_param(){
	structure_version = eeprom_read_byte((const unsigned char*)m_eeprom_addr);
	if(structure_version != DATA_STRUCTURE_VERSION) initialize_param();
	else{
		node_id = eeprom_read_byte((const unsigned char*)m_eeprom_addr+5);
		measureSampleConf = eeprom_read_byte((const unsigned char*)m_eeprom_addr+7);
		measureMax = eeprom_read_byte((const unsigned char*)m_eeprom_addr+9);
		measurePeriode = eeprom_read_byte((const unsigned char*)m_eeprom_addr+11);

		if(eeprom_read_byte((const unsigned char*)m_eeprom_addr+13)==0) serial_enable = false;
		else serial_enable = true;
		if(eeprom_read_byte((const unsigned char*)m_eeprom_addr+15)==0) sd_enable = false;
		else sd_enable = true;

	}
}

//Update saved config data in the eeprom
void FSM::update_param (){
	eeprom_update_byte((unsigned char*)m_eeprom_addr, structure_version);
	eeprom_update_byte((unsigned char*)m_eeprom_addr+5, node_id);
	eeprom_update_byte((unsigned char*)m_eeprom_addr+7, measureSampleConf);
	eeprom_update_byte((unsigned char*)m_eeprom_addr+9, measureMax);
	eeprom_update_byte((unsigned char*)m_eeprom_addr+11, measurePeriode);

	if(serial_enable==false) eeprom_update_byte((unsigned char*)m_eeprom_addr+13, 0);	// store 0 for false
	else eeprom_update_byte((unsigned char*)m_eeprom_addr+13, 1);						// store 1 for true
	if(sd_enable==false) eeprom_update_byte((unsigned char*)m_eeprom_addr+15, 0);	// store 0 for false
	else eeprom_update_byte((unsigned char*)m_eeprom_addr+15, 1);						// store 1 for true

}

// Initialize the eeprom memory
void FSM::initialize_param (){
	eeprom_update_byte((unsigned char*)m_eeprom_addr, DATA_STRUCTURE_VERSION);
	eeprom_update_byte((unsigned char*)m_eeprom_addr+5, 15);
	eeprom_update_byte((unsigned char*)m_eeprom_addr+7, 0);
	eeprom_update_byte((unsigned char*)m_eeprom_addr+9, 0);
	eeprom_update_byte((unsigned char*)m_eeprom_addr+11, 0);
	eeprom_update_byte((unsigned char*)m_eeprom_addr+13, 1);
	eeprom_update_byte((unsigned char*)m_eeprom_addr+15, 0);

	load_param();
}
