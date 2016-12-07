// lm73_functions.c       
// Roger Traylor 11.28.10

#include <util/twi.h>
#include "lm73_functions.h"
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>

void lm73_init()
{
	uint8_t lm73_wr_buf[2];

	lm73_wr_buf[0] = LM73_PTR_CONFIG;
	lm73_wr_buf[1] = LM73_CONFIG_VALUE0; 
	twi_start_wr(LM73_ADDRESS, lm73_wr_buf, 2);
	_delay_ms(2);
	
	lm73_wr_buf[0] = LM73_PTR_CTRL_STATUS;
	lm73_wr_buf[1] = LM73_CONFIG_VALUE1; 
	twi_start_wr(LM73_ADDRESS, lm73_wr_buf, 2);
	_delay_ms(2);

	lm73_wr_buf[0] = LM73_PTR_TEMP;
	twi_start_wr(LM73_ADDRESS, lm73_wr_buf, 1);
	_delay_ms(2);
}

void lm73_temp_convert(char temp_digits[], uint16_t lm73_temp, uint8_t f_not_c){

	int32_t dec_val_times_100 = 0;
	char temp_char, char_sign, dec_str_times_100[4] = {'\0','\0','\0','\0'};

	if (lm73_temp & 0x8000) {
		lm73_temp ^= 0xFFFF;
		lm73_temp += 1;
		char_sign = '-';
	}
	else
		char_sign = ' ';

	dec_val_times_100 = lm73_temp;

	if (f_not_c) {
		if (char_sign == '-') {
			dec_val_times_100 *= -1;
		}
		dec_val_times_100 = ((180 * dec_val_times_100) >> 7) + 3200;
		temp_char = 'F';
		if (dec_val_times_100 >= 0)
			char_sign = ' ';
		else
			dec_val_times_100 *= -1;
	}
	else {
		dec_val_times_100 = ((100 * dec_val_times_100) >> 7);
		temp_char = 'C';
	}

	itoa(dec_val_times_100, dec_str_times_100, 10);

	temp_digits[0] = char_sign;
	if (dec_val_times_100 < 1000) {
		temp_digits[1] = dec_str_times_100[0];
		temp_digits[2] = '.';
		temp_digits[3] = dec_str_times_100[1];	
		temp_digits[4] = temp_char;
	}
	else if (dec_val_times_100 < 10000) {
		temp_digits[1] = dec_str_times_100[0];
		temp_digits[2] = dec_str_times_100[1];
		temp_digits[3] = '.';
		temp_digits[4] = dec_str_times_100[2];
		temp_digits[5] = temp_char;
	}
	else {
		temp_digits[1] = dec_str_times_100[0];
		temp_digits[2] = dec_str_times_100[1];
		temp_digits[3] = dec_str_times_100[2];
		temp_digits[4] = '.';
		temp_digits[5] = dec_str_times_100[3];
		temp_digits[6] = temp_char;
	}

}//lm73_temp_convert
