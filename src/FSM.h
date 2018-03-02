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


#ifndef FSM_H_
#define FSM_H_

//#define DEBUG_FSM

#include "Arduino.h"

#include "Rtc_Pcf8563.h"

class FSM;

typedef void (FSM::*_nextState)(void);		/**< This typedef define a method pointer type */


/**
 * \brief The class FSM regroup hardware peripherals declaration, mechanic methods and useful parameters.
 */
class FSM {
public:
	/******************************************************************************
	 * Constructor and destructor definition
	 */
	FSM();
	virtual ~FSM();
	/******************************************************************************
	 * State machine mechanic methods
	 */
	/**
	 * \brief This method initialize the state machine
	 */
	void init();
	/**
	 * \brief This method is used to test each second and decide if the flag_measure need to be pull-up
	 */
	void timingControl();
	/******************************************************************************
	 * State pointer
	 */
	_nextState nextState = NULL;			/**< This method pointer is use to manipulate the next state function */

	/******************************************************************************
	 * Public flags
	 */
	static bool flag_frequenciesReady;		/**< Set when the timer3 overflow, data are ready*/
	static bool flag_configRequest;			/**< set when a string from usart0 finish by "\r" */
	static bool flag_measure;				/**< set when time come for a new measure */

	/******************************************************************************
	 * transitions list
	 * Each transition for a next state is organize in simple setter method, that try to make more clean code
	 */
	inline void ev_isWaiting(){nextState = &FSM::st_SLEEP;}						///< The nextState is set to st_SLEEP */
	inline void ev_testCounter(){nextState = &FSM::st_CALC_AVERAGES;}			///< The nextState is set to st_CALC_AVERAGES */
	inline void ev_transmitting(){nextState = &FSM::st_OUTPUT;}					///< The nextState is set to st_OUTPUT */
	inline void ev_configRequest(){nextState =&FSM::st_CONFIG;}					///< The nextState is set to st_CONFIG */
	inline void ev_measure(){nextState = &FSM::st_MEASURE;}						///< The nextState is set to st_MEASURE */
	inline void ev_frequenciesReady(){nextState = &FSM::st_READ_FREQUENCIES;}	///< The nextState is set to st_READ_FREQUENCIES */

	/******************************************************************************
	 * State list declaration
	 */
	void st_SETUP();						///< this state is used at the start time */
	void st_CONFIG();						///< this state is used to download or upload configuration */
	void st_MEASURE();						///< this state is used for measurement process */
	void st_READ_FREQUENCIES();				///< This state is used to read frequencies from anemometers or RPMs sensors */
	void st_CALC_AVERAGES();				///< This state is used to calc averages if the max measures is done */
	void st_OUTPUT();						///< this state is used to save or send data */
	void st_SLEEP();						///< When nothing is need to do, the micro-controller go to sleep */

	/******************************************************************************
	 * Configuration management
	 ******************************************************************************/
	/// This method a to serialString the ch character
	inline void addChar(char ch){	serialString[StrIndex]=ch;	StrIndex++;	}

	/// This method print the main menu
	void FSM::menu();
	/**
	 * The config method permit to update private members
	 * @param stringConfig This string contain the ID parameter and the value as "id=value"
	 * @return If the id or value is wrong return 1 else 0.
	 */
	bool config(char *stringConfig);

	/// The printConfig method print all parameters and values on Serial (uart0)
	void printConfig();

	/**
	 * The configDT method permit to update Date and Time
	 * @param stringConfig This string contain the ID parameter and the value as "id=value"
	 * string for time needs to be as : "*21=hh:mm:ss"
	 * string for date needs to be as : "*22=mm/dd/yyyy"
	 */
	void configDT(char *stringConfig);

	/// Thismethod print the date and the time on the serial(uart0)
	void printDateTime();

	/******************************************************************************
	 * FSM parameters in EEPROM management
	 ******************************************************************************/
	/// Load saved parameters for the FSM from the eeprom
	void load_param();

	/// Update saved parameters for the FSM in the eeprom
	void update_param();

	/// \brief Initialize the eeprom memory
	void initialize_param();

private:
	/******************************************************************************
	 * params saved in eeprom
	 ******************************************************************************/
	const unsigned char DATA_STRUCTURE_VERSION = 201;
	unsigned long structure_version;	/**< This permit to improve the structure by auto reset eeprom data when the structure evolve to prevent data bad reading */
	unsigned char node_id;				/**< The node id, permit identify each datalogger (0 - 255)*/
	unsigned char measureSampleConf;	/**< Measurement sampling (0: no measure, 1 : 10 secs, 2: 1 min, 3: 10 min...) */
	unsigned char measureMax;			/**< Measure_max is the number of measure by measure_sample_conf (ex by minute or by 10 minutes...). !Be careful! is a new configuration is create, the Sensor::MAX_DATA_SAMPLE needs to be adjust to the highest value of measure_max! */

	/******************************************************************************
	 * Private FSM members
	 ******************************************************************************/
	unsigned char m_eeprom_addr;		/**< Adress were fsm data are stored in eeprom */
	unsigned char isInConfig;			/**< This is a flag to stay in config mode */
	unsigned char measure;				/**< This is a measure counter, to store data at the correct place and compare with the measureMax value */
	unsigned char secondOld;			/**< Store the old second value to made timing test only each second */
	unsigned char second_counter;		/**< The second_counter is used to compare to the measurePeriode interval */
	unsigned char measurePeriode;		/**< measurePeriode is the interval between two measures in seconds */


	char serialString[64]={'a'};		/**< This string is used for character test and is pass to the config state when a '\r' char comes */
	unsigned char StrIndex=0;			/**< This is serialString index */


	/******************************************************************************
	 * Hardware interface
	 ******************************************************************************/
	Rtc_Pcf8563 rtc;					/**< Real Time Clock instance */
};

#endif /* FSM_H_ */
