/*
 * ATtiny Li-Ion charger.cpp
 *
 * Created: 04.08.2019 23:07:01
 * Author: Martin Danek, martin@embedblog.eu
 * Fuses: low 0xFF, high 0xDF, ext 0xFF
 */ 

#include <avr/io.h>
#include "defines.h"
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <stdlib.h>
#include "UART/uart.h"
#include "LED/ws2812.h"
#include "ADC/adc.h"
#include "portal.h"

uint16_t Vmax, Vpre, Imax, Icut, Ipre, Vcon;
int8_t Tmin, Tmax;

uint16_t Vact, Iact, Itemp;
uint8_t Tact;

uint8_t color[3];		//GRB
bool precharge;

volatile uint16_t seconds;
volatile bool update;

extern volatile bool dataReady;

//disable watchdog on startup
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));

int main(void)
{
    //init UART
	uart_init();
	uart_sendString_P(PSTR("\n\n\nSTART\n"));

	//init TIM1 - timekeeping
	TCCR1B = (1 << WGM12) | (1 << CS10) | (1 << CS11);
	OCR1A = 31250;
	TIMSK1 = (1 << OCIE1A);

	sei();

	//init LED
	DDRA |= (1 << DDA5);

	if (readEEPROM())
		uart_sendString_P(PSTR("Data loaded from EEPROM\n"));
	else
		uart_sendString_P(PSTR("Nothing in EEPROM, defaults used\n"));

	//wait for user to connect the battery
	uart_sendString_P(PSTR("waiting for battery connection...\n"));
	while (1)
	{
		//we have something connected, skip setup
		if ((adc_measurement(ADC_VOUT, 8) * 2) > Vcon)
			break;

		//we have received something over serial, enter setup portal
		if (dataReady)
		{
			color[2] = 128;
			ws2812_sendarray_mask(color, 3, (1 << PA5), (uint8_t*)&PORTA, (uint8_t*)&DDRA);
			configPortal();
		}
		
		uart_sendString_P(PSTR("to start config, send anything to the device\n"));
		color[2] = 128;
		ws2812_sendarray_mask(color, 3, (1 << PA5), (uint8_t*)&PORTA, (uint8_t*)&DDRA);
		_delay_ms(500);
		color[2] = 0;
		ws2812_sendarray_mask(color, 3, (1 << PA5), (uint8_t*)&PORTA, (uint8_t*)&DDRA);
		_delay_ms(500);
	}

	//do we need to precharge?
	if ((adc_measurement(ADC_VOUT, 16) * 2) < Vpre)
	{
		precharge = true;
		Itemp = Imax;
		Imax = Ipre;
	}

	//init TIM0 - 8 bit PWM @ 80 kHz
	TCCR0A = (1 << WGM00) | (1 << WGM01) | (1 << COM0A1) | (1 << COM0A0);
	DDRB |= (1 << DDB2);
	PWM_START;

	update = true;
    while (1)
    {
		Vact = adc_measurement(ADC_VOUT, 16) * 2;
		Iact = adc_measurement(ADC_IOUT, 16) / 2;

		//CC-CV regulation loop
		if (((Vact > Vmax) | (Iact > Imax)) & (OCR0A > 0))
			OCR0A--;
		else if ((Vact < Vmax) & (Iact < Imax) & (OCR0A < 0xFF))
			OCR0A++;

		if (update)
		{
			update = false;

			Tact = (((uint16_t)adc_measurement(ADC_TEMP, 8) - 400) / 20);

			uart_sendString_P(PSTR("Vout: "));
			uart_sendNumber(Vact);
			uart_sendString_P(PSTR(" mV; Iout: "));
			uart_sendNumber(Iact);
			uart_sendString_P(PSTR(" mA; t: "));
			uart_sendNumber(Tact);
			uart_sendString_P(PSTR(" C\n"));
			uart_sendString_P(PSTR("\n"));

			//precharge-CC-CV-finished determination loop
			int16_t Vdelta = Vmax - Vact;
			int16_t Idelta = Imax - Iact;
			if (precharge)
			{
				//precharge - purple color
				color[0] = 35; color[1] = 80; color[2] = 80;
				ws2812_sendarray_mask(color, 3, (1 << PA5), (uint8_t*)&PORTA, (uint8_t*)&DDRA);

				//are the conditions for precharging still true?
				if ((adc_measurement(ADC_VOUT, 16) * 2) > Vpre)
				{
					precharge = false;
					Imax = Itemp;
				}
			}
			else if (abs(Vdelta) >= abs(Idelta))
			{
				//CC mode - red color
				color[0] = 0; color[1] = 128; color[2] = 0;
				ws2812_sendarray_mask(color, 3, (1 << PA5), (uint8_t*)&PORTA, (uint8_t*)&DDRA);
			}
			else
			{
				//CV mode
				if (Iact < Icut)
				{
					//we are finished - green color
					color[0] = 128; color[1] = 0; color[2] = 0;
					ws2812_sendarray_mask(color, 3, (1 << PA5), (uint8_t*)&PORTA, (uint8_t*)&DDRA);
					uart_sendString_P(PSTR("==FINISHED==\n"));
					OCR0A = 0x00;
					PWM_STOP;
					while (1);
				}
			
				//we are not finished - orange color
				color[0] = 50; color[1] = 128; color[2] = 0;
				ws2812_sendarray_mask(color, 3, (1 << PA5), (uint8_t*)&PORTA, (uint8_t*)&DDRA);
			}

			//temperature check
			if ((Tact > Tmax) | (Tact < Tmin))
			{
				uart_sendString_P(PSTR("ERROR\n"));
				OCR0A = 0x00;
				PWM_STOP;
				//color[0] = 0; color[2] = 0;
				while(1);
				/*{
					color[1] = 128;		//does not fit into memory...
					ws2812_sendarray_mask(color, 3, (1 << PA5), (uint8_t*)&PORTA, (uint8_t*)&DDRA);
					_delay_ms(500);
					color[1] = 0;
					ws2812_sendarray_mask(color, 3, (1 << PA5), (uint8_t*)&PORTA, (uint8_t*)&DDRA);
					_delay_ms(500);
				}*/
			}
		}
    }
}

//interrupt each 100 ms
volatile uint8_t OVF_counter;
ISR(TIM1_COMPA_vect)
{
	OVF_counter++;
	if (OVF_counter >= 10)
	{
		OVF_counter = 0;
		seconds++;
		update = true;
	}
}

void wdt_init(void)
{
	MCUSR = 0;
	wdt_disable();
}

