/*
 * adc.h
 */ 


#ifndef ADC_H_
#define ADC_H_

#define ADC_VOUT	0
#define ADC_IOUT	1
#define ADC_TEMP	2

uint16_t adc_measurement(uint8_t channel, uint8_t samples);

#endif /* ADC_H_ */