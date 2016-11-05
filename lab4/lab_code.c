#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "LCDDriver.h"
#include "_functions.h"

ISR(TIMER0_OVF_vect)
{
	static uint8_t alarm_counter = 0;

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
	if (IS_ALARM_TRIGGER) {
		if (alarm_counter > 60) {
			CLEAR_ALARM_TRIGGER;
			if (IS_ALARM_SNOOZE) {
				CLEAR_ALARM_SNOOZE;
			}
		}
		alarm_counter++;
	}
	else
		alarm_counter = 0;


	if (IS_ALARM_ARM) {
		if (IS_ALARM_SNOOZE) {
			if (snooze[0] == time[0] && snooze[1] == time[1] && snooze[2] == time[2]) {
				SET_ALARM_TRIGGER;

				if (lcd_mode != LCD_ALARM_TRIG) {
					LCD_Clr(); LCD_PutStr("RISE AND SHINE!!");
					lcd_mode = LCD_ALARM_TRIG;
				}
			}
		}
		else {
			if (alarm[0] == time[0] && alarm[1] == time[1] && alarm[2] == time[2]) {
				SET_ALARM_TRIGGER;

				if (lcd_mode != LCD_ALARM_TRIG) {
					LCD_Clr(); LCD_PutStr("RISE AND SHINE!!");
					lcd_mode = LCD_ALARM_TRIG;
				}
			}
		}
	}
}

ISR(TIMER2_OVF_vect) {
	uint8_t i, dir[2];
	uint8_t data;

	read_pushbuttons();

	if (IS_ALARM_ARM) {
		if (IS_SNOOZING && IS_ALARM_TRIGGER) {
			SET_ALARM_SNOOZE;
			CLEAR_ALARM_TRIGGER;
			snooze[0] = time[0] + SNOOZE_SECS;
			snooze[1] = time[1] + SNOOZE_MINS;
			snooze[2] = time[2];

			if (snooze[0] >= 60) {
				snooze[0] -= 60;
				snooze[1]++;
			}
			if (snooze[1] >= 60) {
				snooze[1] -= 60;
				snooze[2]++;
			}
			if (snooze[2] >= 24) {
				snooze[0] -= 24;
			}
		}
		else if (!IS_ALARM_TRIGGER){
			if (lcd_mode != LCD_ALARM_ON) {
				lcd_mode = LCD_ALARM_ON;
				LCD_Clr(); LCD_PutStr("----ALARM ON----");
			}
		}
	}
	else {
		CLEAR_ALARM_TRIGGER;
		if (lcd_mode != LCD_ALARM_OFF) {
			lcd_mode = LCD_ALARM_OFF;
			LCD_Clr(); LCD_PutStr("----ALARM OFF---");
		}
	}

	data = spi_send_read(mode);	//display their mode on bar graph + get input from encoders

	for (i = 0; i < 2; i++) {
		if (debounce_spi_buttons(i,(data >> 4) & 0x03))
			encoder_mode ^= 1 << i;
	}

	if (IS_SETTING) {
		//for each encoder, determine which direction it is being turned
		for (i = 0; i < 2; i++) {
			dir[i] = find_direction((data >> (i*2)) & 0x03, i);

			//increment count if encoders are being turned clockwise
			if (dir[i] == CW) {
				if (IS_SHOW_ALARM)
					alarm[2-i]++;
				else
					time[2-i]++;
			}
			//decrement count if encoders are being turned counter clockwise
			else if (dir[i] == CCW) {
				if (IS_SHOW_ALARM)
					alarm[2-i]--;
				else 
					time[2-i]--;
			}
		}

		if (IS_SHOW_ALARM) {
			if (alarm[1] >= 60)
				alarm[1] -= 60;
			else if (alarm[1] < 0)
				alarm[1] += 60;
			if (alarm[2] >= 24)
				alarm[2] -= 24;
			else if (alarm[2] < 0)
				alarm[2] += 24;
		}
		else {
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

	LCD_Init();
	LCD_Clr();

	init_globals();

	//TIMER 0 SETUP
	ASSR  |= (1<<AS0);			//external clock 32,768Hz
	TIMSK |= (1<<TOIE0);			//enable overflow interrupt
	TCCR0 |= (1<<CS02) | (1<<CS00);		//normal mode, prescale by 128

	//TIMER 2 SETUP
	TIMSK |= (1<<TOIE2);			//enable overflow interrupt
	TCCR2 |= (0<<CS22) | (1<<CS21) | (1<<CS20);		//normal mode, prescale by 256

	sei();


	while(1){     //do forever

		if (IS_ALARM_TRIGGER)
			print_get_up();
		else {
			if (IS_SHOW_ALARM)
				decode_time(alarm[2], alarm[1]);		
			else
				decode_time(time[2], time[1]);
		}

		for (i = 0; i < 5; i++) {	//Loop through each 7seg digit
			PORTA = SEGS[i];
			PORTB &= (i << PB4) & 0x70;
			_delay_ms(1);
			PORTB |= (1<<PB6) | (1<<PB5) | (1<<PB4);
		}
	} //while 
} //main
