/*
 * Routines to run the motor shield
 */
#include <math.h>

// pin definitions
#define MANUAL_SW_ON    LOW
#define MANUAL_SW_OFF   HIGH

struct motorGlobals mg;


/* Motor stuff */
#define STEPS_PER_ROT 3600
#define STEPPER_PORT 1          // 2 for the other stepper port

#define NORMAL_MAX_SPEED  100.0    // normal speed of motor
#define FAST_MAX_SPEED    300.0    // after 2 seconds of manual button movement, go faster
#define NORMAL_ACCEL      100.0

Adafruit_MotorShield AFMShield(0x60);      // the motor shield
Adafruit_StepperMotor *motor1 = AFMShield.getStepper(STEPS_PER_ROT, STEPPER_PORT);    // 200 steps per rot, port 1

// you can change these to DOUBLE or INTERLEAVE or MICROSTEP!
void forwardstep1() {  
  motor1->onestep(FORWARD, mg.gMotorStepMode);
}
void backwardstep1() {  
  motor1->onestep(BACKWARD, mg.gMotorStepMode);
}

AccelStepper stepper1(forwardstep1, backwardstep1);

// global parameters controlling the movements

// initPins - initialize the various pins
//
void initPins()
{
  pinMode (ManualInSw, INPUT_PULLUP);
  pinMode(ManualOutSw, INPUT_PULLUP);
  pinMode(EXPANDED_TEMPERATURE, INPUT_PULLUP);
  
  Serial.begin(9600);			//Optec needs 19200

  pinMode(LED_IN,OUTPUT);
  pinMode(LED_OUT,OUTPUT);
  digitalWrite (LED_IN, LOW);
  digitalWrite (LED_OUT, LOW);
  pinMode(LED_TEMP_COMP_ENABLED, OUTPUT);
  
  AFMShield.begin();      // init the shield
  stepper1.setMaxSpeed(NORMAL_MAX_SPEED);    //  steps per sec   Max is 1000
  stepper1.setAcceleration(NORMAL_ACCEL);  //  steps per sec ^2
  mg.gTargetPosition = readLong(ADDR_TARGET_POSITION);
  mg.gMaxPosition = readLong(ADDR_MAX_POSITION);
  stepper1.setCurrentPosition(mg.gTargetPosition);
  stepper1.moveTo(mg.gTargetPosition);
  
  if (! TempSensorInit())
      {
	  long startTime = millis();
	  long endTime = startTime;
      while (endTime - startTime < 5000)
		  {
		  digitalWrite(LED_TEMP_COMP_ENABLED, HIGH);
		  delay(100);
		  digitalWrite(LED_TEMP_COMP_ENABLED, LOW);
		  delay(100);
		  endTime = millis();
		  }
	  }
  SetCompensationValues();
}

// motGetPosition()
//  returns the current position of the focuser 
long motGetPosition()
{
	long curPos = stepper1.currentPosition();
	return (curPos);
}


// GetTargetPosition
//    returns long target position
long motGetTargetPosition()
{
    return (stepper1.targetPosition());
}


// GetMotorSpeed
//    return float speed
float motGetMotorSpeed()
{
    return (stepper1.speed());
}

// GetIsMoving
//    true if moving
bool motIsMoving()
{
	bool ret = true;
	if (stepper1.distanceToGo() == 0)
        {
        ret = false;
        }
    return (ret);
}

// motGetMotorDirection
//  return "I" if moving in,
//         "O" if moving out,
//         "" if not moving
String motGetMotorDirection()
{
	String ret = "";
	if (stepper1.distanceToGo() < 0)
	{
		ret = "I";
	}
	else if (stepper1.distanceToGo() > 0)
	{
		ret = "O";
	}
	return (ret);
}

// motSetPosition   
//    newPos is the designated new current position
void motSetPosition(long newPos)
{
    stepper1.setCurrentPosition(newPos);
	mg.gTargetPosition = newPos;
	writeLong(ADDR_TARGET_POSITION, mg.gTargetPosition);		// update eeprom
	return ;
}

// SetTargetPositionNum   
//    new target Position is long
//    We save the target position in global. 
//    Later the StartMove will start the actual move
void motSetTargetPositionNum(long newPos)
{
	mg.gTargetPosition = newPos;
	writeLong(ADDR_TARGET_POSITION, mg.gTargetPosition);		// update eeprom
	return ;
}

// motSetSpeed   :SDxx
//    Speed is 250,125,...
void motSetSpeed(float newSpeed)
{
    stepper1.setSpeed(newSpeed);
    return ;
}

// motSetStepMode
//    set motor to full/half step
//    I am assuming this means use SINGLE mode
//    or MULTISTEP?
void motSetStepMode(int stepMode)
{
	mg.gMotorStepMode = stepMode;
    return;
}

