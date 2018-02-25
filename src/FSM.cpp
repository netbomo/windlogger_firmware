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
	nextState = &FSM::st_SETUP;
}

FSM::~FSM() {
	// TODO Auto-generated destructor stub
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

void FSM::st_SLEEP(){
#ifdef DEBUG_FSM
	Serial.println("st_SLEEP");
#endif
	digitalWrite(LED_BUILTIN,HIGH);			// led off
	delay(1000);
	digitalWrite(LED_BUILTIN,LOW);

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
	digitalWrite(LED_BUILTIN,HIGH);			// led off
	delay(1000);
	digitalWrite(LED_BUILTIN,LOW);

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
