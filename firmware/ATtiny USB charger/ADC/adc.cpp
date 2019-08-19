/*
 * adc.cpp
 *
 * Created: 07.08.2019 8:19:57
 * Author: martin@embedblog.eu
 * a simple ADC routine to get data from a given channel
 */ 

#include <avr/io.h>
#include "adc.h"
#include "..\defines.h"

//return the value of a given channel in milivolts (with a 4.096 V reference at PA0)
uint16_t adc_measurement(uint8_t channel, uint8_t samples)
{
	switch (channel)
	{
		case ADC_VOUT:
			ADMUX = (1 << REFS0) | (1 << MUX0) | (1 << MUX1);
			break;
		case ADC_IOUT:
			ADMUX = (1 << REFS0) | (1 << MUX0) | (1 << MUX2) | (1 << MUX3);
			break;
		case ADC_TEMP:
			ADMUX = (1 << REFS0) | (1 << MUX2);
			break;
	}

	ADCSRA = (1 << ADEN) | (1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2);

	//start first conversion
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));

	uint16_t data = 0;
	for (uint8_t i = 0; i < samples; i ++)
	{
		ADCSRA |= (1 << ADSC);
		while (ADCSRA & (1 << ADSC));
		data += ADC;
	}

	return ((data * 4) / samples);
}