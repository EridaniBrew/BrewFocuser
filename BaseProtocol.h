#ifndef BASEPROTOCOL_H
#define BASEPROTOCOL_H
#include <Arduino.h>
//#include "Globals.h"

class CBaseProtocol
{
public:
	CBaseProtocol();
	~CBaseProtocol();
	inline virtual void Init() { /*nothing*/ };
	inline virtual boolean ReadCommand() { return (false); };   // true if got a command
	inline virtual String ProtocolProcessCommand() { return (""); };   // true if got a command
	inline virtual void WriteResponse(String response) {};
	inline virtual void MovementDone() {};		// called when motor stops moving
	inline virtual void MotorStepped() {};		// called whenever motor is stepped

public:
	String _inCommand;

	// used in read functions
#define NUM_CHARS  32
	char gReceivedChars[NUM_CHARS];


};

#endif