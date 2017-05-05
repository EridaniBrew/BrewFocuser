#ifndef GLOBALS_H
#define GLOBALS_H

/*
 * Globals for BrewFocuser
 */
#include <Arduino.h>
#include <AccelStepper.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_MS_PWMServoDriver.h"
#include <EEPROM.h>
#include <Adafruit_MCP9808.h>
#include <math.h>

#include "BaseProtocol.h"

// Select the desired protocol below
extern CBaseProtocol *Protocol;

// Pin Assignments
#define ManualInSw   5
#define ManualOutSw  4
#define LED_IN       7		// shows when moving motor in
#define LED_OUT      6		// shows when moving motor out
#define LED_TEMP_COMP_ENABLED 3		// Temp Compensation enabled turns this on

// If this pin is grounded, then output the "Expanded" temperature readings
// see Robofocus.ino: rfGetTemperature for discussion
#define EXPANDED_TEMPERATURE  12

struct motorGlobals {
	// globals for operating the motor shield
	int gMotorStepMode = DOUBLE;      
	long gTargetPosition = 15000;
	long gMaxPosition = 30000;				// Max position allowed
	boolean gMotorRunning = false;
	float gTemperature = 0;					// last temperature read

};

extern struct motorGlobals mg;



#endif

