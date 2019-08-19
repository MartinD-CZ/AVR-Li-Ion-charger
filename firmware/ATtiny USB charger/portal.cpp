/*
 * portal.cpp
 *
 * Created: 10.08.2019 18:29:13
 * Author: martin@embedblog.eu
 * This file provides procedures for sending and working with the configuration portal
 */ 

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include "portal.h"
#include "UART/uart.h"
#include <ctype.h>
#include <stdlib.h>

extern uint16_t Vmax, Vpre, Imax, Icut, Ipre, Vcon;
extern int8_t Tmin, Tmax;

extern volatile bool dataReady;
extern volatile char inputBuffer[16];

const uint16_t PROGMEM defaultValues[] = {4200, 3000, 1000, 100, 200, 0, 45, 700};
const uint16_t PROGMEM minValues[] = {500, 500, 100, 10, 10, 0, 0, 100};
const uint16_t PROGMEM maxValues[] = {4500, 4500, 2000, 2000, 2000, 50, 125, 4000};
const char PROGMEM paramName[] = "VmaxVpreImaxIcutIpreTminTmaxVcon";
const char PROGMEM unit[] = "mVmVmAmAmA C CmV";
uint16_t actValue[8];

bool readEEPROM(void)
{
	if (eeprom_read_byte(0) == EEPROM_FLAG)
	{
		//we have a valid data
		Vmax = eeprom_read_word((const uint16_t*)1);
		Vpre = eeprom_read_word((const uint16_t*)3);
		Imax = eeprom_read_word((const uint16_t*)5);
		Icut = eeprom_read_word((const uint16_t*)7);
		Ipre = eeprom_read_word((const uint16_t*)9);
		Tmin = eeprom_read_byte((const uint8_t*)10);
		Tmax = eeprom_read_byte((const uint8_t*)11);
		Vcon = eeprom_read_word((const uint16_t*)12);

		return true;
	}
	else
	{
		Vmax = pgm_read_word(&defaultValues[0]);
		Vpre = pgm_read_word(&defaultValues[1]);
		Imax = pgm_read_word(&defaultValues[2]);
		Icut = pgm_read_word(&defaultValues[3]);
		Ipre = pgm_read_word(&defaultValues[4]);
		Tmin = pgm_read_word(&defaultValues[5]);
		Tmax = pgm_read_word(&defaultValues[6]);
		Vcon = pgm_read_word(&defaultValues[7]);

		return false;
	}
}

void configPortal(void)
{
	printData();

	while (1)
	{
		if (dataReady)
		{
			dataReady = false;

			for (uint8_t i = 0; i < 4; i++)
				inputBuffer[i] = tolower(inputBuffer[i]);

			uint32_t receivedCode = ((uint32_t)inputBuffer[0] << 24) | ((uint32_t)inputBuffer[1] << 16) | ((uint32_t)inputBuffer[2] << 8)| ((uint32_t)inputBuffer[3] << 0);

			int8_t index = -1;
			switch (receivedCode)
			{
				case 0x64617461:		//command 'data'
					printData();
					break;
				case 0x65786974:		//command 'exit'
					saveEEPROM();
					uart_sendString_P(PSTR("data saved, restarting\n"));
					wdt_enable(WDTO_15MS);
					while (1);
					break;
				case 0x766d6178:		//Vmax
					index = 0;
					Vmax = convertInt();
					checkMinMax(&Vmax, index);
					break;
				case 0x76707265:		//Vpre
					index = 1;
					Vpre = convertInt();
					checkMinMax(&Vpre, index);
					break;
				case 0x696d6178:		//Imax
					index = 2;
					Imax = convertInt();
					checkMinMax(&Imax, index);
					break;
				case 0x69637574:		//Icut
					index = 3;
					Icut = convertInt();
					checkMinMax(&Icut, index);
					break;
				case 0x69707265:		//Ipre
					index = 4;
					Ipre = convertInt();
					checkMinMax(&Ipre, index);
					break;
				case 0x746d696e:		//Tmin
					index = 5;
					Tmin = convertInt();
					checkMinMax((uint16_t*)&Tmin, index);
					break;
				case 0x746d6178:		//Tmax
					index = 6;
					Tmax = convertInt();
					checkMinMax((uint16_t*)&Tmax, index);
					break;
				case 0x76636f6e:		//Vcon
					index = 7;
					Vcon = convertInt();
					checkMinMax(&Vcon, index);
					break;
				default:
					uart_sendString_P(PSTR("Invalid command!\n"));
			}

			if (index >= 0)
			{
				for (uint8_t j = 0; j < 4; j++)
					uart_sendChar(pgm_read_byte(&paramName[index * 4 + j]));

				uart_sendString_P(PSTR(" set to "));

				getActualValues();
				uart_sendNumber(actValue[index]);

				uart_sendChar(' ');
				uart_sendChar(pgm_read_byte(&unit[index * 2]));
				uart_sendChar(pgm_read_byte(&unit[index * 2 + 1]));
				uart_sendChar('\n');
			}
			
			for (uint8_t i = 0; i < 12; i ++)
				inputBuffer[i] = ' ';
		}
	}
}

