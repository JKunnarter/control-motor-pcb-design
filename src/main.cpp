#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

// ADC calibration limits
#define ADC_MIN      15
#define ADC_MAX      1023

// RPM measurement parameters
#define RPM_TIMEOUT  200    // ms until RPM display resets

// LCD pin definitions
#define LCD_DDR      DDRD
#define LCD_PORT     PORTD
#define LCD_EN       PD3
#define LCD_RS_DDR   DDRB
#define LCD_RS_PORT  PORTB
#define LCD_RS       PB2

// PWM output
#define PWM_DDR      DDRB
#define PWM_PIN      PB3

// Button input (INT0)
#define BUTTON_DDR   DDRD
#define BUTTON_PORT  PORTD
#define BUTTON_PIN   PD2

// Motor driver pins
#define MOTOR_DDR    DDRB
#define MOTOR_IN1    PB1
#define MOTOR_IN2    PB4

// Debug LED
#define LED_DDR      DDRB
#define LED_PIN      PB5

// Global flags and counters
volatile uint8_t  is_running = 0;
volatile uint32_t millis_counter;      // Tracks time in milliseconds
volatile uint32_t last_capture_time;   // Last time a capture event occurred

volatile uint32_t timer_overflow_count; // Accumulates timer overflows
volatile uint32_t first_time;           // Time of first pulse in a sequence
volatile uint32_t period;               // Time for 20 pulses (one revolution if 20 pulses/rev)
volatile uint8_t  pulse_count;          // Number of pulses counted
volatile uint8_t  ready;                // Flag indicating RPM is ready to display

// LCD low-level functions
static void LCD_Send4(uint8_t nibble) {
	LCD_PORT = (LCD_PORT & 0x0F) | (nibble & 0xF0);
	LCD_PORT |= (1 << LCD_EN);
	_delay_us(1);
	LCD_PORT &= ~(1 << LCD_EN);
}

static void LCD_Write(uint8_t value, uint8_t rs) {
	if (rs) {
		LCD_RS_PORT |= (1 << LCD_RS);
		} else {
		LCD_RS_PORT &= ~(1 << LCD_RS);
	}

	LCD_Send4(value & 0xF0);
	_delay_us(50);
	LCD_Send4((value << 4) & 0xF0);
	_delay_ms(2);
}

#define LCD_Command(cmd) LCD_Write(cmd, 0)
#define LCD_Data(dat)    LCD_Write(dat, 1)
#define LCD_Clear()      do { LCD_Command(0x01); _delay_ms(2); } while (0)

// Utility function declarations
void LCD_Init(void);
void LCD_GotoXY(uint8_t row, uint8_t col);
void LCD_Print(const char *s);
void LCD_Printf(const char *fmt, ...);

void TIMER2_PWM_Init(void);
static inline void setDuty(uint8_t duty);

void TIMER1_CAPTURE_Init(void);

static inline void ADC_Init(void);
static inline uint16_t adcRead(uint8_t channel);

void Button_Init(void);
void Motor_Init(void);
void System_Init(void);

// Interrupt service routines
ISR(TIMER1_OVF_vect);
ISR(TIMER1_CAPT_vect);
ISR(INT0_vect);

int main(void) {
	// Initialize peripherals
	LCD_Init();
	System_Init();

	// Configure I/O pins
	DDRC &= ~(1 << PC0);    // ADC input
	PORTC |= (1 << PC0);    // Enable pull-up
	LED_DDR |= (1 << LED_PIN);  // LED as output
	PORTB &= ~(1 << LED_PIN);   // LED off initially

	// Initialize system components with interrupts disabled
	cli();
	ADC_Init();
	Motor_Init();
	Button_Init();
	TIMER2_PWM_Init();
	TIMER1_CAPTURE_Init();
	sei();

	uint8_t last_duty = 0xFF;   // Last set duty cycle
	uint8_t prev_rpm = 0xFF;    // Previous RPM value for display update
	static uint16_t prev_adc = 0; // Previous ADC reading for edge case handling

	while (1) {
		// Initialize RPM display on first run
		if (prev_rpm == 0xFF) {
			LCD_GotoXY(0, 0);
			LCD_Printf("RPM:%5d", 0);
			prev_rpm = 0; // Reset after initialization
		}

		// Read ADC and calculate duty cycle
		uint16_t adc = adcRead(0);
		uint8_t duty;

		if (adc <= ADC_MIN) {
			if (prev_adc > 100) {
				duty = 100; // Potentiometer at maximum
				} else {
				duty = 0;   // Potentiometer at minimum
			}
			} else {
			// Map ADC range (15-1023) to duty cycle (0-100%)
			duty = (uint8_t)(((adc - ADC_MIN) * 100UL) / (ADC_MAX - ADC_MIN));
		}
		prev_adc = adc; // Update previous ADC value

		// Update PWM and LCD if duty changes
		if (duty != last_duty) {
			last_duty = duty;
			LCD_GotoXY(1, 0);
			LCD_Printf("Duty:%4d%%", duty);
			setDuty(duty);
		}

		// Update RPM display
		if (!is_running || (millis_counter - last_capture_time > RPM_TIMEOUT)) {
			if (prev_rpm != 0) {
				prev_rpm = 0;
				LCD_GotoXY(0, 0);
				LCD_Printf("RPM:%5d", 0);
			}
			} else if (ready) {
			ready = 0;
			// Calculate RPM: 60 * frequency, where period is time for 20 pulses
			// Assuming 20 pulses per revolution, period/F_CPU is time per rev
			uint16_t rpm = (uint16_t)((60.0f * F_CPU) / (float)period);
			if (rpm != prev_rpm) {
				prev_rpm = rpm;
				LCD_GotoXY(0, 0);
				LCD_Printf("RPM:%5d", rpm);
			}
		}
	}
	return 0;
}

