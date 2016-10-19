#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "_functions.c"
//#include "hd44780.h"

#define IS_ALARM_TRIGGER (alarm_mode&0x01)
#define ALARM_TRIGGER (alarm_mode|=0x01)
#define ALARM_TRIGGER_CLEAR (alarm_mode&=0xFE)

#define IS_SHOW_ALARM (mode&0x01)
#define IS_SETTING ((mode>>1)&0x01)
#define IS_AM_PM ((mode>>2)&0x01)
#define IS_ALARM_ARM ((mode>>3)&0x01)
#define IS_SNOOZING (mode>>4)

#define SNOOZE_MIN 1

volatile int8_t time[3] 	= {0,0,0};
volatile int8_t alarm[3] 	= {0,0,0};
volatile uint8_t SEGS[5] 	= {0,0,0xFF,0,0};
volatile uint8_t mode 		= 0x00;
volatile uint8_t encoder_mode	= 0x00;
volatile uint8_t alarm_mode	= 0x00;

void split_digits(uint8_t hours, uint8_t minutes)
{
	uint8_t divider;

	if (IS_AM_PM && hours > 12)
		hours -= 12;

	if (encoder_mode) {
		divider = 16;
		SEGS[0] &= ~(1<<PA7);
	}
	else {
		divider = 10;
		SEGS[0] |= 1 << PA7;
	}

	//breaks up time value into its separate digits
	SEGS[4] = hours / divider;
	SEGS[3] = hours % divider;

	SEGS[1] = minutes / divider;
	SEGS[0] = minutes % divider;

	//removes all leading zeroes for a cleaner output
	if (SEGS[4] == 0)
		SEGS[4] = -1;

	//decodes each digit into a value for the 7seg display
	SEGS[4] = decode_digit(SEGS[4]);
	SEGS[3] = decode_digit(SEGS[3]);
	SEGS[1] = decode_digit(SEGS[1]);
	SEGS[0] = decode_digit(SEGS[0]);

	//makes colon blink on for 1sec, off for 1sec
	if ((time[0] % 2) == 0)
		SEGS[2] &= ~(0x03);
	else
		SEGS[2] |= 0x03;

	//set the top dot to be on when alarm is armed
	if (IS_ALARM_ARM)
		SEGS[2] &= ~(0x04);
	else
		SEGS[2] |= 0x04;
}

ISR(TIMER0_OVF_vect)
{
	time[0]++;
	if (time[0] >= 60) {
		time[0] -= 60;
		time[1]++;
		if (time[1] >= 60) {
			time[1] -= 60;
			time[2]++;
			if (time[2] >= 24)
				time[2] -= 24;
		}
	}
	if (IS_ALARM_TRIGGER)
		if (time[1] > alarm[1] || time[2] > alarm[2])
			ALARM_TRIGGER_CLEAR;
	if (IS_ALARM_ARM) {
		if (alarm[0] == time[0] && alarm[1] == time[1] && alarm[2] == time[2])
			ALARM_TRIGGER;
	}
}

ISR(TIMER2_OVF_vect) {
	uint8_t i, dir[2], ports_data[2];
	static uint8_t data;
	static int8_t encoder_count[2] = {0,0};

	if (debounce_PORTC(6) || debounce_PORTC(7)) {
		encoder_mode ^= 1;
	}

	ports_data[0] = PORTA;		//save PORTA data
	ports_data[1] = PORTB & 0x70;	//save PORTA data
	PORTA = 0xFF;			//set all pull up resistors on PORTA
	DDRA  = 0x00;			//set PORTA to all inputs
	PORTB |= 0x70;
	PORTB &= (5 << 4) & 0x70;	//set select bits to take input from pushbuttons
	asm("nop");

	for (i = 0; i < 4; i++) {	//take input with debouncing
		if (debounce_switch(i))
			mode ^= 1 << i;
	}
	for ( ; i < 8; i++) {
		if (debounce_switch(i))
			mode |= 1 << i;
		else
			mode &= ~(1<<i);
	}

	asm("nop");
	PORTB&= 0x8F;
	PORTB|= ports_data[1];		//restore PORTB data
	DDRA  = 0xFF;			//set PORTA back to outputs
	PORTA = ports_data[0];		//restore PORTA data

	if (IS_ALARM_ARM) {
		if (IS_SNOOZING && IS_ALARM_TRIGGER) {
			alarm[1] += SNOOZE_MIN;
			if (alarm[1] >= 60) {
				alarm[2]++;
				if (alarm[2] >= 24)
					alarm[2] -= 24;
			}
			ALARM_TRIGGER_CLEAR;
		}
	}
	else
		ALARM_TRIGGER_CLEAR;

	data = spi_send_read(mode);	//display their mode on bar graph + get input from encoders

	if (IS_SETTING) {
		//for each encoder, determine which direction it is being turned
		for (i = 0; i < 2; i++) {
			dir[i] = find_direction(data >> (i*2), i);

			//increment count if encoders are being turned clockwise
			if (dir[i] == CW) {
				encoder_count[i]++;
			}
			//decrement count if encoders are being turned counter clockwise
			if (dir[i] == CCW) {
				encoder_count[i]--;
			}
			if (encoder_count[i] >= 4) {
				time[2-i]++;
				encoder_count[i] -= 4;
			}
			else if (encoder_count[i] <= -4) {
				time[2-i]--;
				encoder_count[i] += 4;
			}
		}
		if (time[1] >= 60)
			time[1] -= 60;
		else if (time[1] < 0)
			time[1] += 60;
		if (time[2] >= 24)
			time[2] -= 24;
		else if (time[2] < 0)
			time[2] += 24;
	}
}



int main()
{
	uint8_t i;

	DDRA = 0xFF;	//set PORTA to all outputs
	PORTA= 0xff;	//set all LEDs off at init

	DDRB = 0xF7;  	//set port B to all outputs (except PB3 - MISO)
	PORTB= 0x78;	//set select bits off, PWM low, pull up resistor on MISO

	DDRC = 0x3F;	//set pin 6 and 7 to inputs
	PORTC= 0xFF;	//with pull up resistors

	DDRD = 0x3F;	//set PORTD to all outputs
	PORTD= 0xC0;

	DDRE = 0xFF;	//set PORTE to all outputs

	SPI_init();

	//TIMER 0 SETUP
	ASSR  |= (1<<AS0);			//external clock 32,768Hz
	TIMSK |= (1<<TOIE0);			//enable overflow interrupt
	TCCR0 |= (1<<CS02) | (1<<CS00);		//normal mode, prescale by 128

	//TIMER 2 SETUP
	TIMSK |= (1<<TOIE2);			//enable overflow interrupt
	TCCR2 |= (1<<CS21) | (1<<CS20);		//normal mode, prescale by 256

	sei();

	while(1){     //do forever

		if (IS_SHOW_ALARM)
			split_digits(alarm[2], alarm[1]);		
		else
			split_digits(time[2], time[1]);

		for (i = 0; i < 5; i++) {	//Loop through each 7seg digit
			PORTA = SEGS[i];
			PORTB &= (i << PB4) & 0x70;
			_delay_ms(1);
			PORTB |= (1<<PB6) | (1<<PB5) | (1<<PB4);
		}
	} //while 
} //main
