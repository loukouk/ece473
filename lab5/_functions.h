#ifndef _FUNCTIONS_H_ECE473
#define _FUNCTIONS_H_ECE473

#define IS_ALARM_TRIGGER (alarm_mode&0x01)
#define SET_ALARM_TRIGGER (alarm_mode|=0x01)
#define CLEAR_ALARM_TRIGGER (alarm_mode&=0xFE)
#define IS_ALARM_SNOOZE (alarm_mode&0x02)
#define SET_ALARM_SNOOZE (alarm_mode|=0x02)
#define CLEAR_ALARM_SNOOZE (alarm_mode&=0xFD)

#define IS_SHOW_ALARM (mode&0x01)
#define IS_SETTING ((mode>>1)&0x01)
#define IS_AM_PM ((mode>>2)&0x01)
#define IS_ALARM_ARM ((mode>>3)&0x01)
#define IS_SNOOZING (mode>>4)

#define SNOOZE_MINS 0
#define SNOOZE_SECS 10

#define CW 1
#define CCW 2

#define VOLUME_INDEX_MAX 16
#define BRIGHTNESS_INDEX_MAX 20

#define BEGIN_ADC_CONVERSION (ADCSRA|=(1<<ADSC))
#define CLEAR_ADC_INT_FLAG (ADCSRA|=(1<<ADIF))
#define ADC_MIN 30
#define ADC_MAX 230
#define ADC_DIV 10

#define F_NOT_C 0
#define UART_WR_DELAY 100

enum {LCD_ALARM_OFF, LCD_ALARM_ON, LCD_ALARM_TRIG, LCD_SET_TIME,
	LCD_SET_ALARM, LCD_SET_FREQ, LCD_SET_SONG, LCD_VOLUME};

void init_globals();
int8_t debounce_switch(uint8_t pin);
int8_t debounce_spi_buttons(uint8_t enc_pin, uint8_t enc_data);
uint8_t decode_digit(int8_t digit);
uint8_t spi_send_read(uint8_t data);
uint8_t find_direction (uint8_t state, uint8_t i);
void read_pushbuttons ();
void print_get_up ();
void decode_time(uint8_t hours, uint8_t minutes);
void adjust_alarm_time(uint8_t encoder_data);
void read_adc();
void read_int_temp(char temp_str[]);
void lcd_update();

volatile  int8_t time[3];
volatile  int8_t alarm[3];
volatile  int8_t snooze[3];
volatile uint8_t SEGS[5];
volatile uint8_t mode;
volatile uint8_t encoder_mode;
volatile uint8_t alarm_mode;
volatile uint8_t lcd_mode;
volatile uint8_t volume_index;
volatile uint8_t brightness_index;

volatile uint16_t current_fm_freq;
volatile uint8_t  current_volume;

volatile uint8_t STC_interrupt;
volatile uint8_t UART_COUNT;

#endif
