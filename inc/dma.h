#ifndef __DMA_H
#define __DMA_H
#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

inline void DMA_power_up(void);
inline void DMA_init(uint32_t src_addr, uint32_t dest_addr);
inline void DMA_set_dest(uint32_t dest_addr);
inline void DMA_start(uint16_t xfer_size);
inline void DMA_wait();
inline void DMA_wait_and_prepare(uint32_t next_addr);

#endif
