#ifndef TEMPCOMP_H
#define TEMPCOMP_H

/*
*  Prototypes from MotorIO.ino
*/

void tempStartReadTemperature();
void tempFinishReadTemperature();
void tempSetTemperatureScale(long slope);
long tempGetTempCoeff();
String EnableTemperatureCompensation(boolean start);
void SetCompensationValues();


#endif