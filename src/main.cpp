#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdint.h>

volatile uint8_t button_on = 0;

volatile uint16_t last_cap=0, this_cap=0;
volatile uint8_t cap_flag=0;

#define PRESCALER 8
#define PPR 20  

#define LCD_DDR   DDRD
#define LCD_PORT  PORTD
#define EN        PD3

#define RS_DDR    DDRB
#define RS_PORT   PORTB
#define RS         PB2

static void LCD_Send4(uint8_t nibble) {
	LCD_PORT = (LCD_PORT & 0x0F) | (nibble & 0xF0);
	LCD_PORT |=  (1 << EN);
	_delay_us(1);
	LCD_PORT &= ~(1 << EN);
}

static void LCD_Write(uint8_t value, uint8_t rs_mode) {
	if (rs_mode) RS_PORT |=  (1 << RS);
	else         RS_PORT &= ~(1 << RS);

	LCD_Send4(value & 0xF0);
	_delay_us(50);
	LCD_Send4((value << 4) & 0xF0);
	_delay_ms(2);
}

void LCD_Command(uint8_t cmd) {
	LCD_Write(cmd, 0);
}

void LCD_Data(uint8_t data) {
	LCD_Write(data, 1);
}

void LCD_Init(void) {
	LCD_DDR = 0xFF;
	RS_DDR |= (1 << RS);

	_delay_ms(20);
	LCD_Send4(0x30); _delay_ms(5);
	LCD_Send4(0x30); _delay_us(200);
	LCD_Send4(0x20); _delay_us(200);

	LCD_Command(0x28);
	LCD_Command(0x0C);
	LCD_Command(0x01);
	_delay_ms(2);
}

void LCD_Clear(void) {
	LCD_Command(0x01);
	_delay_ms(2);
}

void LCD_GotoXY(uint8_t row, uint8_t col) {
	uint8_t addr = (row == 0 ? 0x00 : 0x40) + col;
	LCD_Command(0x80 | (addr & 0x7F));
}

void LCD_Print(const char *s) {
	while (*s) {
		LCD_Data(*s++);
	}
}


ISR(TIMER1_CAPT_vect) {
	this_cap = ICR1;    
	cap_flag = 1;
}


void timer1_init_pwm_and_capture(void) {
	DDRB |= (1<<PB1);
	TCCR1A = (1<<COM1A1) | (1<<WGM11);

	TCCR1B = (1<<WGM13)
	| (1<<WGM12)
	| (1<<CS11)    // prescaler = 8
	| (1<<ICES1);  // rising edge capture

	ICR1  = 20000;
	OCR1A = ICR1/2;   

	TIMSK |= (1<<TICIE1);
	
	DDRB  &= ~(1<<PB0);
	PORTB &= ~(1<<PB0);  
}


// ADC Initialization
static inline void adcInit(void) {
	ADMUX  = (1 << REFS0);
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

static inline uint16_t adcRead(uint8_t channel) {
	ADMUX = (ADMUX & 0xF0) | (channel & 0x07);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADC;
}

void buttonInit(void) {
	DDRD &= ~(1 << PD2);
	PORTD |= (1 << PD2);
	MCUCR |= (1 << ISC01);
	MCUCR &= ~(1 << ISC00);
	GICR |= (1 << INT0);
}

void motorInit(void) {
	DDRB |= (1 << PB3) | (1 << PB4);
	PORTB &= ~((1 << PB3) | (1 << PB4));
}

static inline void setDuty(uint8_t duty) {
	uint16_t tDuty = (uint32_t) ICR1 * duty / 100;
	OCR1A = (duty > 100) ? (ICR1) : (tDuty);
}

ISR(INT0_vect) {
	_delay_ms(10);
	if ((PIND & (1 << PD2)) == 0) button_on = 1;
}

int main(void) {
	LCD_Init();
	
	DDRC &= ~(1 << PC0);
	PORTC |= (1 << PC0);
	DDRB |= (1 << PB5);
	PORTB &= ~(1 << PB5);
	DDRB |= (1 << PB1);
	
	cli();
	motorInit();
	buttonInit();
	timer1_init_pwm_and_capture();
	adcInit();
	sei();
	
	uint16_t rawDuty;
	uint8_t duty;
	double t, f, dt;
	int RPM; 
	uint8_t last_duty = 0xFF; 
	int last_rpm = 1000;
	while (1) {
		rawDuty = adcRead(0);
		duty = rawDuty;
		setDuty(100 - duty);
		LCD_GotoXY(1, 0);  
		LCD_Print("Duty=");
    
		if (duty != last_duty) {
			last_duty = duty;
	
			LCD_GotoXY(1, 0);      
			char buf[5];
			itoa(100 - duty, buf, 10);
			LCD_Print("Duty=");
			LCD_Print(buf);
			LCD_Print("%  ");      
		}
		
		if (RPM  != last_rpm) {
			last_rpm = RPM;
			LCD_GotoXY(0, 0);        
			char buf[5];
			itoa(RPM, buf, 10);
			LCD_Print("RPM=");
			LCD_Print(buf);
    }
    
		if (cap_flag) {
            cap_flag = 0;
            dt = this_cap - last_cap;
            last_cap = this_cap;

            t = (double)dt * PRESCALER / F_CPU;
            f = 1.0 / t;            
            RPM = (int) (f * 60.0) / PPR;         
		}
		
		if (button_on) {
			button_on = 0;
			PORTB ^= (1 << PB5);
			PORTB ^= (1 << PB3);
    }
	}
}
