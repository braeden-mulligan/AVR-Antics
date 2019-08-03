/*
Author: Braeden Mulligan
*/

#include <stdbool.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

//--- Serial Communication.
#define BAUD 9600
#define BAUD_EQ (F_CPU / 16 / BAUD - 1)
#define UART_BUFFER_SIZE 64
void UART_init() {
	UBRR0H = (BAUD_EQ >> 8);
	UBRR0L = BAUD_EQ;
	UCSR0B = (1 << TXEN0);
	UCSR0C = (1 << UCSZ00) | (1 << UCSZ01);
}

char tx_buffer[UART_BUFFER_SIZE];
uint8_t tx_cursor_send = 0;
uint8_t tx_cursor_put = 0;

// return true if buffer is about to be overwritten next call.
bool tx_buffer_append(char c) {
	//if (tx_cursor_put >= UART_BUFFER_SIZE) tx_cursor_put = 0;
	tx_cursor_put %= UART_BUFFER_SIZE;
	tx_buffer[tx_cursor_put] = c;
	++tx_cursor_put;
	if (tx_cursor_put == tx_cursor_send) return true;
	return false;
}

void UART_transmit(char byte) {
	while (!(UCSR0A & (1 << UDRE0))) {}
	UDR0 = byte;
}

void UART_write() {
	do { 
		UART_transmit(tx_buffer[tx_cursor_send]);
		++tx_cursor_send;
		tx_cursor_send %= UART_BUFFER_SIZE;
	}while (tx_cursor_send != tx_cursor_put);
}

// Call this function as wrapper for sending serial data.
// Checks long strings for buffer overflow. 
void serial_write(char* text) {
	uint8_t i = 0;
	bool warning = false;
	while (text[i] != '\0') {
		if (warning) { 
			UART_write();
			UART_transmit('\n'); UART_transmit('\r');
			char error[UART_BUFFER_SIZE] = "ERROR: Buffer overflow.\n\r";
			for (uint8_t j = 0; j < UART_BUFFER_SIZE; ++j) {
				tx_buffer_append(error[j]);
			}
			UART_write();
			return;
		};
		warning = tx_buffer_append(text[i]);
		++i;
	}
	UART_write();
}
//---

//--- 8 bit timer stuff for ATmega328p fast PWM.
#define cycle_max 167
#define delta_min 0.006 // 1 / cycle_max
// Dimming "blink" the external LED.
volatile float duty_cycle = 0.5;
volatile float delta = delta_min; 
volatile bool climb = true;
ISR(TIMER0_OVF_vect) {
	if (duty_cycle > (1.0 - delta)) climb = false;
	if (duty_cycle < delta) climb = true;
	if (climb) {
		duty_cycle += delta;
	} else {
		duty_cycle -= delta;
	};
	OCR0A = (uint8_t) (duty_cycle * cycle_max); 
}

bool pwm_on = false;
void PWM_start(bool init) {
	if (init) {
		DDRD |= (1 << PORTD6);
		OCR0A = (uint8_t) (duty_cycle * 255.0); 
		sei();
	}else {
		PORTB &= ~_BV(DDB5);
		serial_write("on\n\r");
	};
	TCCR0A = (1 << COM0A1) | (1 << WGM00) | (1 << WGM01);
	TIMSK0 = (1 << TOIE0); // Timer mask - type of interrupt.
	TCCR0B = (1 << CS00) | (1 << CS02); // Prescale 1.
	pwm_on = true;
}

void PWM_halt() {
	TCCR0A = 0;
	TIMSK0 = 0; 
	TCCR0B = 0;
	PORTD &= ~_BV(DDD6);

	PORTB |= _BV(DDB5);
	pwm_on = false;
	serial_write("off\n\r");
}
//---

//--- General 16 bit Timer.
// Setting prescaler starts timer.
void TIMER_init() {
	TCCR1B = (1 << WGM12); // CTC: Clear timer on compare
	OCR1A = 785; // Remainder ticks for ~50ms @ 16MHz
	TIMSK1 = (1 << OCIE1A); // Interrupt control
	sei(); // Set SREG bit I
	TCCR1B |= (1 << CS10) | (1 << CS12); // Prescalar 1024
}

volatile bool pressed = false;
volatile bool held = false;
ISR(TIMER1_COMPA_vect) {
	if (!(PINB & _BV(DDB0))) {
		if (!held) {
			pressed = true;
			held = true;
		};
	}else {
		held = false;
	};
}

inline void PWM_toggle() { //TODO: Would inlining improve performance.
	if (pressed) {
		pressed = false;
		if (pwm_on) {
			PWM_halt();
		}else {
			PWM_start(false);
		};
	};
}
//---

//--- Analogue to Digital.
// Potentiometer controls delta magnitude.
void ADC_convert() {
	ADCSRA |= (1 << ADSC);
}

void ADC_init() {
	ADMUX = (1 << REFS0) | (1 << MUX0) | (1 << MUX2);
	ADCSRA = (1 << ADEN) | (1 << ADIE); // Clock scaling for ADC
	ADCSRA |= (1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2); // Trigger interrupt upon conversion.
	DIDR0 = (1 << ADC5D);

	ADC_convert();
}

ISR(ADC_vect) {
	delta = delta_min + (0.33 * ( (float) ADC / 1024.0));
}
//---

//--- External interrupt stuff.
/*
volatile char exti = true; 
ISR(INT0_vect) {
	exti ^= true;
}

void EXT_init(){
	PORTD |= (1 << PORTD2); // Pin 2 input pullup.
	EIMSK |= (1 << INT0);
}

void button_toggle() {
	if (exti) {
		PORTB &= ~(1 << PORTB5);
	}else {
		PORTB |= (1 << PORTB5);
	};
}
*/
//---

int main(void) {
	// Initialize internal LED off.
	DDRB |= _BV(DDB5);
	PORTB &= ~_BV(DDB5);

	// Set pin 8 input.
	DDRB &= ~_BV(DDB0);
	PORTB |= _BV(DDB0);

	PWM_start(true);	
	TIMER_init();
	ADC_init();
	//EXT_init();
	UART_init();

	while (true) {
		PWM_toggle();
		ADC_convert();
	}
} // END MAIN

