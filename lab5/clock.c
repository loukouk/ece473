#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

#include "_functions.h"
#include "LCDDriver.h"
#include "music.h"
#include "twi_master.h"
#include "lm73_functions.h"
#include "uart_functions.h"
#include "si4734.h"

#define LED_DELAY 500
#define TEMPO 8 // 1/64th note = (256*128*TEMPO)/16Mhz

volatile  int8_t time[3]		= {0x00,0x00,0x00};
volatile  int8_t alarm[3]		= {0x05,0x00,0x00};;
volatile  int8_t snooze[3]		= {0x00,0x00,0x00};;
volatile uint8_t SEGS[5]		= {0xFF,0xFF,0xFF,0xFF,0xFF};;
volatile uint8_t mode			= 0x00;
volatile uint8_t buttons		= 0x00;
volatile uint8_t encoder_mode		= 0x00;
volatile uint8_t alarm_mode		= 0x00;
volatile uint8_t volume_index		= VOLUME_INDEX_MAX/3;
volatile uint8_t brightness_index	= BRIGHTNESS_INDEX_MAX;
uint8_t ocr2_lock = 0;

volatile uint16_t current_fm_freq = 10790;
volatile uint8_t  current_volume;

volatile uint8_t  STC_interrupt 	= 0x00;
volatile uint8_t  UART_COUNT 		= 0xFF;
volatile uint16_t LCD_FREEZE_COUNTER 	= 0x00;
volatile uint8_t  lcd_freeze_mode 	= 0x00;
volatile uint8_t lcd_mode 		= 0x00;

char lcd_str_ln1[16]  = "                ";
char lcd_str_ln2[16]  = "                ";
char full_temp_str[16] = " hi          hi ";
char interior_temp[8] = "   ho   ";
char exterior_temp[8] = "   he   ";

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

	static uint8_t alarm_counter = 0;
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
			}
		}
		else {
			if (alarm[0] == time[0] && alarm[1] == time[1] && alarm[2] == time[2]) {
				SET_ALARM_TRIGGER;
				music_on();
			}
		}
	}
	request_temp();
}

ISR(TIMER2_OVF_vect) {
	static uint8_t int_count = 0;

	if (++int_count < 128)
		return;
	int_count = 0;

	static uint16_t volume[VOLUME_INDEX_MAX+1] = {0x0000,0x580,0x600,0x680,0x700,0x780,0x800,0x880,0x900,0x980,0xA00,0xA80,0xB00,0xB80,0xC00,0xD00,0x1000};
	static uint8_t brightness[BRIGHTNESS_INDEX_MAX+1] = {1,2,3,4,5,6,8,10,12,16,20,24,32,40,48,64,96,128,160,192,255};
	uint8_t i, dir[2];
	uint8_t data;
	static uint16_t tempo_count  = 0;

	read_pushbuttons();
	mode_set();
	
	if (IS_RADIO_MODE) {
		music_on();
	}
	else if (!IS_RADIO_MODE) {
		music_off();
	}

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
	}
	lcd_mode_set();

	OCR3C = volume[volume_index];
	if (encoder_mode & 0x02 && !ocr2_lock) {
		read_adc();
		OCR2  = brightness[brightness_index];
	}

	if(tempo_count >= TEMPO){
		//for note duration (64th notes) 
		beat++;
		tempo_count = 0;
	}
	tempo_count++;

	data = spi_send_read(mode);	//display mode on bar graph + get input from encoders

	for (i = 0; i < 2; i++) {
		if (debounce_spi_buttons(i,(data >> 4) & 0x03))
			encoder_mode ^= 1 << i;
	}

	if (IS_SETTING) {
		if (IS_SHOW_FREQ) {
			dir[0] = find_direction((data) & 0x03, 0);
			dir[1] = find_direction((data >> 2) & 0x03, 1);

			if ((dir[0] == CW || dir[1] == CW)) {
				if (current_fm_freq >= 10810)
					current_fm_freq = 8810;
				else
					current_fm_freq += 20;
			}
			if ((dir[1] == CCW || dir[0] == CCW)) {
				if (current_fm_freq <= 8810)
					current_fm_freq = 10810;
				else
					current_fm_freq -= 20;
			}
		}
		else
			adjust_alarm_time(data);
	}
	else {
		dir[0] = find_direction((data) & 0x03, 0);
		dir[1] = find_direction((data >> 2) & 0x03, 1);

		if (dir[0] == CW && volume_index < VOLUME_INDEX_MAX) {
			volume_index++;
			lcd_freeze_mode = LCD_VOLUME;
			LCD_FREEZE_COUNTER = LCD_FREEZE_TIME;
		}
		else if (dir[0] == CCW && volume_index > 0) {
			volume_index--;
			lcd_freeze_mode = LCD_VOLUME;
			LCD_FREEZE_COUNTER = LCD_FREEZE_TIME;
		}
		if (dir[1] == CW && brightness_index < BRIGHTNESS_INDEX_MAX)
			brightness_index++;
		else if (dir[1] == CCW && brightness_index > 0)
			brightness_index--;
	}

	static uint8_t char_count = 0;
	if (char_count < 16)
		LCD_PutChar(lcd_str_ln1[char_count]);
	else
		LCD_PutChar(lcd_str_ln2[char_count-16]);

	if (++char_count >= 32) {
		char_count = 0;
		LCD_MovCursorLn1();
		lcd_update();
	}
	else if (char_count == 16)
		LCD_MovCursorLn2();

}

