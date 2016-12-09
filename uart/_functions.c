#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>

#include "_functions.h"
#include "twi_master.h"
#include "lm73_functions.h"
#include "uart_functions.h"
#include "si4734.h"
#include "LCDDriver.h"
#include "music.h"

uint16_t lm73_temp;
extern volatile uint8_t song;
extern volatile uint8_t mode;
extern volatile uint8_t buttons;
extern volatile uint8_t SEGS[5];
extern volatile uint8_t encoder_mode;
extern volatile uint8_t time[3];
extern volatile uint8_t alarm[3];
extern volatile uint8_t lcd_mode;
extern volatile uint8_t alarm_mode;
extern volatile uint16_t LCD_FREEZE_COUNTER;
extern volatile uint8_t  lcd_freeze_mode;
extern volatile uint8_t brightness_index;
extern volatile uint8_t volume_index;
extern uint8_t ocr2_lock;
extern char lcd_str_ln1[16];
extern char lcd_str_ln2[16];
extern char interior_temp[8];
extern char full_temp_str[16];
extern uint8_t UART_COUNT;

uint8_t IS_RADIO_ON = 0;

//Static string definitions
char alarm_ringing1[16] = "RISE AND SHINE!!";
char alarm_ringing2[16] = "YOU SLEPT ENOUGH";
char alarm_armed[16]    = "Alarm set: xx:xx";
char alarm_off[16]      = "Relax, alarm off";
char showing_freq[16]   = "Showing channel ";
char showing_alarm[16]  = "Showing alarm   ";
char setting_time[16]   = "Setting time    ";
char setting_freq[16]   = "Setting channel ";
char setting_alarm[16]  = "Setting alarm   ";
char setting_song[16]   = "Tune to play:   ";
char song0_name[16]     = "   Beaver Fight!";
char song1_name[16]     = "    Tetris Theme";
char song2_name[16]     = "Super Mario Bros";
char song3_name[16]     = " Legend of Zelda";
char radio_str[16]	= "   Radio of Doom";
char volume_str[16]     = "    Volume xx   ";
char playing_radio[16]	= "Playing music!  ";

//*******************************************************************************
//                            debounce_switch                                  
// Adapted from Ganssel's "Guide to Debouncing"            
// Checks the state of pushbutton S0 It shifts in ones till the button is pushed. 
// Function returns a 1 only once per debounced button push so a debounce and toggle 
// function can be implemented at the same time.  Expects active low pushbutton on 
// Port D bit zero.  Debounce time is determined by external loop delay times 12. 
//*******************************************************************************
int8_t debounce_switch(uint8_t pin) {
	static uint16_t state[8] = {0,0,0,0,0,0,0,0}; //holds present states
	state[pin] = (state[pin] << 1) | (bit_is_set(PINA, pin)) | 0xE000;	//count 12 "presses"
	if (state[pin] == 0xF000) return 1;
	return 0;
}
int8_t debounce_spi_buttons(uint8_t enc_pin, uint8_t enc_data) {
	static uint16_t enc_state[2] = {0,0}; //holds present enc_states
	enc_state[enc_pin] = (enc_state[enc_pin] << 1) | (((enc_data >> enc_pin) & 0x01)) | 0xE000;	//count 12 "presses"i
	if (enc_state[enc_pin] == 0xF000) return 1;
	return 0;
}

uint8_t decode_digit(int8_t digit)
{
	switch(digit){
		case -1:	return 0b11111111;	// turn off digit
		case 0: 	return 0b11000000;	// print 0
		case 1: 	return 0b11111001;	// print 1
		case 2: 	return 0b10100100;	// print 2
		case 3: 	return 0b10110000;	// print 3
		case 4: 	return 0b10011001;	// print 4
		case 5: 	return 0b10010010;	// print 5
		case 6: 	return 0b10000010;	// print 6
		case 7: 	return 0b11111000;	// print 7
		case 8: 	return 0b10000000;	// print 8
		case 9:		return 0b10011000;	// print 9
		case 10: 	return 0b10001000;	// print A
		case 11: 	return 0b10000011;	// print b
		case 12: 	return 0b11000110;	// print C
		case 13: 	return 0b10100001;	// print d
		case 14: 	return 0b10000110;	// print E
		case 15:	return 0b10001110;	// print F
		default:	return 0b10111111;	// print dash when there is an unexpected value
	}
}

uint8_t spi_send_read(uint8_t data)
{
	PORTE &= 0xBF;				//port E bit 6, deassert to logic low
	PORTE |= 0x40;				//port E bit 6, assert active high
	SPDR = data;                    	//send data byte
	while (bit_is_clear(SPSR,SPIF)) {}	//wait till data is sent out
	PORTB &= 0xFE;                  	//port B bit 0, assert active low
	PORTB |= 0x01;				//port B bit 0, deassert to logic high

	return SPDR;				//return the data read from the encoders
}

