#ifndef EEPROMUTIL_H
#define EEPROMUTIL_H

/*
* Routines used to read/write value to EEPROM
*
*/

#define ADDR_TARGET_POSITION   0
#define ADDR_MAX_POSITION   4
#define ADDR_PROTOCOL   8

long readLong(int addr);
void writeLong(int addr, long val);

#endif