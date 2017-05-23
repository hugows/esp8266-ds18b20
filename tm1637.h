#ifndef TM1637_H
#define TM1637_H

#include <stdint.h>
/**
 * Display descriptor. Fill it before use.
 */
typedef struct
{
	uint8_t pin_clock;
	uint8_t pin_data;

	bool backlight; 
	bool colon;
	uint8_t brightness;
} tm1637_t;


void tm1637_init(const tm1637_t *disp);
void tm1637_set_colon(tm1637_t *disp, bool on);
void tm1637_set_brightness(tm1637_t *disp, uint8_t brightness, bool on);
void tm1637_set_segments(const tm1637_t *disp, uint8_t segments[], uint8_t length, uint8_t pos);
uint8_t tm1637_encode_digit(uint8_t digit);

void tm1637_show_number_dec(const tm1637_t *disp, int num, uint8_t pos);
void tm1637_show_number_dec_ex(const tm1637_t *disp, int num, uint8_t dots, bool leading_zero,
                                    uint8_t length, uint8_t pos);
void tm1637_clear(const tm1637_t *disp);

#endif

