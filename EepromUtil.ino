/*
 * Routines used to read/write value to EEPROM
 * 
 */

#define ADDR_TARGET_POSITION   0
#define ADDR_MAX_POSITION   4

long readLong(int addr)
{
	long val = 0;
	EEPROM.get(addr, val);
	return (val);
}

void writeLong(int addr, long val)
{
	EEPROM.put(addr, val);
}
