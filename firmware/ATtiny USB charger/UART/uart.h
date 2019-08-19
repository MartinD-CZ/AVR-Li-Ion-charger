/*
 * softUART.h
 */ 


#ifndef _UART_H_
#define _UART_H_

void uart_init(void);
void uart_sendChar(char ch);
void uart_sendString(char* s);
void uart_sendString_P(const char* s);
uint8_t uart_sendNumber(uint16_t num, uint8_t base = 10);
void uart_sendCharRepeat(char ch, uint8_t repeats);

#endif