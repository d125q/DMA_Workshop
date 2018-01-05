/*
 * led.c
 *
 *  Created on: Jan 4, 2016
 *      Author: Dario Gjorgjevski, Kire Kolaroski
 */

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

/**
 * Turns LED2 on.
 */
inline void LED2_turn_on(void) {
	LPC_GPIO0->FIOPIN |= 1 << 22; // Turn LED2 on
}

/**
 * Turns LED2 off.
 */
inline void LED2_turn_off(void) {
	LPC_GPIO0->FIOPIN |= 0 << 22; // Turn LED2 off
}

/**
 * Configures LED2.
 */
inline void LED2_setup(void) {
	LPC_SC->PCONP |= 1 << 15;            // Power up GPIO & Timer 0
	LPC_SC->PCONP |= 1 << 1;             // Power up Timer 0
	LPC_SC->PCLKSEL0 |= 1 << 2;          // Use the CPU clock
	LPC_TIM0->MR0 = 1 << 24;             // Value suitable for blinking
	LPC_TIM0->MCR |= 1 << 0;             // Interrupt on Match 0 compare
	LPC_TIM0->MCR |= 1 << 1;             // Reset timer on Match 0
	LPC_TIM0->TCR |= 1 << 1;             // Manually reset Timer 0 (forced)
	LPC_TIM0->TCR &= ~(1 << 1);          // Stop resetting the timer
	NVIC_EnableIRQ(TIMER0_IRQn);         // Enable the interrupt via NVIC
	LPC_PINCON->PINSEL1 &= (~(3 << 12)); // Make P0.22 be output
	LPC_GPIO0->FIODIR |= 1 << 22;        // Put P0.22 into output mode
	                                     // (LED2 is connected to P0.22)
	LED2_turn_off();
}

/**
 * Interrupt handler for blinking.
 */
void TIMER0_IRQHandler(void) {
	if ((LPC_TIM0->IR & 0x01) == 0x01) {
		LPC_TIM0->IR |= 1 << 0;       // Clear MR0 interrupt flag
		LPC_GPIO0->FIOPIN ^= 1 << 22; // Toggle LED2
	}
}

/**
 * Makes LED2 start blinking.
 */
inline void LED2_start_blinking(void) {
	LED2_turn_on();
	LPC_TIM0->TCR |= 1 << 0; // Start timer
}

/**
 * Makes LED2 stop blinking.
 */
inline void LED2_stop_blinking(void) {
	LPC_TIM0->TCR |= 1 << 1;    // Manually reset Timer 0 (forced)
	LPC_TIM0->TCR &= ~(1 << 1); // Stop resetting the timer
	LED2_turn_off();
}
