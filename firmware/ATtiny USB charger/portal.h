/*
 * portal.h
 */ 


#ifndef PORTAL_H_
#define PORTAL_H_

bool readEEPROM(void);
void configPortal(void);
void printData(void);
void getActualValues();
uint16_t convertInt();
void saveEEPROM();
void checkMinMax(uint16_t* value, uint8_t arrayIndex);

#define EEPROM_FLAG		0xA1


#endif