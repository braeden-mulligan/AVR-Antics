/*
Author: Braeden Mulligan

*/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define false 0
#define true 1


//--- 8 bit timer stuff for fast PWM.
float duty_cycle = 0.5;
float delta = 0.02;
volatile char climb = true;
ISR(TIMER0_OVF_vect) {
	if (duty_cycle > (1.0 - delta)) climb = false;
	if (duty_cycle < delta) climb = true;
	if (climb) {
		duty_cycle += delta;
	} else {
		duty_cycle -= delta;
	};
	OCR0A = (char) (duty_cycle * 255); 
}

char pwm_on = false;
void PWM_init() {
	DDRD |= (1 << PORTD6);
	TCCR0A = (1 << COM0A1) | (1 << WGM00) | (1 << WGM01);
	TIMSK0 = (1 << TOIE0); // Timer mask - type of interrupt.
	OCR0A = (char) duty_cycle * 255; 
	sei();
	TCCR0B = (1 << CS00) | (1 << CS02); // Prescale 1.
	pwm_on = true;
}

void PWM_halt() {
	TCCR0B = 0;
	TCCR0A = 0;
	TIMSK0 = 0; 
	PORTD &= ~_BV(DDD6);

	PORTB |= _BV(DDB5);
	pwm_on = false;
}

void PWM_restart() {
	PORTB &= ~_BV(DDB5);

	TCCR0A = (1 << COM0A1) | (1 << WGM00) | (1 << WGM01);
	TIMSK0 = (1 << TOIE0); 
	TCCR0B = (1 << CS00) | (1 << CS02);
	pwm_on = true;
}
//---
		

//--- ADC Shit. Depend on PWM.
void ADC_init() {
}
// potentiometer for delta magnitude
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

volatile char pressed = false;
volatile char held = false;
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

inline void PWM_toggle() { //TODO: See if inlining improves performance.
	if (pressed) {
		pressed = false;
		if (pwm_on) {
			PWM_halt();
		}else {
			PWM_restart();
		};
	};
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
//---
*/


int main(void) {
	// Initialize internal LED off.
	DDRB |= _BV(DDB5);
	PORTB &= ~_BV(DDB5);

	// Set pin 8 input.
	DDRB &= ~_BV(DDB0);
	PORTB |= _BV(DDB0);

	PWM_init();	
	TIMER_init();
	//EXT_init();
	//ADC_init();

	while (true) {
		PWM_toggle();
	}
} // END MAIN