void request_temp() {
	UART_COUNT = 0;
	if (F_NOT_C)
		uart_putc('F');
	else
		uart_putc('C');
	_delay_us(50);
}

uint8_t find_direction (uint8_t state, uint8_t i)
{
/*
	static uint8_t sw_table[] = {0,1,2,0,2,0,0,1,1,0,0,2,0,2,1,0};
	static uint8_t prev_state[2] = {0,0};
	uint8_t index = 0;

	index = (prev_state[i] << 2) | state;
	prev_state[i] = state;
	return sw_table[index];
*/
	static uint8_t prev_a[2] = {-1,-1};	//holds last state of b
	static uint8_t prev_b[2] = {-1,-1};	//holds last state of b
	uint8_t a = state & 1;			//current a
	uint8_t b = (state>>1) & 1;		//current b
	uint8_t direction = 0;			//holds resulting direction


	if (!(a && b))
		goto FIND_DIRECTION_EXIT;

	//Determine direction using A tracking method
	if (!(a == -1 || b == -1)) {
		if (prev_a[i] == a){
			if ((a == 1) && (prev_b[i] < b)) direction = CW;
			if ((a == 1) && (prev_b[i] > b)) direction = CCW;
			if ((a == 0) && (prev_b[i] < b)) direction = CCW;
			if ((a == 0) && (prev_b[i] > b)) direction = CW;
		}
		else {
			if ((prev_a[i] < a) && ((prev_b[i] | b) == 0)) direction = CW;
			if ((prev_a[i] < a) && ((prev_b[i] | b) == 1)) direction = CCW;
			if ((prev_a[i] > a) && ((prev_b[i] | b) == 0)) direction = CCW;
			if ((prev_a[i] > a) && ((prev_b[i] | b) == 1)) direction = CW;
		}
	}
	
FIND_DIRECTION_EXIT:
	//set prev variables for next pass
	prev_a[i] = a;
	prev_b[i] = b;

	return direction;
}

void mode_set()
{
	if (IS_ALARM_TRIGGER && buttons) {
		CLEAR_ALARM_TRIGGER;
		if (!IS_RADIO_MODE)
			music_off();

		if (buttons & (~(1<<3)))
			mode |= 1<<7;
		else
			mode &= ~(1<<7);
	}
	if ((buttons >> 1) & 0x01 && !IS_RADIO_MODE) {
		mode &= ~(1<<2);
		mode ^= 1<<1;
	}
	else if ((buttons >> 2) & 0x01 && !IS_RADIO_MODE) {
		mode &= ~(1<<1);
		mode ^= 1<<2;
	}
	if ((buttons >> 0) & 0x01 && !IS_RADIO_MODE) {
		mode ^= 0x01;
	}
	if ((buttons >> 3) & 0x01) {
		mode ^= 1<<3;
	}
	if ((buttons >> 4) & 0x01) {
		mode ^= 1<<4;
	}
	if ((buttons >> 5) & 0x01) {
		mode ^= 1<<5;
	}
	if ((buttons >> 6) & 0x01) {
		mode &= ~((1<<0) | (1<<1) | (1<<2));
		mode ^= 1<<6;
	}
	if ((buttons >> 7) & 0x01) {
		music_off();

		song++;
		if (song > 4)
			song = 0;

		music_on();

		lcd_freeze_mode = LCD_SET_SONG;
		LCD_FREEZE_COUNTER = LCD_FREEZE_TIME;
	}

}

void lcd_mode_set()
{
	if (IS_ALARM_TRIGGER)
		lcd_mode = LCD_ALARM_TRIG;
	else if (IS_RADIO_MODE)
		lcd_mode = LCD_PLAYING_RADIO;
	else if (IS_SETTING) {
		if (IS_SHOW_ALARM)
			lcd_mode = LCD_SET_ALARM;
		else if (IS_SHOW_FREQ)
			lcd_mode = LCD_SET_FREQ;
		else
			lcd_mode = LCD_SET_TIME;
	}
	else if (IS_SHOW_ALARM)
		lcd_mode = LCD_SHOW_ALARM;
	else if (IS_SHOW_FREQ)
		lcd_mode = LCD_SHOW_FREQ;
	else if (IS_ALARM_ARM)
		lcd_mode = LCD_ALARM_ON;
	else
		lcd_mode = LCD_ALARM_OFF;	
}