// motStartMove  
//
void motStartMove()
{
    // Set the motor to move
    // subsequent .run() will start stepping
	stepper1.moveTo(mg.gTargetPosition);
	mg.gMotorRunning = true;
	if (mg.gTargetPosition < stepper1.currentPosition())
		{
		digitalWrite(LED_IN, HIGH);
		digitalWrite(LED_OUT, LOW);
		}
	else
		{
		digitalWrite(LED_OUT, HIGH);
		digitalWrite(LED_IN, LOW);
		}
    return ;
}

// motAbortMove
//
void motAbortMove()
{
	mg.gTargetPosition = stepper1.currentPosition();
	//if (mg.gTargetPosition > mg.gMaxPosition)
	//	{
	//	Serial.println("motAbortMove: Target Pos > Max");
	//	//mg.gTargetPosition = mg.gMaxPosition ;
	//	//stepper1.setCurrentPosition(mg.gTargetPosition);
	//	}
	//if (mg.gTargetPosition < 1)
	//	{
	//	Serial.println("motAbortMove: Target Pos < 1");
	//	//mg.gTargetPosition = 1;
	//	//stepper1.setCurrentPosition(mg.gTargetPosition);
	//	}
	writeLong(ADDR_TARGET_POSITION, mg.gTargetPosition);
	stepper1.moveTo(mg.gTargetPosition);    
	if (stepper1.distanceToGo() != 0)
		{
		stepper1.run();
		}
	
	digitalWrite(LED_IN, LOW);
	digitalWrite(LED_OUT, LOW);
	mg.gMotorRunning = false;
    return ;
}

// RunMotorAsNeeded
//   let the motor run the next step if moving
//   If not moving, do nothing
//   needResponseString true - need to issue an end of movement command
//       true - if responding to a Protocol command
//       false - when running motors manually
//   return true if motor is busy, false if not
boolean RunMotorAsNeeded(boolean needResponseString)
{
	if (!mg.gMotorRunning) return(false);

    boolean was_stepped = stepper1.run();
		
    if (mg.gMotorRunning && !was_stepped)
        {  // was running, now its not
		mg.gMotorRunning = false;
		SetCompensationValues();
		digitalWrite(LED_IN, LOW);
		digitalWrite(LED_OUT, LOW);
		if (needResponseString)
			{  // don't do this if manual switch running
			Protocol->MovementDone();
			}
		}
    else if (was_stepped) 
        {	// moved the motor
		Protocol->MotorStepped();
		mg.gMotorRunning = true;
        }
	return (was_stepped);
    
}



// CheckManualSwitches
// If the user pushes Manual switch start moving to the farthest limit
// If we are moving, 
//   if switch is released, stop the move
void CheckManualSwitches()
{
    int moveInSw = digitalRead(ManualInSw);
    int moveOutSw = digitalRead(ManualOutSw);
	
    if (moveInSw == MANUAL_SW_ON)   
        {
		RunMotorManually(ManualInSw, LED_IN, 1);
		}

    if (moveOutSw == MANUAL_SW_ON)   
        {
		RunMotorManually(ManualOutSw, LED_OUT, mg.gMaxPosition);
		}

}

// RunMotorManually
//   When the manual run switch is pressed, run the motor until the switch is released
void RunMotorManually(int manualSwitch, int led, long target)
{
	int moveSw = digitalRead(manualSwitch);
	long startMoveTime = 0;
	
	startMoveTime = millis();
	digitalWrite(led, HIGH);
	// start moving motor
	motSetTargetPositionNum(target);
	motStartMove();
	long pos = motGetPosition();
	while (((manualSwitch == ManualInSw) && (moveSw == MANUAL_SW_ON) && (pos > 1)) ||
		   ((manualSwitch == ManualOutSw) && (moveSw == MANUAL_SW_ON) && (pos < mg.gMaxPosition) ))
		{
		RunMotorAsNeeded(false);
		/***
		if ((millis() - startMoveTime) > 2000)
			{
			stepper1.setMaxSpeed(FAST_MAX_SPEED);
			}
			***/
		moveSw = digitalRead(manualSwitch);
		pos = motGetPosition();
		//FlashYellow(1);
		}
	motAbortMove();
	stepper1.setMaxSpeed(NORMAL_MAX_SPEED);
	digitalWrite(led, LOW);
}

// for debugging
void FlashIn(int num)
	{
	for (int i = 0; i < num; i++)
		{
		digitalWrite(LED_IN, HIGH);
		//delay(100);
		digitalWrite(LED_IN, LOW);
		//delay(100);
		}
	}

void FlashOut(int num)
	{
	for (int i = 0; i < num; i++)
		{
		digitalWrite(LED_OUT, HIGH);
		delay(100);
		digitalWrite(LED_OUT, LOW);
		delay(100);
		}
	}

// FlashYellow (num)
// For debugging/informational purpose, flash the yellow LED num times.
void FlashYellow(int num)
{
	for (int i = 0; i < num; i++)
		{
		digitalWrite(LED_TEMP_COMP_ENABLED, HIGH);
		delay(5);		// don't delay very long, or the motor will be choppy
		digitalWrite(LED_TEMP_COMP_ENABLED, LOW);
		//delay(100);
		}
}


