/*
 * softUART.cpp
 *
 * Author : martin@embedblog.eu
 * A simple software-implemented UART with some slightly advanced functions for printing
 */ 

#include "..\defines.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include "uart.h"

volatile char inputBuffer[12];
volatile uint8_t bufferPos;
bool dataReady;

void uart_init()
{
	//setup TX pin - PA7
	DDRA |= (1 << DDA7);
	STX_HIGH;

	//setup RX pin - PA6
	DDRA &=~(1 << DDA6);
	GIMSK |= (1 << PCIE0);
	PCMSK0 |= (1 << PCINT6);

	//fill input buffer with predefined values
	for (uint8_t i = 0; i < 12; i++)
		inputBuffer[i] = ' ';
}

void uart_sendChar(char ch)
{
	cli();
	STX_LOW;
	_delay_us(SUART_DELAY);
	STX_HIGH;

	for (uint8_t i = 0; i < 8; i++)
	{
		if ((ch >> i) & 0x01)
			STX_HIGH;
		else
			STX_LOW;
		_delay_us(SUART_DELAY);
	}

	STX_HIGH;
	sei();
	_delay_us(SUART_DELAY);
}

void uart_sendString(char* s)
{
	while(*s)  uart_sendChar(*s++);
}

void uart_sendString_P(const char* s)
{
	while (pgm_read_byte(s)) uart_sendChar(pgm_read_byte(s++));
}

uint8_t uart_sendNumber(uint16_t num, uint8_t base)
{
	char buf[6];
	itoa(num, buf, base);
	uart_sendString(buf);

	return (num < 10 ? 1 : (num	< 100 ? 2 : (num < 1000 ? 3 : (num < 10000 ? 4 : 5))));
}

void uart_sendCharRepeat(char ch, uint8_t repeats)
{
	for (uint8_t i = 0; i < repeats; i ++)
		uart_sendChar(ch);
}

//note: putting delays in interrupts is far from ideal. However, since we are out of timers, we don't really have a choice here...
ISR(PCINT0_vect)
{
	if (SRX_LOW)
	{
		uint8_t data = 0x0;

		_delay_us(SUART_DELAY + (SUART_DELAY / 2));

		for (uint8_t i = 0; i < 8; i ++)
		{
			if (!SRX_LOW)
			data |= (1 << i);
			_delay_us(SUART_DELAY);
		}

		if (data != '\n')
			inputBuffer[bufferPos++] = data;
		else
		{
			dataReady = true;
			bufferPos = 0;
		}
	}
}
