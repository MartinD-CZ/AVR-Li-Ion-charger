/*
 * ws2812.h
 *
 * Created: 07.08.2019 9:01:57
 *  Author: marti
 */ 


#ifndef WS2812_H_
#define WS2812_H_

void ws2812_sendarray_mask(uint8_t *data, uint16_t datlen, uint8_t maskhi, uint8_t *port, uint8_t *portreg);

#endif /* WS2812_H_ */