void read_pushbuttons ()
{
	uint8_t i, ports_data[2];

	ports_data[0] = PORTA;		//save PORTA data
	ports_data[1] = PORTB & 0x70;	//save PORTA data
	PORTA = 0xFF;			//set all pull up resistors on PORTA
	DDRA  = 0x00;			//set PORTA to all inputs
	PORTB |= 0x70;
	PORTB &= (5 << 4) & 0x70;	//set select bits to take input from pushbuttons
	asm("nop");
	asm("nop");

	buttons = 0x00;
	for (i = 0; i < 8; i++) {	//take input with debouncing
		if (debounce_switch(i)) {
			buttons |= 1 << i;
		}
	}

	asm("nop");
	PORTB&= 0x8F;
	PORTB|= ports_data[1];		//restore PORTB data
	DDRA  = 0xFF;			//set PORTA back to outputs
	PORTA = ports_data[0];		//restore PORTA data
}

void print_get_up ()
{
	static uint8_t display_count;

	if (display_count < 80) {
		ocr2_lock = 0;
		SEGS[0] = 0b10000111;	//t
		SEGS[1] = 0b10000110;	//E
		SEGS[2] = 0b11111111;	//off
		SEGS[3] = 0b10010000;	//g
		SEGS[4] = 0xFF;		//off
	}
	else if (display_count < 128 || display_count >= 208) {
		OCR2 = 0x00;
		ocr2_lock = 1;
		lcd_freeze_mode = LCD_CLEAR;
		LCD_FREEZE_COUNTER = 1;
	}
	else {
		ocr2_lock = 0;
		SEGS[0] = 0b11111111;	//off
		SEGS[1] = 0b10001100;	//P
		SEGS[2] = 0b11111111;	//off
		SEGS[3] = 0b11000001;	//U
		SEGS[4] = 0b11111111;	//off
	}
	display_count++;
}

void decode_time(uint8_t hours, uint8_t minutes)
{
	if (IS_AM_PM && hours > 12) {
		hours -= 12;
		SEGS[2] &= ~(0x04);
	}
	else
		SEGS[2] |= 0x04;

	//breaks up time value into its separate digits
	SEGS[1] = minutes / 10;
	SEGS[0] = minutes % 10;

	SEGS[4] = hours / 10;
	SEGS[3] = hours % 10;

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
}

void decode_freq(uint16_t freq)
{
	uint16_t segs[4];
	freq /= 10;

	segs[3] = freq/1000;
	freq -= segs[3]*1000;
	segs[2] = freq / 100;
	freq -= segs[3]*100;
	segs[1] = freq / 10;
	freq -= segs[3]*10;
	segs[0] = freq;


	//removes all leading zeroes for a cleaner output
	if (segs[3] == 0) 
		segs[3] = -1;

	//decodes each digit into a value for the 7seg display
	SEGS[4] = decode_digit(segs[3]);
	SEGS[3] = decode_digit(segs[2]);
	SEGS[2] = 0xFF;
	SEGS[1] = decode_digit(segs[1]);
	SEGS[0] = decode_digit(segs[0]);

	SEGS[1] &= ~(1<<7);
}

