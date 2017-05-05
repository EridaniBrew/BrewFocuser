/*
 * Sketch to control the a focuser motor.
 * I am emulating the Moonlight Focuser communication protocol, so we can use the
 * Moonlight Ascom driver to access this controller. This driver supposedly works
 * with Moonlight controllers as well as Lacey's EasyFocus and LazyFocus
 *
 * ASCOM driver communicates commands via USB serial. 
 *
 * Hardware has the following:
 *    In/Out buttons - allow manual movements of the focuser
 *
 *    Stepper Motor - Can either use the identical Robofocus motor
 *             which is Hurst LSG35012F76P or maybe some other motor like
 *             Phidgets 3321_0.
 *
 *    Motor Driver - either Adafruit v2 Kit - v2.3 Shield (out of stock now)
 *             or Sparkfun Autodriver V13 Breakout board. Undecided yet
 *             For now, using older v1 version of Adafruit
 *
 *    Temperature Sensor - Adafruit MCP9808 High Accuracy I2C Temperature 
 *             Sensor Breakout Board. This is a board breakout.
 *             Hopefully I can get temperature compensation to work.
 *
 *
  Version 1.0    - initial version
         
 */



#include "Globals.h"
#include "EasyFocus.h"
#include "Robofocus.h"
#include "Optec.h"
#include "EepromUtil.h"

CBaseProtocol *Protocol;
#define EASYFOCUS 1
#define ROBOFOCUS 2
#define OPTEC 3

void setup()
{
	initPins();
	
	// Check for changing the protocol
	// If ManualInSw is pressed, use the Moonlight / EasyFocus Robofocus protocol
	// If ManualOutSw is pressed, use the Robofocus protocol
	// If both are pressed use Optec protocol
	if ((digitalRead(ManualInSw) == LOW) && (digitalRead(ManualOutSw) == LOW))
		{
		writeLong(ADDR_PROTOCOL, OPTEC);
		digitalWrite(LED_IN, HIGH);
		digitalWrite(LED_OUT, HIGH);
		} else if (digitalRead(ManualInSw) == LOW)
		{
		writeLong(ADDR_PROTOCOL, EASYFOCUS);
		digitalWrite(LED_IN, HIGH);
		}
	else if (digitalRead(ManualOutSw) == LOW)
		{
		writeLong(ADDR_PROTOCOL, ROBOFOCUS);
		digitalWrite(LED_OUT, HIGH);
		}
	
	// wait until switches are released
	while ((digitalRead(ManualInSw) == LOW) || (digitalRead(ManualOutSw) == LOW));
	digitalWrite(LED_IN, LOW);
	digitalWrite(LED_OUT, LOW);

	long protocolSelection = readLong(ADDR_PROTOCOL);
	if (protocolSelection == ROBOFOCUS)
	{
		Protocol = new CRobofocus();
		digitalWrite(LED_OUT, HIGH);
	}
	else if (protocolSelection == OPTEC)
	{
		Protocol = new COptec();
		digitalWrite(LED_IN, HIGH);
		digitalWrite(LED_OUT, HIGH);
	}
	else  
	{
		Protocol = new CEasyFocus();
		digitalWrite(LED_IN, HIGH);
	}
	delay(1000);
	digitalWrite(LED_IN, LOW);
	digitalWrite(LED_OUT, LOW);
	Protocol->Init();

  if (mg.gMaxPosition < 100) 
    {
    mg.gMaxPosition = 30000;
    writeLong(ADDR_MAX_POSITION, mg.gMaxPosition);
    }
}

void loop()                     // run over and over again
{
	String complStatus = "";        // holds the response string back to the client program on PC
  
	if (Protocol->ReadCommand())
		{
		//Serial.println(inCommand);
		complStatus = Protocol->ProtocolProcessCommand();
		if (complStatus != "")
			Protocol->WriteResponse(complStatus);
		}

	tempStartNextReading();

	CheckManualSwitches();
	DoCompensation();
  //while (RunMotorAsNeeded(true));
	if (!RunMotorAsNeeded(true))
		{
		// only delay if motor is not active
    delay(100);
    }
}

