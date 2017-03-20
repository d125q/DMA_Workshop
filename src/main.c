/*
 * main.c
 *
 *  Created on: Jan 4, 2016
 *      Author: Dario Gjorgjevski
 */

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>
#include "md5.h"
#include "iap_driver.h"
#include "payload_generator.h"
#include "dma.h"
#include "verify.h"
#include "stdbool.h"
#include "led.h"

#define COUNT_CLK_CYCLES (1)

int main(void) {
	e_iap_status iap_status;

	// Fill the flash with payload and hash data
	iap_status = (e_iap_status) generator_init();
	if (iap_status != CMD_SUCCESS) {
		while (1)
			; // Error!!!
	}

	LED2_setup();
	DMA_power_up();

#ifdef COUNT_CLK_CYCLES
	LPC_SC->PCONP |= 1 << 2; // Power up Timer 1
	LPC_SC->PCLKSEL0 |= 0x01 << 4; // CCLK
	LPC_TIM1->PR = 0xffffffff;
	LPC_TIM1->TCR |= 1 << 1; // Manually reset Timer 1 (forced)
	LPC_TIM1->TCR &= ~(1 << 1); // Stop resetting the timer
	LPC_TIM1->MCR = 0x00;
	LPC_TIM1->TCR |= 1 << 0; // Start T1

	uint64_t start = LPC_TIM1->TC * 0xffffffff + LPC_TIM1->PC; // Start
#endif
	bool correct = run_verification();
#ifdef COUNT_CLK_CYCLES
	uint64_t end = LPC_TIM1->TC * 0xffffffff + LPC_TIM1->PC; // End
#endif

	if (correct)
		LED2_turn_on();
	else
		LED2_start_blinking();

	while (1)
		;

	return 0;
}
