/*
 * defines.h
 */ 


#ifndef DEFINES_H_
#define DEFINES_H_

#define F_CPU			20000000UL

#define PWM_START		TCCR0B = (1 << CS00)
#define PWM_STOP		TCCR0B = 0x00

#define STX_HIGH		PORTA |= (1 << PA7)
#define STX_LOW			PORTA &=~(1 << PA7)
#define SRX_LOW			!(PINA & (1 << PINA6))
#define SUART_BAUD		19200

#define SUART_DELAY		1000000/SUART_BAUD

#endif /* DEFINES_H_ */