ISR(USART0_RX_vect)
{
	DDRF = 0x01;
	PORTF^=0x01;
	if (UART_COUNT >= 8)
		return;

	char data = UDR0;
	if (data == 0x00)
		return;

	exterior_temp[UART_COUNT] = data;
	UART_COUNT++;
	if (UART_COUNT >= 8) {
		memcpy(full_temp_str, exterior_temp, 8);
		if (interior_temp[1] != ' ')
			memcpy(full_temp_str + 8, interior_temp, 8);
		UART_COUNT = 0;
	}
}

ISR(INT7_vect){
	STC_interrupt = TRUE;
	PORTB |= 1<<5;
	_delay_ms(5000);
	PORTB ^= 0x80;
}

int main()
{
	uint16_t temp_cnt = 0;
	uint8_t i;

	DDRA = 0xFF;	//set PORTA to all outputs
	PORTA= 0xff;	//set all LEDs off at init

	DDRB = 0xF7;  	//set port B to all outputs (except PB3 - MISO)
	PORTB= 0x78;	//set select bits off, PWM low, pull up resistor on MISO

	DDRC = 0xFF;	//set PORTC to all outputs
	PORTC= 0x00;	//with pull up resistors

	DDRD = 0xFF;	//set PORTD to all outputs
	PORTD= 0xFF;

	DDRE = 0xFF;	//set PORTE to all outputs

	LCD_Init();
	LCD_Clr();
	uart_init();

	//TIMER 0 SETUP
	ASSR  |= (1<<AS0);			//external clock 32,768Hz
	TIMSK |= (1<<TOIE0);			//enable overflow interrupt
	TCCR0 |= (1<<CS02) | (1<<CS00);		//normal mode, prescale by 128

	//TIMER 2 SETUP
	TIMSK |= (1<<TOIE2);					//enable overflow interrupt
	TCCR2 |= (1<WGM21) | (1<<WGM20) | (1<<CS20) | (1<<COM21) | (1<<COM20);	//Fast pwm, no prescale!!, inverting OC2
	OCR2 = 255;

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

	//EXT INT SETUP
//	EICRB |= (1<<ISC70) | (1<<ISC71);
//	EIMSK |= (1<<INT7);

/*	DDRB = 0xFF;
	PORTB = 0x00;

	//RADIO SETUP
	DDRE  |= 0xFF; //Port E bit 2 is active high reset for radio 
	PORTE |= 0x04; //radio reset is on at powerup (active high)

	PORTB |= 0x01;

	//hardware reset of Si4734
	PORTE &= ~(1<<PE7); //int2 initially low to sense TWI mode
	DDRE  |= 0x80;      //turn on Port E bit 7 to drive it low
	PORTE &=  ~(1<<PE2); //hardware reset Si4734 
	_delay_ms(1000);     //hold for 200us, 100us by spec         
	PORTE |= (1<<PE2); //release reset 
	_delay_us(50);      //5us required because of my slow I2C translators I suspect
	//Si code in "low" has 30us delay...no explaination
	DDRE  &= ~(0x80);   //now Port E bit 7 becomes input from the radio interrupt
	PORTE |= 0x80;
*/
	init_twi();
//	PORTB |= 0x02;
	sei();
	lm73_init();
//	request_temp();
/*
	fm_pwr_up(); //powerup the radio as appropriate
	PORTB |= 0x04;
	current_fm_freq = 10790; //arg2, arg3: 99.9Mhz, 200khz steps
	_delay_ms(1000);
	fm_tune_freq(); //tune radio to frequency in current_fm_freq
	PORTB |= 0x08;
*/
	while(1){     //do forever

		if (temp_cnt > 2000) {
			read_int_temp(interior_temp);
			temp_cnt = 0;
		}
		else
			temp_cnt++;

		if (IS_ALARM_TRIGGER)
			print_get_up();
		else {
			if (IS_SHOW_ALARM)
				decode_time(alarm[2], alarm[1]);
			else if (IS_SHOW_FREQ)
				decode_freq(current_fm_freq);
			else
				decode_time(time[2], time[1]);
		}

		for (i = 0; i < 5; i++) {	//Loop through each 7seg digit
			PORTA = SEGS[i];
			PORTB &= (i << PB4) & 0x70;
			_delay_us(LED_DELAY);
			PORTB |= (1<<PB6) | (1<<PB5) | (1<<PB4);
		}

	} //while 
} //main
