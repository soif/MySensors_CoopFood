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
//#define MY_DEBUG_VERBOSE
//#define MY_DEBUG_VERBOSE_RF24


// Define ######################################################################
#define INFO_NAME "CoopFood"
#define INFO_VERS "2.11.00"

// MySensors
#define MY_RADIO_NRF24
#define MY_NODE_ID 		211

//https://forum.mysensors.org/topic/5778/mys-library-startup-control-after-onregistration/7
#define MY_TRANSPORT_WAIT_READY_MS				(	5*1000ul)	// how long to wait for transport ready at boot
#define MY_TRANSPORT_SANITY_CHECK								// check if transport is available
#define MY_TRANSPORT_SANITY_CHECK_INTERVAL_MS	(15*60*1000ul)	// how often to  check if transport is available (already set as default)
#define MY_TRANSPORT_TIMEOUT_EXT_FAILURE_STATE	(5*	60*1000ul)	//  how often to reconnect if no transport
//#define MY_REPEATER_FEATURE										// set as Repeater

#define SLEEP_TIME	(	60*60*1000ul) 	// sleep period
//#define SLEEP_TIME		(	2*1000ul) 		// every 2 sec
#define FORCE_REPORT	6 					// force report every X cycles

#define MIN_FOOD_DIST	9	// minimum food distance in cm (when full)
#define MAX_FOOD_DIST	135	// maximum food distance in cm (when empty)

#define WATER_INVERT	false	// floating switch logic (NC or NO)
#define WATER_PULLUP	true	// floating switch internal pullup
#define WATER_DEBOUNCE	50		// floating switch debounce time

//For 2 rechargeable AA  batteries min voltage is 2V and max is 2.7V
#define MIN_BAT		594		// = 2.0V  / 0.003363075
#define MAX_BAT		828		// = 2.7V / 0.003363075
#define BAT_READS	100  	// read battery this number of time

#define CHILD_ID_FOOD	0
#define CHILD_ID_WATER	1
#define CHILD_ID_TEMP	2

// includes ####################################################################
#include "own_debug.h"

// MySensors
#include <SPI.h>
#include <MySensors.h>

// ultrasonic lib
#include <NewPing.h>

// dallas temperature
#include <OneWire.h>
#include <DallasTemperature.h>

// be SURE to use THIS Button Library
#include <Button.h>				// https://github.com/JChristensen/Button

// Pins ########################################################################
#define PIN_BATTERY		A0		// Battery measurement
#define PIN_ONEWIRE		3		// OneWire (DS18B20) Bus
#define PIN_WATER		2		// Floating switch pin

#define PIN_US_POWER	A4		// Ultrasonic sensor : VCC Pin
#define PIN_US_TRIG		6		// Ultrasonic sensor : Trigger Pin
#define PIN_US_ECHO		5		// Ultrasonic sensor : Echo Pin

// Variables ###################################################################
boolean				init_msg_sent		=false;			//did we sent the init message?

unsigned int		last_battery_value 	= 0;			// LAST battery read value
byte				last_battery_level 	= 0;			// LAST battery level
byte				last_food_level 	= 0;			// LAST food level
boolean				last_water_state	= true;			// LAST Water state
float				last_temp_value		= -1000;		// LAST temperature value

unsigned int		cycles_count			=0;
boolean				force_report		=true;			//force report

NewPing 			sonarFood(PIN_US_TRIG, PIN_US_ECHO, MAX_FOOD_DIST + 5 );
Button 				buttWater(PIN_WATER, WATER_PULLUP, WATER_INVERT, WATER_DEBOUNCE);
OneWire 			oneWire(PIN_ONEWIRE);
DallasTemperature 	dallas(&oneWire);

MyMessage 			msgFood		(	CHILD_ID_FOOD,	V_DISTANCE);
MyMessage 			msgWater	(	CHILD_ID_WATER,	V_TRIPPED);
MyMessage 			msgTemp		(	CHILD_ID_TEMP,	V_TEMP);

// #############################################################################
// ## MAIN  ####################################################################
// #############################################################################

// --------------------------------------------------------------------
void before() {
	DEBUG_PRINTLN("");
	DEBUG_PRINTLN("+++++Before START+++++");

	// Setup Pins -----------------------
	pinMode(PIN_BATTERY,	INPUT);
	pinMode(PIN_US_POWER,	OUTPUT);
	pinMode(PIN_WATER,		INPUT);

	dallas.begin();
	analogReference(INTERNAL);

	DEBUG_PRINTLN("+++++Before END  +++++");
}

// --------------------------------------------------------------------
void setup() {
	DEBUG_PRINTLN("");
	DEBUG_PRINTLN("----Setup START-------");

	Serial.begin(9600); //compatible with Bluetooth HC05

	DEBUG_PRINTLN("----Setup END  -------");
}


// --------------------------------------------------------------------
void loop() {
	SendInitialtMsg();
	if(init_msg_sent){
		DEBUG_PRINTLN("");
		DEBUG_PRINTLN("#############################");

		cycles_count++;
		DEBUG_PRINT("# (");
		DEBUG_PRINT(cycles_count);
		DEBUG_PRINT(") ");
		if(cycles_count >= FORCE_REPORT ){
			DEBUG_PRINTLN("Forcing ALL reports !!!");
			force_report=true;
			cycles_count=0;
		}
		else{
			DEBUG_PRINTLN("Reporting changed :");
		}

		reportsFood();
		reportsWater();
		reportsTemp();
		reportsBattery();

		force_report=false;

		DEBUG_PRINTLN("");
		DEBUG_PRINTLN("# sleeping...");
		sleep(SLEEP_TIME);
	}
}


