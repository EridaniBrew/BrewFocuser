#pragma once
#include "BaseProtocol.h"

//extern void FlashIn();

/*
* Routines to operate using the RoboFocus protocol
*
* General form is FXNNNNNNZ
* where X is the command letter (i.e., G for GetPositio)
*       Z is a checksum
*       NNNNNN is decimal digits.

FVXXXXXXZ.			FVXXXXXXZ
Commands RoboFocus to respond with RoboFocus firmware version number.
Response is FVXXXXXXZ.
This command is alsoo sent to stop motor movement.

FG?XXXXXZ.			FD?XXXXXZ
Commands RoboFocus to goto position ?XXXXX.  If ?XXXXX are all zeros, response
from the RoboFocus is the current position in the form FD?XXXXXZ.  If ?XXXXX represents non-zero,
this is the target position and begin moving.  As the stepper moves, RoboFocus
responds to the command with the character "I" or "O" for each step. (NOT IMPLEMENTED)
At the conclusion of the move, respond with the current position FD?XXXXXZ.

FI?XXXXXZ.			FD?XXXXXZ
Commands RoboFocus to move inward by ?XXXXX steps then sends current position FD?XXXXXZ.

FO?XXXXXZ.			FD?XXXXXZ
Commands RoboFocus to move outward by ?XXXXX steps then sends current position.

FS?XXXXXZ.			FD?XXXXXZ
Commands RoboFocus to set current position to ?XXXXX.  If ?XXXXX=0 then
RoboFocus responds with the current position.  If ?XXXXX<>0, RoboFocus responds with new current
position.

FL?XXXXXZ			FL0XXXXXZ
Set maximum travel to XXXXX.
If XXXXX=0 then RoboFocus responds with the old setting.
If ?XXXXX<>0, RoboFocus responds with the new setting.

FP??XXXXZ			FP??XXXXZ
Set the four outputs for the remote power module.  The channels
are 1-4 L-R.
If ??XXXX=0 then RoboFocus responds with the current power settings.
If X=1 the channel will be set to OFF; if an X=2, the channel will be set to ON.
Values other than 0-2 are interpreted as 0.
RoboFocus responds with the new power settings in form FP??XXXXZ.

FBNXXXXXZ.			FBNXXXXXZ
Change form of backlash compensation.
If XXXX=0 then respond with the current settings.
N=1 is no compensation (N=1 not supported in V3 or later firmware),
N=2 is compensation added to IN motion,
N=3 is compensation added to OUT motion.
XXXXX is amount of compensation as 1-255.
Factory default is 200020 (IN with 20 steps).
Response is FBNXXXXXZ.

Any command.    This is not true with the Basic ASCOM driver.
During movement, RoboFocus monitors its input serial channel. Any serial activity is
taken as an immediate stop command and movement will cease (if the buttons on the RoboFocus are
pushed during computer driven movement it will also stop).  Response will be the current position.
With the Robofocus Server, a bunch of "V" characters are sent to stop the motor.

FCABCDEFZ.
Set configuration.  If ABCDE=0 then responds with
current configuration settings.  If non-zero, configurations be set as follows:
A-spare
B-spare
C-spare
D-Duty Cycle.  This character has an ASCI value of 0-250 which corresponds to a duty cycle for the
stepper of 0-100%.
E-Step Delay.  This character has an allowed ASCI value of 1-64 approximately equal to the time delay in
ms per microstep of the stepper.
F-Step Size.  This character has an allowed ASCI value of 1-64 equal to the number of microsteps needed
to produce one "step" (count) of the focus position.  High numbers are coarse steps.

FTXXXXXXZ.				FTXXXXXXZ
Take temperature reading and respond.  Response is FTXXNNNNZ where NNNN is the
temperature reading in raw counts 0-1024 from the ten-bit ADC.  The
count is normally within 1% of 2X the Kelvin temperature (Count of 600 equals 300K equals 27C).
NOTE - I have implemented an additional mode "Expanded Temperature".
The standard method above gives temperature readings precise to about 0.5 degC.
My temperature probe is precise to 0.06 degC, so I would like to get better precision with
the readings sent back.
If pin 12 is connected to ground, the Centigrade temperature is converted to a Units value by mapping the
temperature to the range -20-50 degC --> 0-1023. This gives a temperature precision of about 0.07 degC.
Unfortunately I have no way to get the Robofocus driver to convert these values back to Centigrade.
So, in this mode the only display units that make sense will be "Raw Units"; the user cannot see the
Centigrade temperature.
To convert units to degC,
degC = Units/1024 * 70 - 20

******/

class CRobofocus :
	public CBaseProtocol
{
public:
	CRobofocus();
	~CRobofocus();
	void Init();
	virtual boolean ReadCommand();
	virtual String ProtocolProcessCommand();   
	virtual void WriteResponse(String response);
	virtual void MovementDone();		// called when motor stops moving
	virtual void MotorStepped();		// called whenever motor is stepped

private:
	boolean gPowerSwitch[4];			// tracking the 4 power switches in case I come up with a use for this

	String rfGoToPosition();
	String rfMoveDelta();
	String rfGetTemperature();
	String rfSetPosition();
	String rfGetCurPosition();
	String rfConfigureMotor();
	String rfSetMaxTravel();
	String rfFourOutputs();
	char RfCheckSum(String s);


};

