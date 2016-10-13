// lab3.c
// template: Roger Traylor
// modified: Luke  Morrison

#define CW 1
#define CCW 2
#define READY 1
#define TRIG 2
#define NOT_SET 3

//#define F_CPU 16000000

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "LCDDriver.h"
#include "twi_master.h"

int clk_time[3] = {0,0,0};				//holds the time value
int alarm[3] = {0,0,0};					//holds the time of the alarm
volatile static uint8_t alarm_state = NOT_SET;	//tells whether the alarm is triggered, armed or idle
volatile uint8_t mode = 0;	//holds the active mode bits
			//bit 0: high -> Set time
			//bit 1: high -> Set alarm
			//bit 2: Arm alarm, 1 -> on, 0 -> off
			//bit 3: high -> 12 hour, low -> 24 hour
			//bit 4:
			//bit 5:
			//bit 6: 
			//bit 7: AM/PM indicator, 0 -> AM, 1 -> PM

uint8_t spi_send_read(uint8_t data);
uint8_t debounce_switches();
uint8_t print_digit(int val);
void find_digits(int *val, int *dig);
uint8_t find_direct(uint8_t encoder_state, uint8_t i);
void change_time (int *val, uint8_t dir, int *count, uint8_t max);
void setoff_alarm();
void clear_alarm();
void ready_alarm();
void SPI_init();

ISR(ADC_vect)
{
	OCR2 = ADCH;	//whenever analog value is converted to digital
					//write to PB7 through PWM, losing last 2 bits of precision
}


ISR(TIMER1_COMP_vect)
{
	if (alarm_state == TRIG)	//when alarm is triggered
		PORTD ^= 1<<3;			//On compare, toggle PD4 to create waveform of uniform duty cycle
}


ISR(TIMER0_OVF_vect)	//Happens every second
{
	clk_time[0]++;		//add one second to the time

	if (clk_time[0] >= 60) {		//60 secs in a minute
		clk_time[0] -= 60;
		clk_time[1]++;

		if (clk_time[1] >= 60) {	//60 minutes in an hour
			clk_time[1] -= 60;
			clk_time[2]++;
			
			if (clk_time[2] >= 24)	//24 hours in a day
				clk_time[2] -= 24;
		}
	}
}

/*
void toggle_PE3 ()
{
	if (alarm_state == TRIG)	//Only when the alarm is triggered, play the sound
		PORTE ^= 1<<3;
	else						//Otherwise set volume to zero
		PORTE &= ~(1<<3);
}

ISR(TIMER3_COMP_vect)
{
	toggle_PE3();		//Toggle on compare
}

ISR(TIMER3_OVF_vect)
{
	toggle_PE3();		//and toggle on overflow to create PWM waveform with
						//a specific duty cycle
}
*/

ISR(TIMER2_OVF_vect)
{
	static uint8_t curr_direction[2];		//holds direction of the encoder when last checked
	static int count[2] = {0,0};
	static uint8_t artificial_prescale = 0;
	uint8_t buttons = 0;		//holds the pressed pushbuttons
	uint8_t SPI;
	
	if (artificial_prescale < 128)		//Since there is no prescale, we emulate one by
		artificial_prescale++;			//looping to 128 so that our pushbuttons and 
	else{								//encoders are not continually probed
		artificial_prescale = 0;
		
		PORTB |= 0x70; 	//enable the push button tristate buffer 
		PORTB &= (0x8F | (5<<4));
		DDRA  = 0x00;   //set port A to all inputs
		PORTA = 0xFF;   //set pin A pull up resistors

		//take input of two of the pushbuttons
		buttons = debounce_switches();
		PORTB &= 0x70; 	//disable the push button tristate buffer
		
		mode ^= (0x0F & buttons);		//toggle mode bits if some of the first 4 buttons are pushed

		//Simultaneously read the encoder bits and write the right data to the bar graph display
		SPI = spi_send_read(mode);
		//determine direction pots were turned from encoder data
		curr_direction[0] = find_direct(SPI, 0);
		curr_direction[1] = find_direct(SPI >> (2), 1);
		
		//if the 'set time' bit is 1 in mode, when encoders are turned
		//change the minutes and the hours of the time accordingly
		if ((mode&0x03) == 0x1) {
			change_time(&(clk_time[1]), curr_direction[0], &(count[0]), 60);
			change_time(&(clk_time[2]), curr_direction[1], &(count[1]), 24);
		}
		//if the 'set alarm' bit in mode is set, when encoders are turned
		//change the alarm time accordingly
		else if ((mode&0x03) == 0x2) {
			change_time(&(alarm[1]), curr_direction[0], &(count[0]), 60);
			change_time(&(alarm[2]), curr_direction[1], &(count[1]), 24);
		}
		
		//if alarm is armed, check that it has not been unarmed nor triggered
		if (alarm_state == READY) {
			if (!((mode >> 2) & 1)) {
				clear_alarm();
				alarm_state = NOT_SET;
			}
			else if (clk_time[1] == alarm[1] && clk_time[2] == alarm[2]) {
				setoff_alarm();
				alarm_state = TRIG;
			}
		}
		//if the alarm is triggered check if the snooze button has been pressed
		//or if the alarm has been unarmed
		else if (alarm_state == TRIG) {
			if ((buttons >> 7) & 1) {
				alarm[1] += 1;
				ready_alarm();
				alarm_state = READY;
			}
			if (!((mode >> 2) & 1)) {
				clear_alarm();
				alarm_state = NOT_SET;
			}
		}
		//if the alarm is unarmed, check if it has been armed
		else {
			if ((mode >> 2) & 1) {
				ready_alarm();
				alarm_state = READY;
			}
		}	

		//Make sure that PORTA is outputs showing all 1's so that no garbage
		//gets written to LED display
		PORTB |= 0x70;
		DDRA  = 0xFF;   //set port 1A to all outputs
		PORTA = 0xFF;   //set port A to all ones  (off, active low)
	}
}

