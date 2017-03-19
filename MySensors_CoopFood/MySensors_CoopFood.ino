/*
	MySensors CoopFood
	https://github.com/soif/MySensors_CoopFood
	Copyright 2017 François Déchery

	** Description **
	This Arduino Pro Mini (3.3V) based project is a [MySensors](https://www.mysensors.org/)  node which periodically watches and reports the level of food and the level of water, in my chicken coop.

	** Compilation **
		- needs MySensors version 2.11+
*/

// debug #######################################################################
#define OWN_DEBUG	// Comment/uncomment to remove/show debug (May overflow Arduino memory when set)
#define MY_DEBUG	// Comment/uncomment to remove/show MySensors debug messages (May overflow Arduino memory when set)


// Define ######################################################################
#define INFO_NAME "CoopFood"
#define INFO_VERS "2.11.00"

// MySensors
#define MY_RADIO_NRF24
#define MY_NODE_ID 211
#define MY_TRANSPORT_WAIT_READY_MS 5000	//set how long to wait for transport ready. in milliseconds
//#define MY_TRANSPORT_SANITY_CHECK
//#define MY_REPEATER_FEATURE

#define SLEEP_TIME		(30*60*1000ul) 		// sleep period
#define FORCE_TIME		(4*60*60*1000ul) 	// force report every

#define MIN_FOOD_DIST	3	// minimum food distance in cm (when full)
#define MAX_FOOD_DIST	150	// maximum food distance in cm (when empty)

#define WATER_INVERT	true	// floating switch logic (NC or NO)
#define WATER_PULLUP	true	// floating switch internal pullup
#define WATER_DEBOUNCE	50		// floating switch debounce time

#define CHILD_ID_FOOD	0
#define CHILD_ID_WATER	1

// includes ####################################################################
#include "own_debug.h"
#include <SPI.h>
#include <MySensors.h>

#include <NewPing.h>

// be SURE to use THIS Button Library
#include <Button.h>						// https://github.com/JChristensen/Button

// Pins ########################################################################
#define PIN_ONEWIRE		3		// OneWire Bus
#define PIN_US_TRIG		5		// Ultrasonic sensor : Trigger Pin
#define PIN_US_ECHO		6		// Ultrasonic sensor : Echo Pin
#define PIN_WATER		4		// Floating switch pin

// Variables ###################################################################
byte			last_food_level 	= 0;			// Ultrasonic level
//int				us_readings[POT_READ_COUNT];	// all potentiometer reading
boolean			last_water_state	= true;			//strip current anim state
boolean			init_msg_sent		=false;			//did we sent the init message?
boolean			force_report		=true;			//force report

unsigned long	last_report_time			=millis();

NewPing 		sonarFood(PIN_US_TRIG, PIN_US_ECHO, MAX_FOOD_DIST + 5 );
Button 			buttWater(PIN_WATER, WATER_PULLUP, WATER_INVERT, WATER_DEBOUNCE);

MyMessage 		msgFood		(	CHILD_ID_FOOD,	V_PERCENTAGE);
MyMessage 		msgWater	(	CHILD_ID_WATER,	V_STATUS);


// #############################################################################
// ## MAIN  ####################################################################
// #############################################################################

// --------------------------------------------------------------------
void before() {
	DEBUG_PRINTLN("");
	DEBUG_PRINTLN("+++++Before START+++++");

	// Setup Pins -----------------------
	pinMode(PIN_US_TRIG,	OUTPUT);
	pinMode(PIN_US_ECHO,	INPUT);
	pinMode(PIN_WATER,		INPUT);

	DEBUG_PRINTLN("+++++Before END  +++++");
}

// --------------------------------------------------------------------
void setup() {
	DEBUG_PRINTLN("");
	DEBUG_PRINTLN("----Setup START-------");

	DEBUG_PRINTLN("----Setup END  -------");
}


// --------------------------------------------------------------------
void loop() {
	SendInitialtMsg();
	if(init_msg_sent){

		if(millis() > last_report_time + FORCE_TIME ){
			force_report=true;
			last_report_time=millis();
		}

		reportsFood();
		reportsWater();
		reportsBattery();

		force_report=false;
		sleep(SLEEP_TIME);
	}
}



// FUNCTIONS ###################################################################

// --------------------------------------------------------------------
void SendInitialtMsg(){
	if (init_msg_sent == false && isTransportReady() ) {
	   	init_msg_sent = true;
   	}
}

// --------------------------------------------------------------------
void presentation(){
	DEBUG_PRINTLN("*** Presentation START ******");
	sendSketchInfo(INFO_NAME ,	INFO_VERS );
	present(CHILD_ID_FOOD,		S_RGB_LIGHT);
	present(CHILD_ID_WATER,		S_DIMMER);
	DEBUG_PRINTLN("*** Presentation END ******");
}

// --------------------------------------------------------------------
void receive(const MyMessage &msg){
}

// --------------------------------------------------------------------
void reportsFood(){
	wait(100);
	int distance= sonarFood.ping_cm();
	byte level=map(distance, MIN_FOOD_DIST, MAX_FOOD_DIST, 100, 0);

	DEBUG_PRINT("# Reading Food : ");
	DEBUG_PRINT(distance);
	DEBUG_PRINT(" cm , ");
	DEBUG_PRINT(level);
	DEBUG_PRINT("% ( last = ");
	DEBUG_PRINT(last_food_level);
	DEBUG_PRINTLN("% )");

	if(level != last_food_level || force_report){
		DEBUG_PRINTLN(" ==> Reporting !");
		send(msgFood.set(level), false);
		last_food_level = level;
	}
}

// --------------------------------------------------------------------
void reportsWater(){
	buttWater.read();
	wait( WATER_DEBOUNCE + 5 );
	buttWater.read();
	boolean state= buttWater.isPressed();

	DEBUG_PRINT("# Reading Water : ");
	DEBUG_PRINT(state);
	DEBUG_PRINT(" ( last = ");
	DEBUG_PRINT(last_water_state);
	DEBUG_PRINTLN(" )");

	if(state != last_water_state || force_report){
		DEBUG_PRINTLN(" ==> Reporting !");
		send(msgWater.set(state), false);
		last_water_state =state;
	}
}

// --------------------------------------------------------------------
void reportsBattery(){
	//sendBatteryLevel(level,false);
}
