#ifndef OPTEC_H
#define OPTEC_H

#include "BaseProtocol.h"

/*
* Routines to operate using the Optec protocol
*
FMMODE	(Focuser Manual Mode)		“!” LF CR

FAMODE	(Focuser Auto-A Mode)		“P=” nnnn LF CR “T=” ±nn.n LF CR
FBMODE

FInnnn	(Focuser In “nnnn”)			“*” LF CR

FOnnnn	(Focuser Out “nnnn”)		“*” LF CR

FPOSRO	(Focuser Position Read Out)	“P=nnnn” LF CR

FTMPRO	(Focuser Temperature Read Out)	“T=+/-nn.n” LF CR

FCENTR	(Focuser Center)			“CENTER” LF CR

FSLEEP	(Focuser Sleep)				“ZZZ” LF CR

FWAKUP	(Focuser Wake Up)			“WAKE” LF CR
FWAKEE

FREADA	(Focuser Read Slope)		“A=0nnn” or “B=0nnn” LF CR
FREADB

FLAnnn	(Focuser Load Slope)		“DONE” LF CR
FLBnnn

FQUITn	(Focuser Quiet)				“DONE” LF CR

FDAnnn	(Focuser Delay)				“DONE” LF CR
FDBnnn

FHOME	(Focuser Home)				“DONE” LF CR

FFMODE	(Focuser Free Mode)			“END” LF CR

FtxxxA	(Slope sign)				“A=n” or “B=n” LF CR
FTxxxB

FZAxxn	(Load slope sign )			“DONE” LF CR
FZBxxn

*/

class COptec :
	public CBaseProtocol
{
public:
	COptec();
	~COptec();
	virtual boolean ReadCommand();   // true if got a command
	virtual String ProtocolProcessCommand();
	virtual void WriteResponse(String response);
	inline virtual void MovementDone() {};
	inline virtual void MotorStepped() {};		// called whenever motor is stepped

private:
	String COptec::optMoveDelta(boolean moveIn, String pulses);
	String COptec::optGetPosition();
	String COptec::optGetTemperature();
	String COptec::optGetCoeff(String whichCoeff);
	String COptec::optSetCoeff(String whichCoeff, String slope);
	String COptec::optSetTelemetry(String onOff);
	String COptec::optCompensationDelay(String whichComp, String del);
	String COptec::optReportSign(char whichCoeff);
	String COptec::optSetCoeffSign(char whichCoeff, char plusMinus);

};

#endif
