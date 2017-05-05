#include "EasyFocus.h"
#include "Globals.h"
#include "MotorIO.h"
#include "TempComp.h"

boolean gNewData = false;


CEasyFocus::CEasyFocus()
{
}


CEasyFocus::~CEasyFocus()
{
}

boolean CEasyFocus::ReadCommand()   // true if got a command
{
	static boolean recvInProgress = false;
	static byte ndx = 0;
	char rc;
	boolean gotCommand = false;

	_inCommand = "";

	while (Serial.available() > 0)
		{
		rc = Serial.read();
		
		if (recvInProgress == true)
		{
			if (rc != endMarker)
			{
				gReceivedChars[ndx] = rc;
				ndx++;
				if (ndx >= NUM_CHARS)
				{
					ndx = NUM_CHARS - 1;
				}
			}
			else
			{
				gReceivedChars[ndx] = '\0'; // terminate the string
				recvInProgress = false;
				ndx = 0;
				gotCommand = true;
				_inCommand = gReceivedChars;
				_inCommand.toUpperCase();
				FlashYellow(1);				// debug to see when receive command
				return(gotCommand);
			}
		}

		else if (rc == startMarker)
		{
			recvInProgress = true;
			ndx = 0;
			gReceivedChars[ndx] = rc;
			ndx++;
			if (ndx >= NUM_CHARS)
			{
				ndx = NUM_CHARS - 1;
			}
		}
	}

	return (gotCommand);
}

void CEasyFocus::WriteResponse(String s)
{
	Serial.print(s);
	Serial.flush();
}

String CEasyFocus::ProtocolProcessCommand()   
{
	String complStatus = "";
	if (_inCommand == ":GP")
	{
		complStatus = GetPosition();
	}
	else if (_inCommand == ":GN")
	{
		complStatus = GetTargetPosition();
	}
	else if (_inCommand == ":GC")
	{
		complStatus = GetTempCoeff();
	}
	else if (_inCommand == ":GD")
	{
		complStatus = GetMotorSpeed();
	}
	else if (_inCommand == ":GH")
	{
		complStatus = GetStepMode();
	}
	else if (_inCommand == ":GI")
	{
		complStatus = GetIsMoving();
	}
	else if (_inCommand == ":GT")
	{
		complStatus = GetTemperature();
	}
	else if (_inCommand == ":GV")
	{
		complStatus = FIRMWAREVERSION;
		complStatus = complStatus + "#";
	}
	else if (_inCommand.startsWith(":SP"))
	{
		complStatus = SetPosition();
	}
	else if (_inCommand.startsWith(":SN"))
	{
		complStatus = SetTargetPosition();
	}
	else if (_inCommand == ":SF")
	{
		complStatus = SetFullStep();
	}
	else if (_inCommand == ":SH")
	{
		complStatus = SetHalfStep();
	}
	else if (_inCommand.startsWith(":SD"))
	{
		complStatus = SetSpeed();
	}
	else if (_inCommand == ":FG")
	{
		complStatus = StartMove();
	}
	else if (_inCommand == ":FQ")
	{
		complStatus = AbortMove();
	}
	else if (_inCommand == ":C")
	{
		complStatus = StartReadTemperature();
	}
	else if (_inCommand == ":-")
	{
		complStatus = EnableTemperatureCompensation(false);
	}
	else if (_inCommand == ":+")
	{
		complStatus = EnableTemperatureCompensation(true);
	}
	else if (_inCommand.startsWith(":PO"))
	{
		complStatus = SetTemperatureOffset();
	}
	else if (_inCommand.startsWith(":PS"))
		{
		complStatus = SetTemperatureScale();
		}
	else if (_inCommand == ":XS")
		{
		complStatus = F("");
		StatusToSerial();
		}
	else
	{ // invalid command
		if (_inCommand.length() > 0)
		{
			FlashOut(5);
		}
	}

	return (complStatus);
}



