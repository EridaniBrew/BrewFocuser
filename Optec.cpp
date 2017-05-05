#include "Optec.h"
#include "Globals.h"
#include "MotorIO.h"
#include "TempComp.h"


COptec::COptec()
{
}


COptec::~COptec()
{
}

boolean COptec::ReadCommand()   // true if got a command
{
	_inCommand = "";
	static boolean recvInProgress = false;
	char rc;
	static int idx = 0;

	while (Serial.available() > 0)
	{
		rc = Serial.read();
		if (!recvInProgress && (rc == 'F'))
		{  // start a new command
			recvInProgress = true;
			idx = 0;
			gReceivedChars[idx] = rc;
			idx++;
			gReceivedChars[idx] = 0;
		}
		else if (recvInProgress)
		{  // add rc to command
			gReceivedChars[idx] = rc;
			idx++;
			gReceivedChars[idx] = 0;
			if ((idx == 6) || (strcmp(gReceivedChars, "FHOME") == 0))
			{  // last character!
				_inCommand = gReceivedChars;
				_inCommand.toUpperCase();
				idx = 0;
				recvInProgress = false;
				FlashIn(1);
				return;
				//Serial.print("Command is "); Serial.println(_inCommand);
			}
		}
	}

	return;
}

void COptec::WriteResponse(String s)
{
	Serial.println(s);
	Serial.flush();
}

// ProtocolProcessCommand
// Call the appropriate routine(s) based on the command
String COptec::ProtocolProcessCommand()
{
	String complStatus = "";
	if (_inCommand == F("FMMODE"))
	{
		complStatus = F("!");
	}
	else if (_inCommand == F("FAMODE"))
	{	// TODO: understand this one. I think we start compensation, and maybe start
		// telemetry of position/Temp
		complStatus = "P=3500  T=0030";
	}
	else if (_inCommand == F("FBMODE"))
	{
		complStatus = "P=3500  T=0030";
	}
	else if (_inCommand.startsWith(F("FI")))
	{
		complStatus = optMoveDelta(true, _inCommand.substring(2));
	}
	else if (_inCommand.startsWith(F("FO")))
	{
		complStatus = optMoveDelta(false, _inCommand.substring(2));
	}
	else if (_inCommand == F("FPOSRO"))
	{
		complStatus = optGetPosition();
	}
	else if (_inCommand == F("FTMPRO"))
	{
		complStatus = optGetTemperature();
	}
	else if (_inCommand == F("FCENTR"))
	{
		complStatus = F("CENTER");
	}
	else if (_inCommand.startsWith(F("FLA")))
	{
		complStatus = optSetCoeff(F("A"), _inCommand.substring(3));
	}
	else if (_inCommand.startsWith(F("FLB")))
	{
		complStatus = optSetCoeff(F("B"), _inCommand.substring(3));
	}
	else if (_inCommand == F("FSLEEP"))
	{
		complStatus = F("ZZZ");
	}
	else if (_inCommand == F("FWCXXX"))
	{
		complStatus = F("WAKE");			// never sleeps
	}
	else if ((_inCommand == F("FWAKEE")) || (_inCommand == F("FWAKUP")))
	{
		complStatus = F("WAKE");
	}
	else if (_inCommand == F("FREADA"))
	{
		complStatus = optGetCoeff(F("A"));
	}
	else if (_inCommand == F("FREADB"))
	{
		complStatus = optGetCoeff(F("B"));
	}
	else if (_inCommand.startsWith(F("FQUIT")))
	{
		complStatus = optSetTelemetry(_inCommand.substring(5));
	}
	else if (_inCommand.startsWith(F("FDA")))
	{
		complStatus = optCompensationDelay(F("A"), _inCommand.substring(3));
	}
	else if (_inCommand.startsWith(F("FDB")))
	{
		complStatus = optCompensationDelay(F("B"), _inCommand.substring(3));
	}
	else if (_inCommand == F("FFMODE"))
	{
		complStatus = F("END");
	}
	else if (_inCommand == "FHOME")
	{
		// TODO: perform compensation from last SLEEP command
		complStatus = F("DONE");
	}
	else if (_inCommand.startsWith(F("FT")))
	{
		complStatus = optReportSign(_inCommand[5]);   // 'A' or 'B'
	}
	else if (_inCommand.startsWith(F("FZA")))
	{
		complStatus = optSetCoeffSign('A', _inCommand[5]);
	}
	else if (_inCommand.startsWith(F("FZB")))
	{
		complStatus = optSetCoeffSign('B', _inCommand[5]);
	}
	else
	{ // invalid command
		if (_inCommand.length() > 0)
		{
			//Serial.println(_inCommand);
			FlashOut(5);
		}
	}

	return (complStatus);
}



