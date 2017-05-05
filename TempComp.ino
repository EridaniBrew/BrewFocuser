/*
 * Routines for accessing temperature and performing Temperature Compensation
 * Temperature readings made using MCP9808 High Accuracy I2C Temperature Sensor Breakout Board
 * from Adafruit. It has good accuracy (0.25C) and precision (0.0625C)
 * Uses address 0x18 on I2C.
 * Uses SCL and SDA pins for communication
 */
#include <Wire.h>

// Create the MCP9808 temperature sensor object
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();

boolean gCompensationActive = false;    // track whether compensation is enabled
float gLastTemperature = 0.0;           // temperature after last move
long gLastPosition = 0;                 // position corresponding the LastTemperature
unsigned long gStartTempTime = 0;				// delay 250 msec for best precision (0.0625 C)
										//       130 msec (0.125 C)
										//        65 msec (0.25 C)
										//        30 msec (0.5 C)
#define TARGET_PRECISION_TIME  250

float gTempSlope = 100;                 // Steps per degC
#define MIN_POS_CHANGE  10              // Min position change to cause compensation shift


// TempSensorInit
// Initialize the temperature sensor (part of initPins)
// Start the first temperature reading
boolean TempSensorInit()
{
	boolean ret = tempsensor.begin();
	if (ret)
		{
		tempStartReadTemperature();
		}
	return (ret);
}

// tempStartReadTemperature()
//    reads temperature. Subsequent GetTemperature returns the temperature
void tempStartReadTemperature()
{
	tempsensor.shutdown_wake(0); // wake up
	gStartTempTime = millis();
	return ;
}

// tempFinishReadTemperature
// reads temperature (enough time should have elapsed)
void tempFinishReadTemperature()
{
	mg.gTemperature = tempsensor.readTempC();
	tempsensor.shutdown_wake(1);  // back to sleep
								  //Serial.print("Temp "); Serial.println(c);
	return ;
}

// Check on whether to start next temperature reading.
// It takes 250 msec to read
void tempStartNextReading()
{
	// look for wrap condition
	if (millis() < gStartTempTime)
	{
		gStartTempTime = millis();
		return;
	}
	
	if ((millis() - gStartTempTime) > TARGET_PRECISION_TIME)
		{
		tempFinishReadTemperature();
		//Serial.print("Temp is "); Serial.println(mg.gTemperature);
		tempStartReadTemperature();
		}
}

// tempSetTemperatureScale
//    scale (slope) is the compensation coefficient
void tempSetTemperatureScale(long slope)
{
    gTempSlope = slope;
    return;
}

// tempGetTempCoeff
//    return compensation coefficient
long tempGetTempCoeff()
{
	return (gTempSlope);
}

// EnableTemperatureCompensation
// start = true to begin compensation
//         false to stop
String EnableTemperatureCompensation(boolean start)
{
    gCompensationActive = start;
	if (gCompensationActive)
		{
		digitalWrite(LED_TEMP_COMP_ENABLED, HIGH);
		}
	else
		{
		digitalWrite(LED_TEMP_COMP_ENABLED, LOW);
		}
    return(F(""));
}

// SetCompensationValues
// the motor has stopped running, so measure temperature and position
// for later calculations
void SetCompensationValues()
{
	gLastTemperature = mg.gTemperature;
	gLastPosition  = mg.gTargetPosition;
}

// DoCompensation
// If compensation is active, do the check
//^Pos               Y (curTemp,expected)
//|
//|
//| X (gLastTemp,gLastPos)
//|   ---------> Temp
//   ExpectedPosition = deltaTemp * slope + gLastPos
//   If Expected - gLastPos > someMin
//      move to Expected
void DoCompensation()
{
    if (gCompensationActive && ! mg.gMotorRunning)
        {
		long expected = (mg.gTemperature - gLastTemperature) * gTempSlope + gLastPosition;
        //Serial.println(F("DC: curTemp, gLastTemp, slope, gLastPos, expected "));
        //Serial.print(mg.gTemperature);Serial.print(" ");
        //Serial.print(gLastTemperature);Serial.print(F(" "));
        //Serial.print(gTempSlope);Serial.print(F(" "));
		//Serial.print(gLastPosition); Serial.print(F(" "));
		//Serial.println(expected);
		if (fabs(expected - gLastPosition) > MIN_POS_CHANGE)
            { // move to expected
            motSetTargetPositionNum(expected);
			      motStartMove();
            //Serial.print(F("DoCompensation moving to ")); Serial.println(expected);
            mg.gMotorRunning = true;   
			if (expected > gLastPosition)
				{ // moving out 
				digitalWrite(LED_IN, LOW);
				digitalWrite(LED_OUT, HIGH);
				}
			else
				{ // moving in 
				digitalWrite(LED_IN, HIGH);
				digitalWrite(LED_OUT, LOW);
				}
            }
        }
}



