#ifndef EASYFOCUS_H
#define EASYFOCUS_H
#include "BaseProtocol.h"

/*
* Routines to operate using the EasyFocus/Moonlight protocol
*
*  Command protocol (from Moonlight manual):
*    Command from PC             Response
*    :GP#                        XXXX#
*      Current motor position returned in unsigned Hex format.
*
*    :GC#                        XX#
*      Get Temperature Coefficient in signed Hex format.
*
*    :GN#                        XXXX#
*      Gets New Motor position
*
*    :GT#                        XXXX#
*      Get current temperature. Signed Hex. Value is 2* temp in C
*      (reported in half degree increments)
*
*    :GD#                        XX#
*      Gets motor speed. Returns 02,04,08,10,20 hex values
*
*    :GH#                        FF# (half step) 00# (full step)
*      Get motor stepping mode
*
*    :GI#                        01# if moving, 00# if stopped
*      Get Is Motor moving.
*
*    :GV#                        Current firmware version?
*      Returns the current firmware version.
*
*    :SPxxxx#                    none?
*      Sets the motor current position value
*
*    :SNxxxx#                    none
*      Sets the New Position (target position) for focuser.
*      Follow with :FG# command to start move
*
*    :SF#  :SH#                  none
*      Sets motor to Full or Half step mode
*
*    :SDxx#                      none
*      Sets motor speed. Values are 02,04,08,10,20
*
*    :FG#                        none
*      Starts a move to the New Motor position
*
*    :FQ#                        none
*      Quits / aborts motor movement. Position is retained.
*
*    :POxx#
*      Adjust Temperature offset, signed Hex
*
*    :PSxx#
*      Adjust Temperature scale, signed hex
*
*    :PHxx#
*      Find home for motor. Options are 01, 02?
*
*    :C#
*      Starts Read temperature
*
*    :-#  :+#
*      Disable / enable temperature compensation
*
*/

#define startMarker  ':'
#define endMarker    '#'
#define FIRMWAREVERSION "23"     // hex value   was 10

class CEasyFocus :
	public CBaseProtocol
{
public:
	CEasyFocus();
	~CEasyFocus();
	virtual boolean ReadCommand();   // true if got a command
	virtual String ProtocolProcessCommand();   
	virtual void WriteResponse(String response);
	inline virtual void MovementDone() {};
	inline virtual void MotorStepped() {};		// called whenever motor is stepped

private:

#define NUM_SPEEDS 5
	struct speedEntry {
		char hexKey[3];
		float motorSpeed;
	} speedTable[NUM_SPEEDS] = {
		{ "02", 250.0 },
		{ "04", 125.0 },
		{ "08", 75.0 },
		{ "10", 50.0 },
		{ "20", 25.0 },
	};

	String GetTargetPosition();
	String GetStepMode();
	String GetMotorSpeed();
	String SetPosition();
	String SetTargetPosition();
	String SetSpeed();
	String SetFullStep();
	String SetHalfStep();
	String StartMove();
	String AbortMove();
	String GetPosition();
	String GetIsMoving();
	String StartReadTemperature();
	String GetTemperature();
	String SetTemperatureOffset();
	String SetTemperatureScale();
	String GetTempCoeff();

private:
	String CvtIntToHex(int n, int len);
	int CvtSHexToInt(String sHex);
	unsigned int CvtUHexToInt(String uHex);
	float GetSpeedByKey(String hexKey);
	char *GetHexKeyBySpeed(float motorSpeed);
	void StatusToSerial();

};

#endif