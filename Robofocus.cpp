#include "Robofocus.h"
#include "Globals.h"
#include "MotorIO.h"
#include "TempComp.h"
#include "EepromUtil.h"

// used to issue "I"/"O" response while motor is moving. Only issue the character every 
// 10 steps, for example
#define MOVEMENT_STEPS 10
int movementCount = 0;   

CRobofocus::CRobofocus()
{
}


CRobofocus::~CRobofocus()
{
}

void CRobofocus::Init()
{
	for (int i = 0; i < 4; i++)
	{
		gPowerSwitch[i - 4] = false;
	}

}

boolean CRobofocus::ReadCommand()
// ReadCommand
// reads command. Commands are 9 characters.
// Command starts with "F". No other Fs in commands (oops, in checksum).
// fills in inCommand 
{
	_inCommand = "";
	static boolean recvInProgress = false;
	char rc;
	static int idx = 0;
	boolean gotCommand = false;

	while (Serial.available() > 0)
	{
		rc = Serial.read();
		if (mg.gMotorRunning)
		{
			//motAbortMove();
			//return (gotCommand);
		}
		if ((toupper(rc) == 'F') && !recvInProgress)
		{  // start a new command
			recvInProgress = true;
			idx = 0;
			gReceivedChars[idx] = rc;
			idx++;
			gReceivedChars[idx] = 0;
		}
		else if (recvInProgress)
		{  // add rc to command
			if (idx <= 8)
			{
				gReceivedChars[idx] = rc;
				idx++;
			}
			if (idx > 8)
			{  // last character!
				gReceivedChars[idx - 1] = 0;
				_inCommand = gReceivedChars;
				_inCommand.toUpperCase();
				idx = 0;
				gotCommand = true;
				recvInProgress = false;
				//FlashIn(1);
				//Serial.print("Command is "); Serial.println(_inCommand);
				return (gotCommand);
			}
		}
	}

	return (gotCommand);
}

// writeRoboFocus
// Writes string to RoboFocus. 
// Adds checksum at end of string. No cr/lf
void CRobofocus::WriteResponse(String s)
{
	char checkSum = 0;
	checkSum = RfCheckSum(s);
	s.concat(checkSum);
	Serial.print(s);
	Serial.flush();
}

char CRobofocus::RfCheckSum(String s)
{
	int cs = 0;
	for (int i = 0; i < s.length(); i++)
	{
		cs = cs + s[i];
	}
	cs = cs & 0xff;
	return (cs);
}


/////////////////////////////////////////////////////////////////////////////////////////
String CRobofocus::ProtocolProcessCommand()
{
	String complStatus = "";
	if (_inCommand[1] == 'G')
	{
		complStatus = rfGoToPosition();
	}
	else if (_inCommand[1] == 'S')
	{	// FS?XXXXX   Set position
		complStatus = rfSetPosition();
	}
	else if (_inCommand[1] == 'I')
	{	// FI?XXXXX   
		complStatus = rfMoveDelta();
	}
	else if (_inCommand[1] == 'O')
	{	// FO?XXXXX   
		complStatus = rfMoveDelta();
	}
	else if (_inCommand[1] == 'L')
	{	// FL?XXXXX   
		complStatus = rfSetMaxTravel();
	}
	else if (_inCommand[1] == 'P')
	{	// FP?XXXXX   I don't implement 4 outputs feature
		complStatus = rfFourOutputs();
	}
	else if (_inCommand[1] == 'B')
	{	// FBNXXXXX   Backlash not being implemented
		complStatus = F("FB200000");
	}
	else if (_inCommand[1] == 'C')
	{	// FCABCDEF			Configure   
		complStatus = rfConfigureMotor();
	}
	else if (_inCommand[1] == 'T')
	{	// FTXXXXXX   
		complStatus = rfGetTemperature();
	}
	else if (_inCommand[1] == 'V')
	{	// FVXXXXXX   Version
		motAbortMove();      // RF Basic Driver uses Version command to stop the motor movement
		complStatus = F("FV000300");
	}
	else
	{ // invalid command
		if (_inCommand.length() > 0)
		{
			//Serial.print("Bad cmd: "); Serial.println(_inCommand);
			FlashOut(5);
		}
	}

	return (complStatus);
}


// rfGoToPosition		FG?XXXXX
// go to XXXXX
// If zero, send CUrrent Position string
//    (means cannot go to pos 0?)
// If non-zero, start movement
//    then start emitting I/O
//    When done, send Current Position String FD?XXXXX
String CRobofocus::rfGoToPosition()
{
	String ret = F("");
	long pos = strtol(_inCommand.substring(3).c_str(), 0, 10);
	if (pos > mg.gMaxPosition) pos = mg.gMaxPosition;
	if (pos != 0)
	{ // Do move to pos
		motSetTargetPositionNum(pos);
		motStartMove();
	}
	else
	{
		ret = rfGetCurPosition();
	}
	return (ret);
}

