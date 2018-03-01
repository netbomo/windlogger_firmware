/*
 * FSM.cpp
 *
 *  Created on: 19 févr. 2018
 *      Author: gilou
 */


#include "Arduino.h"
#include "FSM.h"


bool FSM::flag_configRequest = false;
bool FSM::flag_frequenciesReady = false;
bool FSM::flag_measure = false;

FSM::FSM() {
	// TODO Auto-generated constructor stub

}

FSM::~FSM() {
	// TODO Auto-generated destructor stub
}

void FSM::init(){
	m_eeprom_addr = 0;
	nextState = &FSM::st_SETUP;

	load_param();	// load FSM param from EEPROM

	rtc.getDateTime();	// upload date and time from the RTC

}

bool FSM::config(char *stringConfig){
	uint8_t item = stringConfig[2]-'0';	// convert item in char

	double arg_f = atof(stringConfig + 4);	// convert the second part, the value in double to cover all possibilities.
	unsigned char arg_uc = (unsigned char)arg_f;
	switch (item) {
		case 1:	// choose measurement periode parameter
			switch (arg_uc) {
			case 0:	// no measurement, use at the first wake up
				measure_sample_conf = 0;measure_max = 0;measure_periode = 0;
				update_param();	measure = 0; second_counter = 0;
				break;
			case 1:	// Config 1 : 2 measures in 10 secondes
				measure_sample_conf = 1;measure_max = 2;measure_periode = 5;
				update_param();	measure = 0; second_counter = 0;
				break;
			case 2:	// Config 2 : 4 measures in 1 minute
				measure_sample_conf = 2;measure_max = 4;measure_periode = 15;
				update_param();	measure = 0; second_counter = 0;
				break;
			case 3:	// Config 3 : 10 measures in 10 minutes
				measure_sample_conf = 3;measure_max = 10;measure_periode = 60;
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
		default:
			Serial.print("Bad request : ");Serial.println(item);
	}

	return 0;
}

void FSM::printConfig(){
	Serial.println("FSM config stuff :");
	Serial.print("	*11 measure_sample_conf = ");	Serial.print(measure_sample_conf);	Serial.println("		0: not measure,1: 10s average,2:1min average,3:10min average.");
	Serial.print("	*12 node id = ");	Serial.print(node_id);	Serial.println("		permit identify each datalogger (0 - 255).");
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
			if(stringConfig[6]=='/'&&stringConfig[9]=='/'){	// test time separator
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


void FSM::printDateTime(){
	Serial.println("Date/Time config :");
	Serial.print("	*21 Time : ");	Serial.println(rtc.formatTime());
	Serial.print("	*22 Date : ");	Serial.println(rtc.formatDate());
}

void FSM::timingControl(){
	// If a new second comes
	FSM::rtc.getTime();

	unsigned char tempSec = rtc.getSecond();
	if(second_old!=tempSec){
		second_counter++;						// Use a second counter to compare with the measure_stamp
Serial.print(second_counter);	Serial.print("	");	Serial.println(measure_periode);
		if((second_counter%measure_periode)==0)
		{
			flag_measure = 1;				// if it's true, we can do a new measure
			second_counter=0;					// reset the counter
			Serial.print("Flag_measure = 1");
		}
	second_old = tempSec;		// update timestamp_old
	}
}

void FSM::st_SETUP(){
#ifdef DEBUG_FSM
	Serial.println("st_SETUP");
#endif

	digitalWrite(LED_BUILTIN,HIGH);			// led off
	delay(1000);
	digitalWrite(LED_BUILTIN,LOW);

	// by default the transition is ev_waiting
	ev_isWaiting();
}

void display(){
	Serial.println("Configuration menu :");
	Serial.println("	$1 - FSM");
	Serial.println("	$2 - Date/Time");
	Serial.println("	$3 - Anemo");
}

void FSM::st_CONFIG(){
#ifdef DEBUG_FSM
	Serial.println("st_CONFIG");
#endif

	if(flag_configRequest==true){

		Serial.println(serialStrIndex);
		Serial.println(serialString);

		switch (serialString[0]) {
			case '$':
				// display config
				switch (serialString[1]) {
					case '$':
						isInConfig = 1;		// set config menu index to main
						display();
						break;
					case '1':
						printConfig();
						break;
					case '2':
						printDateTime();
						break;
					case 'q':
						Serial.println("Config Doneand start measurement.");
						isInConfig = 0;	// go out menu and config
						break;
					default:
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
					default:
						break;
				}
				break;
			default:
				Serial.println("Bad request");
				break;
		}
		serialStrIndex=0;
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
	else if(flag_measure) ev_measure();
	else if(flag_frequenciesReady)ev_frequenciesReady();
	else ev_isWaiting();	// by default the transition is ev_waiting
}


void FSM::st_MEASURE(){
#ifdef DEBUG_FSM
	Serial.println("st_MEASURE");
#endif
	Serial.println("st_MEASURE");
	digitalWrite(LED_BUILTIN,HIGH);			// led off
	delay(500);
	digitalWrite(LED_BUILTIN,LOW);

	// reset the flag
	flag_measure = false;

	// Transition test ?
	if(flag_configRequest) ev_configRequest();
	else if(flag_frequenciesReady)ev_frequenciesReady();
	else ev_isWaiting();	// by default the transition is ev_waiting
}

void FSM::st_READ_FREQUENCIES(){
#ifdef DEBUG_FSM
	Serial.println("st_READ_FREQUENCIES");
#endif


	// Transition test ?
	if(flag_configRequest) ev_configRequest();
	else ev_testCounter();
}

void FSM::st_CALC_AVERAGES(){
#ifdef DEBUG_FSM
	Serial.println("st_CALC_AVERAGES");
#endif

	bool isMeasureMax = false;

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
		measure_sample_conf = eeprom_read_byte((const unsigned char*)m_eeprom_addr+7);
		measure_max = eeprom_read_byte((const unsigned char*)m_eeprom_addr+9);
		measure_periode = eeprom_read_byte((const unsigned char*)m_eeprom_addr+11);
	}
}

//Update saved config data in the eeprom
void FSM::update_param (){
	eeprom_update_byte((unsigned char*)m_eeprom_addr, structure_version);
	eeprom_update_byte((unsigned char*)m_eeprom_addr+5, node_id);
	eeprom_update_byte((unsigned char*)m_eeprom_addr+7, measure_sample_conf);
	eeprom_update_byte((unsigned char*)m_eeprom_addr+9, measure_max);
	eeprom_update_byte((unsigned char*)m_eeprom_addr+11, measure_periode);
}

// Initialize the eeprom memory
void FSM::initialize_param (){
	eeprom_update_byte((unsigned char*)m_eeprom_addr, DATA_STRUCTURE_VERSION);
	eeprom_update_byte((unsigned char*)m_eeprom_addr+5, 0);
	eeprom_update_byte((unsigned char*)m_eeprom_addr+7, 0);
	eeprom_update_byte((unsigned char*)m_eeprom_addr+9, 0);
	eeprom_update_byte((unsigned char*)m_eeprom_addr+9, 15);

	load_param();
}

