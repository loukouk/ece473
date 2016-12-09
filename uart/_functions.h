#ifndef _FUNCTIONS_H_ECE473
#define _FUNCTIONS_H_ECE473

#define IS_ALARM_TRIGGER (alarm_mode&0x01)
#define SET_ALARM_TRIGGER (alarm_mode|=0x01)
#define CLEAR_ALARM_TRIGGER (alarm_mode&=0xFE)
#define IS_ALARM_SNOOZE (alarm_mode&0x02)
#define SET_ALARM_SNOOZE (alarm_mode|=0x02)
#define CLEAR_ALARM_SNOOZE (alarm_mode&=0xFD)

// pusbutton definitions
//	0: setting mode
//	1: show the alarm instead of time
//	2: show the radio freq instead of time
//	3: turn alarm on/off
//	4: AM/PM mode
//	5: Celcius/Farenheit
//	6: Radio mode (all other buttons become station presets)
//	7: change song/radio that the alarm plays

#define IS_SETTING 	(mode&0x01)
#define IS_SHOW_ALARM 	((mode>>1)&0x01)
#define IS_SHOW_FREQ 	((mode>>2)&0x01)
#define IS_ALARM_ARM 	((mode>>3)&0x01)
#define IS_AM_PM	((mode>>4)&0x01)
#define F_NOT_C 	((mode>>5)&0x01)
#define IS_RADIO_MODE 	((mode>>6)&0x01)
#define IS_SNOOZING 	((mode>>7)&0x01)

#define SNOOZE_MINS 0
#define SNOOZE_SECS 10
#define LCD_FREEZE_TIME 16

#define CW 1
#define CCW 2

#define VOLUME_INDEX_MAX 16
#define BRIGHTNESS_INDEX_MAX 20

#define BEGIN_ADC_CONVERSION (ADCSRA|=(1<<ADSC))
#define CLEAR_ADC_INT_FLAG (ADCSRA|=(1<<ADIF))
#define ADC_MIN 30
#define ADC_MAX 230
#define ADC_DIV 10

enum {LCD_ALARM_OFF = 0x01, LCD_ALARM_ON, LCD_ALARM_TRIG, LCD_SET_SONG,
	LCD_SET_TIME,LCD_SET_ALARM, LCD_SET_FREQ, LCD_VOLUME,
	LCD_SHOW_ALARM, LCD_SHOW_FREQ, LCD_PLAYING_RADIO, LCD_CLEAR};

void init_globals();
int8_t debounce_switch(uint8_t pin);
int8_t debounce_spi_buttons(uint8_t enc_pin, uint8_t enc_data);
uint8_t decode_digit(int8_t digit);
uint8_t spi_send_read(uint8_t data);
uint8_t find_direction (uint8_t state, uint8_t i);
void read_pushbuttons ();
void print_get_up ();
void decode_time(uint8_t hours, uint8_t minutes);
void decode_freq(uint16_t freq);
void adjust_alarm_time(uint8_t encoder_data);
void read_adc();
void read_int_temp(char temp_str[]);
void lcd_update();
void mode_set();
void lcd_mode_set();
void init_radio();
void play_radio();
void stop_radio();
void reset_radio();
void request_temp();
#endif
