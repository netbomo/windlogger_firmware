#include "Arduino.h"
#include "src/FSM.h"

//#define DEBUG_MAIN

FSM fsm;		// define a FSM
//The setup function is called once at startup of the sketch
void setup()
{
// Add your initialization code here
	Serial.begin(9600);
	Serial.println("Initialization");
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
		char ch=Serial.read();
	 switch(ch)
	   {
	   case '\r':
#ifdef DEBUG_MAIN
		   Serial.print("CR");
#endif
	     fsm.flag_configRequest = true;
	     break;
	   case '\n':
		  //
	     break; // ignore
	   default:
#ifdef DEBUG_MAIN
		   Serial.print(ch);
#endif
	     fsm.addChar(ch);	// add char to config request
	   }
	 }

	// check timing
#ifdef DEBUG_MAIN
		   Serial.print("check timing");
#endif
	fsm.timingControl ();	// check if time comes to do a new measure or not



	// execute state, each next state is decide in the current state
	(fsm.*(fsm.nextState))();
}
