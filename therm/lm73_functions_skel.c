// lm73_functions.c       
// Roger Traylor 11.28.10

#include <util/twi.h>
#include "lm73_functions_skel.h"
#include <util/delay.h>
#include <stdlib.h>

volatile uint8_t lm73_wr_buf[2];
volatile uint8_t lm73_rd_buf[2];

void lm73_temp_convert(char temp_digits[], uint16_t lm73_temp, uint8_t f_not_c){

	int32_t dec_val_times_10 = 0;
	char dec_str_times_10[3];

	if (0x8000 & lm73_temp) {
		temp_digits[0] = '-';
		dec_val_times_10 = 0xFFFFFFFF;
		dec_val_times_10 &= lm73_temp >> 2;
	}
	else {
		temp_digits[0] = '+';	
		dec_val_times_10 |= lm73_temp >> 2;
	}


	if (f_not_c) {
		temp_digits[5] = 'F';
		dec_val_times_10 = ((18 * dec_val_times_10) >> 5) + 320;
	}
	else {
		dec_val_times_10 = (10 * dec_val_times_10) >> 5;
		temp_digits[5] = 'C';

		if (dec_val_times_10 > 290)
			PORTB |= 0xF0;
		else
			PORTB &= 0x0F;

	}


	itoa(dec_val_times_10, dec_str_times_10, 10);

	temp_digits[1] = dec_str_times_10[0];
	temp_digits[2] = dec_str_times_10[1];
	temp_digits[3] = '.';
	temp_digits[4] = dec_str_times_10[2];

}//lm73_temp_convert