//print "ALARM READY" on LCD display
void ready_alarm()
{
	LCD_SPIInit();
	LCD_Clr();
	LCD_PutStr("ALARM READY     ");
	SPI_init();
}

//print "ALARM TRIGGERED!" on LCD display
void setoff_alarm()
{
	LCD_SPIInit();
	LCD_Clr();
	LCD_PutStr("ALARM TRIGGERED!");
	SPI_init();
}

//Clear LCD display
void clear_alarm()
{
	LCD_SPIInit();
	LCD_Clr();
	SPI_init();
}

void change_time (int *val, uint8_t dir, int *count, uint8_t max)
{
	if (dir == CW)			//if direction is CW, increase the count
		(*count)++;
	else if (dir == CCW)	//if direction is CCW, decrease the count
		(*count)--;
	if ((*count) >= 4) {	//if the encoder count is greater than 4
		(*val)++;   		//increase the value (hours or minutes)  
		(*count) -= 4;		//and reset the count
	}
	else if ((*count) <= -4) {//if the encoder count is greater than -4	
		(*val)--;			//decrease the value (hours or minutes) 
		(*count) += 4;		//and reset the count
	}
	if ((*val) >= max)		//if the value is greater than its maximum (e.g. 24 hours or 60 minutes)
		(*val) -= max;		//subtract by its maximum
	else if ((*val) < 0)	//if the value is smaller than zero 
		(*val) += max;		//add its max
}

uint8_t spi_send_read(uint8_t data)
{
	PORTD &= 0xFB;                      //port D bit 2, assert active low
	PORTE |= 0x40;						//port E bit 6, assert active high
	SPDR = data;                        //send data byte
	while (bit_is_clear(SPSR,SPIF)) {}  //wait till data is sent out
	PORTD |= 0x04;                      //port D bit 2, deassert to logic high
	PORTE &= 0xBF;						//port E bit 6, deassert to logic low

	return SPDR;						//return the data read from the encoders
}

uint8_t find_direct (uint8_t encoder_state, uint8_t i)
{
	static uint8_t prev_a[2] = {-1,-1};		//holds last state of b
	static uint8_t prev_b[2] = {-1,-1};		//holds last state of b
	uint8_t a = encoder_state & 1;			//current a
	uint8_t b = (encoder_state>>1) & 1;		//current b
	uint8_t direction = 0;					//holds resulting direction
	
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
	
	//set prev variables for next pass
	prev_a[i] = a;
	prev_b[i] = b;
	
	return direction;
}

uint8_t debounce_switches()
{
  static uint16_t state[8] = {0,0,0,0,0,0,0,0}; //holds states for each push button
  uint8_t debounced_PIN = 0;					//holds which push button is true after debouncing
  
  for (int i = 0; i < 8; i++) {
	state[i] = (state[i] << 1) | (! bit_is_clear(PINA, i)) | 0xF000;	//Debounce. (removed 1 cycle)
	if (state[i] == 0xF800)			//if the switch has been pressed for long enough
		debounced_PIN |= 1 << i;	//add 2^(pin no.) to the result
  }
  return debounced_PIN;
}

uint8_t print_digit(int val)
{
	switch (val) {
		case 0:
			return 0b11000000;	//seven segment display: 0
		case 1:
			return 0b11111001;	//seven segment display: 1
		case 2:
			return 0b10100100;	//seven segment display: 2
		case 3:
			return 0b10110000;	//seven segment display: 3
		case 4:
			return 0b10011001;	//seven segment display: 4
		case 5:
			return 0b10010010;	//seven segment display: 5
		case 6:
			return 0b10000010;	//seven segment display: 6
		case 7:
			return 0b11111000;	//seven segment display: 7
		case 8:
			return 0b10000000;	//seven segment display: 8
		case 9:
			return 0b10010000;	//seven segment display: 9
		case 10:
			return 0b11111100;	//seven segment display: colon
		default:
			return 0b11111111;	//Nothing
	}
}