void adjust_alarm_time(uint8_t encoder_data)
{
	uint8_t i, dir[2];

	//for each encoder, determine which direction it is being turned
	for (i = 0; i < 2; i++) {
		dir[i] = find_direction((encoder_data >> (i*2)) & 0x03, i);

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

void read_adc()
{
	uint8_t value = ADCH;
	CLEAR_ADC_INT_FLAG;
	BEGIN_ADC_CONVERSION;

	if (value < ADC_MIN)
		brightness_index = BRIGHTNESS_INDEX_MAX;
	else if (value >= ADC_MAX)
		brightness_index = 0;
	else
		brightness_index = BRIGHTNESS_INDEX_MAX - ((value - ADC_MIN) / ADC_DIV);
}

void read_int_temp (char temp_str[])
{
	uint8_t buf[2];
	memset(temp_str, ' ', 8);

	twi_start_rd(LM73_ADDRESS, buf, 2);
	_delay_ms(2);

	lm73_temp = (buf[0] << 8) | buf[1];
	lm73_temp_convert(temp_str, lm73_temp, F_NOT_C);
}

void int_to_str_2( uint8_t val, char str[])
{
	if (val > 99)
		return;

	itoa(val, str, 10);

	if (val < 10) {
		str[1] = str[0];
		str[0] = '0';
	}
}

void lcd_update()
{
	static uint8_t mode = 0xFF;
	static uint8_t counter = 0;
	char temp_str[2];

	if (LCD_FREEZE_COUNTER > 0) {
		LCD_FREEZE_COUNTER--;
		mode = lcd_freeze_mode;	
	}
	else
		mode = lcd_mode;

	switch(mode) {
		case LCD_CLEAR:
			LCD_Clr();
		case LCD_ALARM_OFF:
			memcpy(lcd_str_ln1, alarm_off, 16);
			memcpy(lcd_str_ln2, full_temp_str, 16);
			break; 
		case LCD_ALARM_ON:
			memcpy(lcd_str_ln1, alarm_armed, 16);
			int_to_str_2(alarm[2], temp_str);
			lcd_str_ln1[11] = temp_str[0];
			lcd_str_ln1[12] = temp_str[1];
			int_to_str_2(alarm[1], temp_str);
			lcd_str_ln1[14] = temp_str[0];
			lcd_str_ln1[15] = temp_str[1];
			memcpy(lcd_str_ln2, full_temp_str, 16);
			break; 
		case LCD_ALARM_TRIG:
			if (counter < 160) {
				memcpy(lcd_str_ln1, alarm_ringing1, 16);
				memcpy(lcd_str_ln2, alarm_ringing2, 16);
			}
			else {
				memset(lcd_str_ln1, ' ', 16);
				memset(lcd_str_ln2, ' ', 16);
			}
			counter++;
			break;
		case LCD_SET_TIME:
			memcpy(lcd_str_ln1, setting_time, 16);
			memcpy(lcd_str_ln2, full_temp_str, 16);
			break;
		case LCD_SET_ALARM:
			memcpy(lcd_str_ln1, setting_alarm, 16);
			memcpy(lcd_str_ln2, full_temp_str, 16);
			break;
		case LCD_SET_FREQ:
			memcpy(lcd_str_ln1, setting_freq, 16);
			memcpy(lcd_str_ln2, full_temp_str, 16);
			break;
		case LCD_SET_SONG:
			memcpy(lcd_str_ln1, setting_song, 16);
			switch(song) {
				case 0:
					memcpy(lcd_str_ln2, song0_name, 16);
					break;
				case 1:
					memcpy(lcd_str_ln2, song1_name, 16);
					break;
				case 2:
					memcpy(lcd_str_ln2, song2_name, 16);
					break;
				case 3:
					memcpy(lcd_str_ln2, song3_name, 16);
					break;
				case 4:
					memcpy(lcd_str_ln2, radio_str, 16);
					break;
			}
			break;
		case LCD_VOLUME:
			memcpy(lcd_str_ln1, volume_str, 16);
			int_to_str_2(volume_index, temp_str);
			lcd_str_ln1[11] = temp_str[0];
			lcd_str_ln1[12] = temp_str[1];
			memset(lcd_str_ln2, ' ', 16);
			memset(lcd_str_ln2, '-', volume_index);
			break;
		case LCD_SHOW_ALARM:
			memcpy(lcd_str_ln1, showing_alarm, 16);
			memcpy(lcd_str_ln2, full_temp_str, 16);
			break;
		case LCD_SHOW_FREQ:
			memcpy(lcd_str_ln1, showing_freq, 16);
			memcpy(lcd_str_ln2, full_temp_str, 16);
			break;
		case LCD_PLAYING_RADIO:
			memcpy(lcd_str_ln1, playing_radio, 16);
			switch(song) {
				case 0:
					memcpy(lcd_str_ln2, song0_name, 16);
					break;
				case 1:
					memcpy(lcd_str_ln2, song1_name, 16);
					break;
				case 2:
					memcpy(lcd_str_ln2, song2_name, 16);
					break;
				case 3:
					memcpy(lcd_str_ln2, song3_name, 16);
					break;
				case 4:
					memcpy(lcd_str_ln2, radio_str, 16);
					break;
			}
			break;
			
		default:
			break;
	}
}

void init_radio()
{
	//EXT INT SETUP
	EICRB |= (1<<ISC70) | (0<<ISC71);
	EIMSK |= (1<<INT7);
}

void reset_radio()
{
	DDRE  |= 0x04; //Port E bit 2 is active high reset for radio 
	PORTE |= 0x04; //radio reset is on at powerup (active high)

	//hardware reset of Si4734
	PORTE &= ~(1<<PE7); //int2 initially low to sense TWI mode
	DDRE  |= 0x80;      //turn on Port E bit 7 to drive it low
	PORTE |=  1<<PE2; //hardware reset Si4734 
	_delay_us(200);     //hold for 200us, 100us by spec         
	PORTE &= ~(1<<PE2); //release reset 
	_delay_us(30);      //5us required because of my slow I2C translators I suspect
			//Si code in "low" has 30us delay...no explaination
	DDRE  &= ~(0x80);   //now Port E bit 7 becomes input from the radio interrupt
}

void play_radio()
{
	static uint16_t prev_freq = 0;

	if (!IS_RADIO_ON) {
		reset_radio();
		fm_pwr_up(); //powerup the radio as appropriate
		fm_tune_freq(); //tune radio to frequency in current_fm_freq
	}
	else if (prev_freq != current_fm_freq)
		fm_tune_freq();

	prev_freq = current_fm_freq;
	IS_RADIO_ON = 1;
}

void stop_radio()
{
	if (!IS_RADIO_ON)
		return;

	radio_pwr_dwn();
	IS_RADIO_ON = 0;
}
