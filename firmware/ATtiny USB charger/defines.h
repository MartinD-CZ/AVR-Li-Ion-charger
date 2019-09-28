/*
 * defines.h
 */ 


#ifndef DEFINES_H_
#define DEFINES_H_

#define F_CPU			20000000UL

#define PWM_START		TCCR0B = (1 << CS00)
#define PWM_STOP		TCCR0B = 0x00; DDRB &=~(1 << PB2)

#define STX_HIGH		PORTA |= (1 << PA7)
#define STX_LOW			PORTA &=~(1 << PA7)
#define SRX_LOW			!(PINA & (1 << PINA6))
#define SUART_BAUD		19200

#define SUART_DELAY		1000000/SUART_BAUD

#define Vmax			0
#define Vpre			1
#define Imax			2
#define Icut			3
#define Ipre			4
#define Vcon			5
#define Tmin			6
#define Tmax			7

#endif /* DEFINES_H_ */