// rfMoveDelta		FO?XXXXX  or FI?XXXXX
// 
String CRobofocus::rfMoveDelta()
{
	long delta = strtol(_inCommand.substring(3).c_str(), 0, 10);
	long curPos = motGetPosition();
	if (_inCommand[1] == 'I')
	{
		curPos -= delta;
		if (curPos < 0) curPos = 1;
	}
	else
	{
		curPos += delta;
		if (curPos > mg.gMaxPosition) curPos = mg.gMaxPosition;
	}
	// do the move
	motSetTargetPositionNum(curPos);
	motStartMove();
	return (F(""));
}

// rfGetTemperature		FTXXXXXX
// return FTXXNNNN  where NNNN is 0-1024. 
// NNNN 600 = 300K = 27C
//
// Expanded Temperature
// The normal method of returning the temperature converts the Centigrade value
// to integer "units", which have a precision of roughly 0.5 degrees.
// My temp sensor is supposed to be precise to 0.06 degrees, so I would like to
// get values back with more precision.
// So, Expanded units maps the Centigrade reading (-20..50 deg C) to the Units range 0-1023.
// This give a precision of about 0.07 degC (15 units per degC).
// This cannot be reconverted by RoboFocus to deg C, so the user will only see Units for the temperature.
//
// To convert units to degC,
//   degC = Units/1024 * 70 - 20
String CRobofocus::rfGetTemperature()
{
	long rdg = 0;
	if (digitalRead(EXPANDED_TEMPERATURE) == LOW)
	{ // Use "expanded" temperature
		rdg = map((long)mg.gTemperature, -20, 50, 0, 1023);
	}
	else
	{ // Use actual centigrade units
		rdg = (mg.gTemperature + 0.5 + 273.3) * 2.0;
	}

	char buf[12];
	sprintf(buf, "FT%06ld", rdg);
	String ret = buf;
	return (ret);
}

// rfSetPosition		FS?XXXXX
// Set current position to XXXXX
// If zero, send Current Position string
//    (means cannot go to pos 0?)
// If non-zero, respond with new position (Current Position)
String CRobofocus::rfSetPosition()
{
	long pos = strtol(_inCommand.substring(3).c_str(), 0, 10);
	if (pos > 0)
	{
		motSetPosition(pos);
	}
	return (rfGetCurPosition());
}

// rfGetCurPosition		
// Send current position with FD?XXXXX
// Several commands respond with this string
String CRobofocus::rfGetCurPosition()
{
	String ret = "";
	char buf[12];
	long curPos = motGetPosition();
	sprintf(buf, "FD%06d", curPos);
	ret = buf;
	return (ret);
}

// rfConfigureMotor		FCABCDEF
// A - spare
// B - spare
// C - spare
// D - Duty cycle 0-250 ==> 0-100%
// E - Step Delay. 1-64 ms per step
// F - Step Size. 1-64 microsteps to produce 1 "step". High numbers are coarse steps.
String CRobofocus::rfConfigureMotor()
{
	String ret = "FC000DEF";
	return(ret);
}

// rfSetMaxTravel		FL?XXXXX
// Set the Max travel allowed for the focuser
String CRobofocus::rfSetMaxTravel()
{
	String ret = "";
	long pos = strtol(_inCommand.substring(3).c_str(), 0, 10);
	if (pos > 0)
	{ // Set the new position
		mg.gMaxPosition = pos;
		writeLong(ADDR_MAX_POSITION, mg.gMaxPosition);		// update eeprom
		ret = _inCommand;
	}
	else
	{// Asking for current setting
		char buf[12];
		sprintf(buf, "FL%06ld", mg.gMaxPosition);
		ret = buf;
	}
	return(ret);
}

// rfFourOutputs     FP001212
//  0 = report current value
//  1 = turn off
//  2 = turn on
String CRobofocus::rfFourOutputs()
{
	String ret = "FP000000";
	for (int i = 4; i < 8; i++)
	{
		if (_inCommand[i] == '1') gPowerSwitch[i - 4] = false;
		if (_inCommand[i] == '2') gPowerSwitch[i - 4] = true;
		ret[i] = gPowerSwitch[i - 4] ? '2' : '1';
	}
	return (ret);
}

void CRobofocus::MovementDone()
{
	String allDone = rfGetCurPosition();
	WriteResponse(allDone);
}

// MotorStepped
//   when motor is stepped, send an 'i' or 'O' 
void CRobofocus::MotorStepped()
{
	movementCount++;
	if (movementCount > MOVEMENT_STEPS)
	{
		movementCount = 0;
		String moved = motGetMotorDirection();			// gets an I or O
		
		if (moved != "")
			{
			//WriteResponse(moved);
			}
	}

}
