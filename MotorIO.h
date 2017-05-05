#ifndef MOTORIO_H
#define MOTORIO_H

/*
 *  Prototypes from MotorIO.ino
 */

long motGetPosition();
long motGetTargetPosition();
float motGetMotorSpeed();
bool motIsMoving();
String motGetMotorDirection();
void motSetPosition(long newPos);
void motSetTargetPositionNum(long newPos);
void motSetSpeed(float newSpeed);
void motSetStepMode(int stepMode);
void motStartMove();
void motAbortMove();
boolean RunMotorAsNeeded(boolean needResponseString);

void FlashIn(int count);
void FlashOut(int count);
void FlashYellow(int count);
#endif