void printData(void)
{
	dataReady = false;
	uart_sendString_P(PSTR("\n\n\n=== AVR Li-Ion Charger ===\n"));
	uart_sendString_P(PSTR("rev. 1.0      embedblog.eu\n\n"));
	uart_sendString_P(PSTR("to change parameters, send: paramName=Value   for example: Vmax=4500\ndo not forget to send the NewLine (aka LineFeed) character at the end\n\n"));
	uart_sendString_P(PSTR("paramName   unit    value   min     max     default\n"));
	//characters:			12			6	    8	    8       8

	getActualValues();
	uint8_t numLength;
	for (uint8_t i = 0; i < 8; i ++)
	{
		for (uint8_t j = 0; j < 4; j++)
			uart_sendChar(pgm_read_byte(&paramName[i * 4 + j]));
		uart_sendCharRepeat(' ', 8);

		uart_sendChar(pgm_read_byte(&unit[i * 2]));
		uart_sendChar(pgm_read_byte(&unit[i * 2 + 1]));
		uart_sendCharRepeat(' ', 6);

		numLength = uart_sendNumber(actValue[i]);
		uart_sendCharRepeat(' ', 8 - numLength);

		numLength = uart_sendNumber(pgm_read_word(&minValues[i]));
		uart_sendCharRepeat(' ', 8 - numLength);

		numLength = uart_sendNumber(pgm_read_word(&maxValues[i]));
		uart_sendCharRepeat(' ', 8 - numLength);

		uart_sendNumber(pgm_read_word(&defaultValues[i]));
		uart_sendChar('\n');
	}

	uart_sendString_P(PSTR("\nto print the table again, send DATA; to save & exit, send EXIT\n"));
}

void getActualValues()
{
	actValue[0] = Vmax;
	actValue[1] = Vpre;
	actValue[2] = Imax;
	actValue[3] = Icut;
	actValue[4] = Ipre;
	actValue[5] = Tmin;
	actValue[6] = Tmax;
	actValue[7] = Vcon;
}

uint16_t convertInt()
{
	char number[4];

	for (uint8_t i = 0; i < 4; i ++)
		number[i] = inputBuffer[5 + i];

	uint16_t result = atoi(number);
	return result;
}

void saveEEPROM()
{
	eeprom_write_byte((uint8_t*)0, EEPROM_FLAG);
	eeprom_busy_wait();
	eeprom_write_word((uint16_t*)1, Vmax);
	eeprom_busy_wait();
	eeprom_write_word((uint16_t*)3, Vpre);
	eeprom_busy_wait();
	eeprom_write_word((uint16_t*)5, Imax);
	eeprom_busy_wait();
	eeprom_write_word((uint16_t*)7, Icut);
	eeprom_busy_wait();
	eeprom_write_word((uint16_t*)9, Ipre);
	eeprom_busy_wait();
	eeprom_write_byte((uint8_t*)10, Tmin);
	eeprom_busy_wait();
	eeprom_write_byte((uint8_t*)11, Tmax);
	eeprom_busy_wait();
	eeprom_write_word((uint16_t*)12, Vcon);
	eeprom_busy_wait();
}

void checkMinMax(uint16_t* value, uint8_t arrayIndex)
{
	uint16_t max = pgm_read_word(&maxValues[arrayIndex]);
	uint16_t min = pgm_read_word(&minValues[arrayIndex]);
	
	if (*value > max)
		*value = max;
	else if (*value < min)
		*value = min;
	else
		return;

	uart_sendString_P(PSTR("Value clipped - "));
}
