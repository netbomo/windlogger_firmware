/*
 * FSM.h
 *
 *  Created on: 19 févr. 2018
 *      Author: gilou
 */

#ifndef FSM_H_
#define FSM_H_

//#define DEBUG_FSM

#include "Arduino.h"

class FSM;

typedef void (FSM::*_nextState)(void);		/**< This method pointer is use to manipulate the next state function */


/**
 *
 */
class FSM {
public:
	FSM();
	virtual ~FSM();

	/******************************************************************************
	 * State pointer
	 */
	_nextState nextState = NULL;

	/******************************************************************************
	 * Public flags
	 */
	static bool flag_frequenciesReady;
	static bool flag_configRequest;
	static bool flag_measure;

	/******************************************************************************
	 * transitions list
	 */
	inline void ev_isWaiting(){
		nextState = &FSM::st_SLEEP;
#ifdef DEBUG_FSM
		Serial.println("----ev_isWaiting");
#endif
	}

	inline void ev_testCounter(){
		nextState = &FSM::st_CALC_AVERAGES;
#ifdef DEBUG_FSM
		Serial.println("----ev_testCounter");
#endif
	}

	inline void ev_transmitting(){
		nextState = &FSM::st_OUTPUT;
#ifdef DEBUG_FSM
		Serial.println("----ev_testCounter");
#endif
	}

	inline void ev_configRequest(){
		nextState =&FSM::st_CONFIG;
#ifdef DEBUG_FSM
		Serial.println("----ev_ConfigRequest");
#endif
	}

	inline void ev_measure(){
		nextState = &FSM::st_MEASURE;
#ifdef DEBUG_FSM
		Serial.println("----ev_measure");
#endif
	}

	inline void ev_frequenciesReady(){
		nextState = &FSM::st_READ_FREQUENCIES;
#ifdef DEBUG_FSM
		Serial.println("----ev_frequenciesReady");
#endif
	}

	/******************************************************************************
	 * State list declaration
	 */
	void st_SETUP();
	void st_CONFIG();						/**< this state is used to download or upload configuration */
	void st_MEASURE();						/**< this state is used for measurement process */
	void st_READ_FREQUENCIES();				/**< This state is used to read frequencies from anemometers or RPMs sensors */
	void st_CALC_AVERAGES();					/**< This state is used to calc averages if the max measures is done */
	void st_OUTPUT();						/**< this state is used to save or send data */
	void st_SLEEP();							/**< When nothing is need to do, the micro-controller go to sleep */

	/******************************************************************************
	 * Configuration management
	 ******************************************************************************/
	/**
	 * The config methods permit to update private members
	 * @param stringConfig This string contain the ID parameter and the value as "id=value"
	 * @return If the id or value is wrong return 1 else 0.
	 */
	bool config(char *stringConfig);

	/**
	 * The printConfig method print all parameters and values on Serial (uart0)
	 */
	void printConfig();


	/******************************************************************************
	 * FSM parameters in EEPROM management
	 ******************************************************************************/
	/**
	 * \brief Load saved parameters for the FSM from the eeprom
	 * \return void
	 */
	void load_param();

	/**
	 * \brief Update saved parameters for the FSM in the eeprom
	 * \return void
	 */
	void update_param();

	/**
	 * \brief Initialize the eeprom memory
	 * \return void
	 */
	void initialize_param();

private:
	/******************************************************************************
	 * Private FSM members
	 ******************************************************************************/
	unsigned char measure_max = 4;

	unsigned char measure = 0;


};

#endif /* FSM_H_ */