void find_digits(int *val, int *dig)
{
	//Find minutes digits by modding by 10 and then subtracting that number
	(dig[0]) = val[1] % 10;
	(dig[1]) = ((val[1] - (dig[0]))/10) % 10;

	//find colon value depending on whether seconds are even or odd
	if ((val[0] % 2) == 0)
		(dig[2]) = 10;
	else
		(dig[2]) = -1;

	//if clock is in 12hr mode and the hours are more than 12
	//find hour digits and subtract that value by 12
	if (((mode >> 3) & 1) && val[2] > 12) {
		(dig[3]) = (val[2] - 12) % 10;
		(dig[4]) = (((val[2] - 12) - (dig[3]))/10) % 10;
		mode |= 0x80;
	}
	//otherwise find hour digits the same way as minute digits
	else {
		(dig[3]) = val[2] % 10;
		(dig[4]) = ((val[2] - (dig[3]))/10) % 10;
		mode &= ~(0x80);
	}
	return;
}

void SPI_init()
{
	SPCR = (1<<SPE) | (1<<MSTR);	//SPI enabled, master, low polarity, MSB 1st
	SPSR = (1<<SPI2X);				//run at I/O clock/2
}

int main()
{
	int digits[5];		//holds each decimal digit in count

	LCD_Init();			//Initialize LCD
	
	//timer counter 0 setup, running off external clock
	ASSR |= (1<<AS0);
	TCCR0 |= (1<<CS02) | (1<<CS00);	//normal mode, no prescaled by 128
									// 32768/(128*256) = 1

	//Fast PWM mode, clear on match, Tosc clock, prescale by 256
	TCCR2 = (1<<WGM20) | (1<<WGM21) | (1<<COM21) | (1<<COM20) | (1<<CS20);
	OCR2 = 0xFF;	//initialize at lowest brightness
	
	TCCR1A = 0x00;	//OCn pins off, CTC mode
	TCCR1B = (1<<WGM12) | (1<<CS11) | (1<<CS10);	//CTC, 64 prescale
	TCCR1C = 0x00;				//no forced compare
	OCR1A  = 0x0031;

	TCCR3A = (1<<COM3A1) | (1<<WGM31) | (1<<WGM30);		//non-inverting mode, fast PWM
	TCCR3B = (1<<WGM32) | (1<<WGM33) | (1<<CS30);		//8 bit fast pwm, no prescaling like a boss
	TCCR3C = 0x00;		//no forced compare
	OCR3A = 0x0FE0;
	
	DDRA  = 0xFF;   	//set port A to all outputs
	PORTA = 0xFF;   	//set port A to all ones  (off, active low)

	DDRB = 0xF7; 		//set port B as outputs except PB3 (MISO)
	PORTB = 0x08;		//pull up resistor on SS_n

	DDRD = 0x07;		//First PD0, PD1 pins for temp sensor, PD2 for bar graph RGCLK
	PORTD |= 0x04;
	
	DDRE = 0xC0;		//enable last 2 pins as output
	PORTE = 0x00;
	
	DDRF = 0x00;		//All inputs for ADC
	
	//Enable ADC, free running mode, enable interrupt, 128 prescale
	ADCSRA = (1<<ADEN) | (1<<ADFR) | (1<<ADSC) | (1<<ADIE) | (1<<ADPS2) | (1<<ADPS2) | (1<<ADPS2);
	ADMUX = (1<<REFS0) | (1<<ADLAR) | 0b00101;		//set Aref to AVcc, left flushed, PF5
	
//	init_twi();
	SPI_init();				//Initialize bar graph display
	
	TIMSK |= (1<<TOIE0) | (1<<TOIE2) | (1<<OCIE1A);			//enable interrupts
	ETIMSK|= (1<<TOIE3) | (1<<OCIE3A);
	sei();
	
	while (1){

		if ((mode&0x03) == 0x02)			//if 'set alarm' bit in mode is set
			find_digits(alarm, digits);		//show alarm time on LED display
		else								//otherwise show time on lED display
			find_digits(clk_time, digits);

		for (int i = 0; i < 5; i++) {
			PORTB |= 0x70;				//reset select bits
			PORTB &= (0x8F | (i<<4));	//choose correct LED digit
			PORTA = print_digit(digits[i]);		//decode digit
			_delay_us(10); 	//small delay to make the 7seg more readable
			PORTA = 0xFF;	//turn off display to prevent ghosting
		}

	} //infinite while
}  //main