// optMoveDelta   nnnn
//    num pulses to move
//    Later the StartMove will start the actual move
String COptec::optMoveDelta(boolean moveIn, String pulses)
{
	long tSteps = strtol(pulses.c_str(), 0, 10);
	if (moveIn)
	{
		mg.gTargetPosition -= tSteps;
		if (mg.gTargetPosition < 0) mg.gTargetPosition = 0;
	}
	else
	{
		mg.gTargetPosition += tSteps;
		if (mg.gTargetPosition > 9999) mg.gTargetPosition = 9999;
	}
	motSetTargetPositionNum(mg.gTargetPosition);
	motStartMove();
	while (RunMotorAsNeeded(true))
	{
		delay(1);
	}
	return (F("*"));
}


// optGetPosition()
//  returns the current position of the focuser “P=nnnn”
String COptec::optGetPosition()
{
	char buf[10];
	long curPos = motGetPosition();
	sprintf(buf, "P=%04d", curPos);
	String ret = buf;
	return (ret);
}

// GetTemperature()
//   returns T=+/-nn.n
String COptec::optGetTemperature()
{
	char buf[10];

	dtostrf(mg.gTemperature + 0.05, 4, 1, buf);   // 0.05 to round
	char sign = '+';
	if (mg.gTemperature < 0) sign = '-';
	String ret = "T=";
	ret.concat(sign);
	ret.concat(buf);
	return (ret);
}

// optGetCoeff()
//    scale (slope) is int
//    whichCoeff is "A" or "B"; for now, I only have one coeff
//    A=0nnn
String COptec::optGetCoeff(String whichCoeff)
{
	char buf[8];
	long slope = tempGetTempCoeff();
	sprintf(buf, "%c=%04d", whichCoeff[0], slope);
	String ret = buf;
	return (ret);
}

// optSetCoeff
//    whichCoeff is "A" or "B"     only "A" is used for now
//    coeff is string "999"
String COptec::optSetCoeff(String whichCoeff, String slope)
{
	long coeff = strtol(slope.c_str(), 0, 10);
	tempSetTemperatureScale(coeff);
	return(F("DONE"));
}

// optSetTelemetry
//    onOff   0 send telemetry every 1 sec
//            1 turn off telemetry
String COptec::optSetTelemetry(String onOff)
{
	if (onOff == "0") FlashOut(10);			// TODO: implement telemetry
	return(F("DONE"));
}

// optCompensationDelay
//  The default delay between step movements during temperature compensation 
//  (A or B mode) is 1.00 seconds. This delay between steps can be increased using 
//  this new command. Values of 001 to 999 correspond to 0.01 seconds to 9.99 seconds 
//  of additional delay.
//  example: FDA400 will increase the default delay for FAMODE from 1.00 second to 5.00 seconds.
String COptec::optCompensationDelay(String whichComp, String del)
{
	// TODO: set delay between compensation adjustments
	return(F("DONE"));
}

// optReportSign
//   return A=n    n=0 for pos coeff, 1 for neg coeff
String COptec::optReportSign(char whichCoeff)
{
	String ret = "";
	ret.concat(whichCoeff);
	ret.concat("=");
	if (tempGetTempCoeff() < 0)
		ret.concat("1");
	else
		ret.concat("0");
	return(ret);
}

// optSetCoeffSign
//   Set the sign of the selected coefficient
//   plusMinus is '0' - positive   '1' - negative
String COptec::optSetCoeffSign(char whichCoeff, char plusMinus)
{
	long coeff = tempGetTempCoeff();
	if ((plusMinus == '1') && (coeff > 0))
	{  // pos should be neg
		tempSetTemperatureScale(-coeff);
	}
	else if ((plusMinus == '0') && (coeff < 0))
	{  // neg should be positive
		tempSetTemperatureScale(-coeff);
	}
	return (F("DONE"));
}
