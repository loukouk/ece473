#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "LCDDriver.h"
#include "music.h"
#include "_functions.h"

#define LED_DELAY 1
#define TEMPO 8 // 1/64th note = (256*128*TEMPO)/16Mhz

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
			music_off();
		}
		alarm_counter++;
	}
	else
		alarm_counter = 0;


	if (IS_ALARM_ARM) {
		if (IS_ALARM_SNOOZE) {
			if (snooze[0] == time[0] && snooze[1] == time[1] && snooze[2] == time[2]) {
				SET_ALARM_TRIGGER;
				music_on();

				if (lcd_mode != LCD_ALARM_TRIG) {
					LCD_Clr(); LCD_PutStr("RISE AND SHINE!!");
					lcd_mode = LCD_ALARM_TRIG;
				}
			}
		}
		else {
			if (alarm[0] == time[0] && alarm[1] == time[1] && alarm[2] == time[2]) {
				SET_ALARM_TRIGGER;
				music_on();

				if (lcd_mode != LCD_ALARM_TRIG) {
					LCD_Clr(); LCD_PutStr("RISE AND SHINE!!");
					lcd_mode = LCD_ALARM_TRIG;
				}
			}
		}
	}
}

ISR(TIMER2_OVF_vect) {
	static uint8_t int_count = 0;

	if (++int_count < 128)
		return;
	int_count = 0;

	static uint16_t volume[VOLUME_INDEX_MAX+1] = {0x0000,0x0100,0x200,0x300,0x400,0x500,0x600,0x700,0x800,0x900,0xA00,0xB00,0xC00,0xD00,0xE00,0x1000};
	static uint8_t brightness[BRIGHTNESS_INDEX_MAX+1] = {1,2,3,4,5,6,8,10,12,16,20,24,32,40,48,64,96,128,160,192,255};
	uint8_t i, dir[2];
	uint8_t data;
	static uint16_t tempo_count  = 0;

	read_adc();
	read_pushbuttons();

	if (IS_ALARM_ARM) {
		if (IS_SNOOZING && IS_ALARM_TRIGGER) {
			SET_ALARM_SNOOZE;
			CLEAR_ALARM_TRIGGER;
			music_off();
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
		music_off();
		if (lcd_mode != LCD_ALARM_OFF) {
			lcd_mode = LCD_ALARM_OFF;
			LCD_Clr(); LCD_PutStr("----ALARM OFF---");
		}
	}

	OCR3C = volume[volume_index];
	if (!(encoder_mode & 0x02))
		OCR2  = brightness[brightness_index];

	if(tempo_count >= TEMPO){
		//for note duration (64th notes) 
		beat++;
		tempo_count = 0;
	}
	tempo_count++;

	data = spi_send_read(mode);	//display their mode on bar graph + get input from encoders

	for (i = 0; i < 2; i++) {
		if (debounce_spi_buttons(i,(data >> 4) & 0x03))
			encoder_mode ^= 1 << i;
	}

	if (IS_SETTING) {
		adjust_alarm_time(data);
	}
	else {
		dir[0] = find_direction((data) & 0x03, 0);
		dir[1] = find_direction((data >> 2) & 0x03, 1);

		if (dir[0] == CW && volume_index < VOLUME_INDEX_MAX)
			volume_index++;
		else if (dir[0] == CCW && volume_index > 0)
			volume_index--;
		if (dir[1] == CW && brightness_index < BRIGHTNESS_INDEX_MAX)
			brightness_index++;
		else if (dir[1] == CCW && brightness_index > 0)
			brightness_index--;
	}
}



int main()
{
	uint8_t i, off_count=0;

	DDRA = 0xFF;	//set PORTA to all outputs
	PORTA= 0xff;	//set all LEDs off at init

	DDRB = 0xF7;  	//set port B to all outputs (except PB3 - MISO)
	PORTB= 0x78;	//set select bits off, PWM low, pull up resistor on MISO

	DDRC = 0xFF;	//set PORTC to all outputs
	PORTC= 0xFF;	//with pull up resistors

	DDRD = 0xFF;	//set PORTD to all outputs
	PORTD= 0xFF;

	DDRE = 0xFF;	//set PORTE to all outputs

	LCD_Init();
	LCD_Clr();

	init_globals();

	//TIMER 0 SETUP
	ASSR  |= (1<<AS0);			//external clock 32,768Hz
	TIMSK |= (1<<TOIE0);			//enable overflow interrupt
	TCCR0 |= (1<<CS02) | (1<<CS00);		//normal mode, prescale by 128

	//TIMER 2 SETUP
	TIMSK |= (1<<TOIE2);					//enable overflow interrupt
	TCCR2 |= (1<WGM21) | (1<<WGM20) | (1<<CS20) | (1<<COM21) | (1<<COM20);	//Fast pwm, no prescale!!, inverting OC2

	//TIMER 3 SETUP
	TCCR3A = (1<<COM3C1) | (0<<COM3C0) | (1<<WGM31) | (0<<WGM30);
	TCCR3B = (1<<WGM33) | (1<<WGM32) | (1<<CS30);
	ICR3   = 0x1000;
	OCR3C  = 0x0900;

	music_init();

	//ADC SETUP
	ADMUX = (1<<REFS0) | (1<<ADLAR) | (1<<MUX1);	//5V ref, left adjust, ADC2
	ADCSRA= (1<<ADEN) | 0x07;				//enable adc, 128 prescale
	CLEAR_ADC_INT_FLAG;
	BEGIN_ADC_CONVERSION;

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
			_delay_ms(LED_DELAY);
			PORTB |= (1<<PB6) | (1<<PB5) | (1<<PB4);
		}
	} //while 
} //main