// FUNCTIONS ###################################################################

// --------------------------------------------------------------------
void SendInitialtMsg(){
	if (init_msg_sent == false && isTransportReady() ) {
	   	init_msg_sent = true;
		DEBUG_PRINTLN("");
		DEBUG_PRINTLN("# Transport is Ready");
		DEBUG_PRINTLN("");
   	}
}

// --------------------------------------------------------------------
void presentation(){
	DEBUG_PRINTLN("");
	DEBUG_PRINTLN("*** Presentation START ******");
	sendSketchInfo(INFO_NAME ,	INFO_VERS );
	present(CHILD_ID_FOOD,		S_DISTANCE);
	present(CHILD_ID_WATER,		S_WATER_LEAK);
	present(CHILD_ID_TEMP,		S_TEMP);
	DEBUG_PRINTLN("*** Presentation END ******");
}

// --------------------------------------------------------------------
void receive(const MyMessage &msg){
}

// --------------------------------------------------------------------
void reportsFood(){
	DEBUG_PRINTLN("");
	DEBUG_PRINT("# Reading Food : ");
	digitalWrite(PIN_US_POWER, HIGH);
	wait(100);

	int distance= sonarFood.convert_cm(sonarFood.ping_median());
	byte level=map(distance, MIN_FOOD_DIST, MAX_FOOD_DIST, 100, 0);

	DEBUG_PRINT(distance);
	DEBUG_PRINT(" cm , ");
	DEBUG_PRINT(level);
	DEBUG_PRINT("% ( last = ");
	DEBUG_PRINT(last_food_level);
	DEBUG_PRINTLN("% )");

	if(level != last_food_level || force_report){
		DEBUG_PRINTLN("  ==> Reporting !");
		send(msgFood.set(level), false);
		last_food_level = level;
	}
	digitalWrite(PIN_US_POWER, LOW);
}

// --------------------------------------------------------------------
void reportsWater(){
	buttWater.read();
	wait( WATER_DEBOUNCE + 5 );
	buttWater.read();
	boolean state= buttWater.isPressed();

	DEBUG_PRINTLN("");
	DEBUG_PRINT("# Reading Water : ");
	DEBUG_PRINT(state);
	DEBUG_PRINT(" ( last = ");
	DEBUG_PRINT(last_water_state);
	DEBUG_PRINTLN(" )");

	if(state != last_water_state || force_report){
		DEBUG_PRINTLN("  ==> Reporting !");
		send(msgWater.set(state), false);
		last_water_state =state;
	}
}


// --------------------------------------------------------------------
void reportsTemp(){
	dallas.requestTemperatures();
	wait( dallas.millisToWaitForConversion(dallas.getResolution()) +5 ); // make sure we get the latest temps
	float temp = dallas.getTempCByIndex(0);

	DEBUG_PRINTLN("");
	DEBUG_PRINT("# Reading Temperature : ");
	DEBUG_PRINT(temp);
	DEBUG_PRINT(" ( last = ");
	DEBUG_PRINT(last_temp_value);
	DEBUG_PRINTLN(" )");

	if (! isnan(temp)) {
		temp = ( (int) (temp * 10 ) ) / 10.0 ; //rounded to 1 dec
		if ( (temp != last_temp_value || force_report ) && temp != -127.0 && temp != 85.0) {
			DEBUG_PRINTLN("  ==> Reporting !");
			send(msgTemp.set(temp, 1), false);
			last_temp_value = temp;
		}
	}
}

// --------------------------------------------------------------------
void reportsBattery(){
	DEBUG_PRINTLN("");
	DEBUG_PRINT("# Reading Battery : ");

	//stabilize
	for (byte counter = 0; counter < 5; counter++) {
      analogRead(PIN_BATTERY);
      wait(50);
    }

	unsigned long sum =0;
	unsigned int read =0;
	for (int counter = 0; counter < BAT_READS; counter++) {
		read=analogRead(PIN_BATTERY);
	  	sum +=read;
	  	//DEBUG_PRINT(read);
	  	//DEBUG_PRINT(" + ");
     	wait(5);
    }

	//average
	int value = ceil(sum / (BAT_READS) );

    //int level = value / 10;
	int level=map(value, MIN_BAT, MAX_BAT, 0, 100);
    float volts  = value * 0.003363075;
	DEBUG_PRINT(" / ");
	DEBUG_PRINT( BAT_READS );
	DEBUG_PRINT(" = ");
	DEBUG_PRINT(value);
	DEBUG_PRINT(" => ");
	DEBUG_PRINT(volts);
	DEBUG_PRINT("V, ");
	DEBUG_PRINT(level);
	DEBUG_PRINT("%  ( last = ");
	DEBUG_PRINT(last_battery_level);
	DEBUG_PRINTLN("% )");

    if (level != last_battery_level  || force_report ) {
		DEBUG_PRINTLN("  ==> Reporting !");
        sendBatteryLevel(level);
        last_battery_level = level;
    }
	last_battery_value = value;
}