// GetTargetPosition
//    unsigned hex
String CEasyFocus::GetTargetPosition()
{
	long pos = motGetTargetPosition();
	String hexPos = CvtIntToHex(pos, 4);
	hexPos = hexPos + F("#");
	return (hexPos);
}

// GetStepMode
//    return FF for half step, 00 for full step
String CEasyFocus::GetStepMode()
{
	String ret = "00#"; // full step
	if (mg.gMotorStepMode == DOUBLE)
	{
		ret = F("FF#");
	}
	return (ret);
}

// GetMotorSpeed
//    return xx = 02,04,08,10,20
String CEasyFocus::GetMotorSpeed()
{
	float theSpeed = motGetMotorSpeed();
	String ret = GetHexKeyBySpeed(theSpeed);
	ret.concat("#");
	return (ret);
}

// GetIsMoving
//    00 is stopped, 01 if moving
String CEasyFocus::GetIsMoving()
{
	String ret = F("01#");
	if (!motIsMoving())
	{
		ret = F("00#");
	}
	return (ret);
}

// SetPosition   :SPxxxx
//    Position is unsigned hex XXXX
String CEasyFocus::SetPosition()
{
	// convert xxxx to long
	String pos = _inCommand.substring(3);
	long newPos = CvtUHexToInt(pos);
	if (newPos < 1) newPos = 1;
	if (newPos > mg.gMaxPosition) newPos = mg.gMaxPosition;
	motSetPosition(newPos);
	return (F(""));
}

// SetTargetPosition   :SNxxxx
//    Position is unsigned hex XXXX
//    We save the target position in global. 
//    Later the StartMove will start the actual move
String CEasyFocus::SetTargetPosition()
{
	String pos = _inCommand.substring(3);
	long tPos = CvtUHexToInt(pos);
	if (tPos < 1) tPos = 1;
	if (tPos > mg.gMaxPosition) tPos = mg.gMaxPosition;
	motSetTargetPositionNum(tPos);
	return (F(""));
}

// SetSpeed   :SDxx
//    Speed is 02,04,08,10,20
//    Corresponds to 250, 125, ...
String CEasyFocus::SetSpeed()
{
	String hexKey = _inCommand.substring(3, 5);
	float newSpeed = GetSpeedByKey(hexKey);
	motSetSpeed(newSpeed);
	return (F(""));
}

// SetFullStep
//    set motor to full step
//    I am assuming this means use SINGLE mode
//    or MULTISTEP?
String CEasyFocus::SetFullStep()
{
	motSetStepMode(SINGLE);
	return (F(""));
}

// SetHalfStep
//    set motor to half step
String CEasyFocus::SetHalfStep()
{
	motSetStepMode(DOUBLE);
	return (F(""));
}

// StartMove  (:FG)
//
String CEasyFocus::StartMove()
{
	// Set the motor to move
	motStartMove();
	return (F(""));
}

// AbortMove
//
String CEasyFocus::AbortMove()
{
	motAbortMove();
	return (F(""));
}

// GetPosition()
//  returns the current position of the focuser unsigned hex XXXX
String CEasyFocus::GetPosition()
{
	long curPos = motGetPosition();
	String ret = CvtIntToHex(curPos, 4) + F("#");
	return (ret);
}


// StartReadTemperature()
//    reads temperature. Subsequent GetTemperature returns the temperature
String CEasyFocus::StartReadTemperature()
{
	return (F(""));
}

// GetTemperature()
//   returns Signed Hex of gTemperature (previously read)
// NOTE - if EXPANDED_TEMPERATURE pin is grounded, uses an expanded temperature
//      to get better temperature precision.
//  To convert expanded units to degC, the temp will be scaled to the range -10..40 degC
//   Units = (degC + 10)/(40 + 10) * 1023
//   degC = (Units*2 /1023 * (40 + 10) + 10
#define MIN_DEGC  -10.0
#define MAX_DEGC  40.0
String CEasyFocus::GetTemperature()
{
	// temperature is reported in half degree increments
	long rdg = 0;
	float c = mg.gTemperature;    
	if (digitalRead(EXPANDED_TEMPERATURE) == LOW)
		{ // Use "expanded" temperature
		rdg = (mg.gTemperature - MIN_DEGC) * 1023.0 / (MAX_DEGC - MIN_DEGC);
		}
	else
		{ // Use actual centigrade units
		rdg = mg.gTemperature * 2.0 + 0.5;
		}
	String ret = CvtIntToHex(rdg, 4) + F("#");
	return (ret);
}

