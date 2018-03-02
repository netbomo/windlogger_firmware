#include "Arduino.h"
#include "src/FSM.h"

//#define DEBUG_MAIN

FSM fsm;		// define a FSM
//The setup function is called once at startup of the sketch
void setup()
{
// Add your initialization code here
	Serial.begin(9600);
	Serial.println("Initialization windlogger v0.2");
	pinMode(LED_BUILTIN,OUTPUT);			// initialize the LED_BUILDING as output (pin 13)
	digitalWrite(LED_BUILTIN,LOW);			// led off
	fsm.init();

}

// The loop function is called in an endless loop
void loop()
{
#ifdef DEBUG_MAIN
	//Serial.println("Main function...");
#endif

//Add your repeated code here
	// hardware management
	// Serial input processing
	while (Serial.available())
	 {
		char ch=Serial.read();		// copy the char in ch
	 switch(ch)
	   {
	   case '\r':					// if carriage return, pull-up the flag.confgRequest flag
#ifdef DEBUG_MAIN
		   Serial.print("CR");
#endif
	     fsm.flag_configRequest = true;
	     break;
	   case '\n':					// if it's new line, just do nothing
		  //
	     break;
	   default:
#ifdef DEBUG_MAIN
		   Serial.print(ch);
#endif
	     fsm.addChar(ch);			// else, add char to the configRequest
	   }
	 }

	// Check timing :
#ifdef DEBUG_MAIN
		   Serial.print("check timing");
#endif
	fsm.timingControl ();	// check if time comes to do a new measure or not



	// execute state, each next state is decide in the current state
	(fsm.*(fsm.nextState))();
}
