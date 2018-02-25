#include "Arduino.h"
#include "src/FSM.h"

FSM fsm;		// define a FSM
//The setup function is called once at startup of the sketch
void setup()
{
// Add your initialization code here
	Serial.begin(9600);
	Serial.println("Initialization");
	pinMode(LED_BUILTIN,OUTPUT);			// initialize the LED_BUILDING as output (pin 13)
	digitalWrite(LED_BUILTIN,LOW);			// led off

}

// The loop function is called in an endless loop
void loop()
{
#ifdef DEBUG_MAIN
	Serial.println("Main function...");
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
	     Serial.print("CR");
	     break;
	   case '\n':
		  //
	     break; // ignore
	   default:
	     Serial.print(ch);
	   }
	 }


	// execute state
	(fsm.*(fsm.nextState))();


}