// SetTemperatureOffset(:POxx)
//    offset is signed hex
String CEasyFocus::SetTemperatureOffset()
{
	long steps = _inCommand.substring(3, 5).toInt();			// not implemented
	return (F(""));
}

// SetTemperatureScale(:PSxx)
//    scale (slope) is signed hex
String CEasyFocus::SetTemperatureScale()
{
	long slope = CvtSHexToInt(_inCommand.substring(3, 5));
	tempSetTemperatureScale(slope);
	return (F(""));
}

// GetTempCoeff(:GC)
//    scale (slope) is signed hex
String CEasyFocus::GetTempCoeff()
{
	long slope = tempGetTempCoeff();
	String ret = CvtIntToHex(slope, 2) + F("#");
	return (ret);
}



// CvtIntToHex
//    Takes signed int value and makes it into hex string.
//    Pads leading zeros to make the target length
//  example: 12 --> 000C
//          -12 --> FFF4
String CEasyFocus::CvtIntToHex(int n, int len)
{
	String s = String(n, HEX);
	s.toUpperCase();
	while (s.length() < len)
	{
		s = "0" + s;
	}
	return (s);
}

// CvtSHexToInt
//    Takes signed hex string and converts to signed int value.
//  example: 000C --> 12 
//           FFF4 --> -12
int CEasyFocus::CvtSHexToInt(String sHex)
{
	if (sHex.length() == 2)
	{
		char first = sHex[0];
		if (first & 0x10)
		{
			sHex = "FF" + sHex;
		}
	}
	int val = strtol(sHex.c_str(), 0, 16);
	return (val);
}

// CvtUHexToInt
//    Takes unsigned hex string and converts to unsigned int value.
//  example: 000C --> 12 
//           FFF4 --> 65524
unsigned int CEasyFocus::CvtUHexToInt(String uHex)
{
	unsigned int val = strtoul(uHex.c_str(), 0, 16);
	return (val);
}




// GetSpeedByKey 
//    returns the speed from the speedTable
//    i.e., "02" gives 250
float CEasyFocus::GetSpeedByKey(String hexKey)
{
	int i;
	for (i = 0; i< NUM_SPEEDS; i++)
	{
		if (speedTable[i].hexKey == hexKey.c_str())
			return (speedTable[i].motorSpeed);
	}
	// not in table
	return (100.0);
}

// GetHexKeyBySpeed 
//    returns the speed from the speedTable
//    i.e., 250 gives "02" 
char *CEasyFocus::GetHexKeyBySpeed(float motorSpeed)
{
	int i;
	for (i = 0; i< NUM_SPEEDS; i++)
	{
		if (fabs(motorSpeed - speedTable[i].motorSpeed) < 5)
			return (speedTable[i].hexKey);
	}
	// not in table
	return ("02");
}
// StatusToSerial
//   Prints values to Serial
//   This is for debugging, using special backdoor command :XS#
//   Should only be used when debugging with Serial Console -
//   never issued by a program/driver
void CEasyFocus::StatusToSerial()
{
	Serial.println("");
	Serial.print("Cur Pos: "); Serial.println(motGetPosition());
	Serial.print("Target Pos: "); Serial.println(mg.gTargetPosition);
	Serial.print("motorRunning flag: "); Serial.println(mg.gMotorRunning? "true" : "false");
	Serial.print("Max pos: "); Serial.println(mg.gMaxPosition);
	Serial.print("Motor Moving (dist): "); Serial.println(motIsMoving() ? "true" : "false");
	//Serial.print(); Serial.println();
}