// Function definitions

void LCD_Init(void) {
	LCD_DDR = 0xFF;         // LCD data port as output
	LCD_RS_DDR |= (1 << LCD_RS); // RS pin as output
	_delay_ms(20);
	LCD_Send4(0x30);
	_delay_ms(5);
	LCD_Send4(0x30);
	_delay_us(200);
	LCD_Send4(0x20);        // Set 4-bit mode
	_delay_us(200);
	LCD_Command(0x28);      // 4-bit, 2-line mode
	LCD_Command(0x0C);      // Display on, cursor off
	LCD_Clear();
}

void LCD_GotoXY(uint8_t row, uint8_t col) {
	uint8_t addr = (row ? 0x40 : 0x00) + col;
	LCD_Command(0x80 | addr);
}

void LCD_Print(const char *s) {
	while (*s) {
		LCD_Data(*s++);
	}
}

void LCD_Printf(const char *fmt, ...) {
	char buf[32];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	LCD_Print(buf);
}

void TIMER2_PWM_Init(void) {
	PWM_DDR |= (1 << PWM_PIN);  // PWM pin as output
	OCR2 = 128;                 // Initial 50% duty cycle
	// Fast PWM mode, clear OC2 on compare match, prescaler = 8
	TCCR2 |= (1 << COM21) | (1 << WGM21) | (1 << WGM20) | (1 << CS21);
}

static inline void setDuty(uint8_t duty) {
	if (duty > 100) {
		duty = 100; // Cap duty cycle at 100%
	}
	// Map duty 0-100% to OCR2 82-255 (~32%-100% PWM)
	// Possibly to ensure motor starts above a minimum threshold
	OCR2 = (uint8_t)(82 + (duty * 173UL) / 100);
}

void TIMER1_CAPTURE_Init(void) {
	DDRB &= ~(1 << PB0);    // ICP1 (PB0) as input
	PORTB |= (1 << PB0);    // Enable pull-up
	TCNT1 = 0;
	timer_overflow_count = 0;
	// Timer1: no prescaler, input capture noise canceler
	TCCR1B = (1 << CS10) | (1 << ICNC1);
	TIMSK = (1 << TICIE1) | (1 << TOIE1); // Enable capture and overflow interrupts
}

ISR(TIMER1_OVF_vect) {
	timer_overflow_count += 0x10000UL; // Add 65536 per overflow
	millis_counter += 4; // Approx. 4ms per overflow at 16MHz
}

ISR(TIMER1_CAPT_vect) {
	uint32_t now = timer_overflow_count + ICR1; // Total counts at capture
	last_capture_time = millis_counter;
	if (++pulse_count == 1) {
		first_time = now; // Start of pulse sequence
		} else if (pulse_count >= 20) {
		period = now - first_time; // Time for 20 pulses
		pulse_count = 0;
		first_time = now;
		ready = 1; // Signal RPM ready
	}
}

static inline void ADC_Init(void) {
	ADMUX = (1 << REFS0);   // AVCC as reference
	// Enable ADC, prescaler = 128 (125kHz at 16MHz)
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

static inline uint16_t adcRead(uint8_t channel) {
	ADMUX = (ADMUX & 0xF0) | (channel & 0x07); // Select channel
	ADCSRA |= (1 << ADSC);  // Start conversion
	while (ADCSRA & (1 << ADSC)); // Wait for completion
	return ADC;
}

void Button_Init(void) {
	BUTTON_DDR &= ~(1 << BUTTON_PIN);   // Button pin as input
	BUTTON_PORT |= (1 << BUTTON_PIN);   // Enable pull-up
	MCUCR |= (1 << ISC01);              // Falling edge trigger
	MCUCR &= ~(1 << ISC00);
	GICR |= (1 << INT0);                // Enable INT0
}

ISR(INT0_vect) {
	_delay_ms(10); // Simple debounce
	if (!(PIND & (1 << BUTTON_PIN))) { // Button still pressed
		PORTB ^= (1 << LED_PIN); // Toggle LED
		if (!is_running) {
			PORTB |= (1 << MOTOR_IN1);
			PORTB &= ~(1 << MOTOR_IN2); // Forward direction
			} else {
			PORTB &= ~((1 << MOTOR_IN1) | (1 << MOTOR_IN2)); // Brake mode
		}
		is_running = !is_running;
	}
}

void Motor_Init(void) {
	MOTOR_DDR |= (1 << MOTOR_IN1) | (1 << MOTOR_IN2); // Motor pins as outputs
	PORTB &= ~((1 << MOTOR_IN1) | (1 << MOTOR_IN2));  // Initially off
}

void System_Init(void) {
	LCD_GotoXY(0, 0);
	LCD_Print("Linh");
	LCD_GotoXY(1, 0);
	LCD_Print("20226662");
	_delay_ms(1000);
	LCD_Clear();
}