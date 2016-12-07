#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "uart_functions_m48.h"
#include "twi_master.h"
#include "lm73_functions.h"

uint16_t mega48_temp = 0;
volatile char temp_str[8];

ISR(USART_RX_vect)
{
	uint8_t i;
	char data;

	data = (char)UDR0;
	memset(temp_str, ' ', 8);

	if (data == 'F')
		lm73_temp_convert(temp_str, mega48_temp, 1);
	else if (data == 'C')
		lm73_temp_convert(temp_str, mega48_temp, 0);
	else {
		temp_str[0] = 'N';
		temp_str[1] = 'U';
		temp_str[2] = 'L';
		temp_str[3] = 'L';
	}

	for (i = 0; i < 8; i++) {
		uart_putc(temp_str[i]);
//		_delay_us(10);
	}
}

void measure_temp()
{
	uint8_t rd_buf[2];

	twi_start_rd(LM73_ADDRESS, rd_buf, 2);
	_delay_ms(4);
	mega48_temp = rd_buf[0];
	mega48_temp = mega48_temp << 8;
	mega48_temp |= rd_buf[1];
}

int main()
{
	uint8_t i;
	init_twi();
	sei();
	lm73_init();
	measure_temp();
	uart_init();

	while(1) {
		measure_temp();
		_delay_ms(50);
/*	_delay_ms(5000);
	PORTB=0xFF;
	_delay_ms(10);
	PORTB=0x00;
	_delay_ms(10);
	PORTB=0xFF;
	_delay_ms(10);
	PORTB=0x00;
	_delay_ms(10);
	for (i = 0; i < 16; i++) {	
		if (mega48_temp & (1<<i))
			PORTB = 0xFF;
		else
			PORTB = 0x00;
		_delay_ms(2000);
	}
*/	}
}
