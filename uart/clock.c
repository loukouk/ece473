#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

#include "LCDDriver.h"
#include "uart_functions.h"

char exterior_temp[8] = "   he   ";


void request_temp() {
	uart_putc('C');
	_delay_us(50);
}

ISR(USART0_RX_vect)
{
	PORTB++;
	LCD_PutChar(UDR0);
}

int main()
{
	DDRB = 0xff;
	PORTB = 0;
	LCD_Init();
	LCD_Clr();
	uart_init();

	sei();

	while(1){     //do forever

	request_temp();
	_delay_ms(1000);

	} //while 
} //main
