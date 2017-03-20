#include <dma.h>

/*
 * The DMA transfers in this file use a
 * 32-bit (4B) bus width.  Bear in mind.
 */

volatile uint32_t DMATCCount = 0;

/**
 * DMA interrupt handler.
 */
void DMA_IRQHandler(void) {
	uint32_t reg_val;
	reg_val = LPC_GPDMA->DMACIntTCStat;
	if (reg_val) {
		DMATCCount++;
		LPC_GPDMA->DMACIntTCClear |= reg_val;
	}
}

/**
 * Powers up the DMA controller and disables
 * linked list.
 */
inline void DMA_power_up(void) {
	LPC_SC->PCONP |= 1 << 29; // Power up
	LPC_GPDMA->DMACConfig = 0x01; // Enable GPDMA
	while (!(LPC_GPDMA->DMACConfig & 0x01))
		;
	NVIC_EnableIRQ(DMA_IRQn);
	LPC_GPDMACH0->DMACCLLI = 0; // Disable linked list
}

/**
 * Changes the DMA destination address.
 *
 * @param dest_addr the destination address
 */
inline void DMA_set_dest(uint32_t dest_addr) {
	LPC_GPDMACH0->DMACCDestAddr = dest_addr;
}

/**
 * Initializes the DMA controller with given addresses.
 *
 * @param src_addr  the source address
 * @param dest_addr the address of the buffer to store
 *     the data in
 */
inline void DMA_init(uint32_t src_addr, uint32_t dest_addr) {
	LPC_GPDMACH0->DMACCSrcAddr = src_addr;
	LPC_GPDMACH0->DMACCDestAddr = dest_addr;
}

/**
 * Starts a DMA transfer.  The DMA controller must be
 * initialized previously.
 *
 * @param xfer_size the number of 32-bit transfers to be performed
 */
inline void DMA_start(uint16_t xfer_size) {
	// Clear any current TC interrupts
	LPC_GPDMA->DMACIntTCClear = 0x01;

	LPC_GPDMACH0->DMACCControl =
			(xfer_size & 0x0fff) // transfer size (0-11) = 32
	        | (0 << 12)          // source burst size (12-14) = 1
			| (0 << 15)          // destination burst size (15-17) = 1
			| (2 << 18)          // source width (18-20) = 32-bit
			| (2 << 21)          // destination width (21-23) = 32-bit
			| (0 << 24)          // source AHB select (24) = AHB 0
			| (0 << 25)          // destination AHB select (25) = AHB 0
			| (1 << 26)          // source increment (26) = increment
			| (1 << 27)          // destination increment (27) = increment
			| (0 << 28)          // mode select (28) = access in user mode
			| (0 << 29)          // (29) = access not bufferable
			| (0 << 30)          // (30) = access not cacheable
			| (1 << 31);         // terminal count interrupt enabled

	LPC_GPDMACH0->DMACCConfig =
			1                  // channel enabled (0)
			| (0 << 1)         // source peripheral (1-5) = none
			| (0 << 6)         // destination request peripheral (6-10) = none
			| (0 << 11)        // flow control (11-13) = M2M
			| (0 << 14)        // (14) = mask out error interrupt
			| (1 << 15)        // (15) = don't mask out terminal count interrupt
			| (0 << 16)        // (16) = no locked transfers
			| (0 << 18);       // (27) = no HALT

	//while (!(LPC_GPDMACH0->DMACCConfig & 0x01))
	//	;
}

/**
 * Waits for a DMA transfer to finish.
 */
inline void DMA_wait(void) {
	while (!DMATCCount)
		;

	DMATCCount = 0;
	LPC_GPDMACH0->DMACCSrcAddr += 4; // Increment once more.
}

/**
 * Waits for a DMA transfer to finish and initializes a
 * destination address to be used for a subsequent transfer.
 *
 * @param next_addr the destination address to be used in a
 *     subsequent transfer
 */
inline void DMA_wait_and_prepare(uint32_t next_addr) {
	DMA_wait();
	LPC_GPDMACH0->DMACCDestAddr = next_addr;
}
