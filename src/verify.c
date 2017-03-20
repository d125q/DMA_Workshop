/*
 * verify.c
 *
 *  Created on: Jan 2, 2016
 *      Author: Dario Gjorgjevski
 */

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include "verify.h"
#include "dma.h"
#include "md5.h"
#include <stdlib.h>
#include <string.h>

#define HEADER_ADDR (0x00004000)
#define ARCHIVE_ADDR (0x00005000)
#define MEMORY_AVAILABLE (15716)

#define CHECK_HEADER (1)
#define CHECK_FOOTER (1)

#ifdef CHECK_HEADER
#define HEADER_PATTERN 0xABBA
#endif
#ifdef CHECK_FOOTER
#define FOOTER_PATTERN 0xABABABABABABABAB
#endif

/**
 * Optimal chunk buffer size.  Set to a reasonable
 * default (1), but will be updated as soon as the
 * header is read.
 */
uint32_t CHUNK_BUF_SIZE = 1;

/**
 * Struct to keep the archive header in.
 */
typedef struct {
	uint16_t preamble;
	uint16_t chunk_count;
	uint32_t chunk_size;
} header_t;

header_t h;

/**
 * Archive chunk buffers.
 */
uint8_t chunk_buf1[MEMORY_AVAILABLE];
uint8_t chunk_buf2[MEMORY_AVAILABLE];
typedef uint8_t * chunk_buf_t;

/**
 * Allocates a chunk buffer.
 *
 * @return a chunk buffer (pointer to memory)
 */
chunk_buf_t alloc_chunk_buf() {
	return (chunk_buf_t) malloc(sizeof(uint8_t) * h.chunk_size * CHUNK_BUF_SIZE);
}

/**
 * Checks a chunk buffer for errors (mismatching hash values).
 *
 * @param chunk_buf the chunk buffer to check
 * @param buf_size  the size of the chunk buffer
 * @return true if the hashes match, false otherwise
 */
bool check_chunk_buf(const chunk_buf_t chunk_buf, uint32_t buf_size) {
	unsigned int i;
	uint8_t hash[16];

	for (i = 0; i < buf_size; ++i) {
		MD5_CTX ctx;

		MD5_Init(&ctx);
		MD5_Update(&ctx, chunk_buf + (i * h.chunk_size + 16), h.chunk_size - 16);
		MD5_Final(hash, &ctx);

		if (memcmp(hash, chunk_buf + (i * h.chunk_size), 16) != 0)
			return false;
	}

	return true;
}

/**
 * Verifies all chunks in the archive.
 *
 * @return true if the chunks are correct, false otherwise
 */
bool validate_chunks()
{
	// Use a pointer because we will be interchanging buffers
	chunk_buf_t curr_buf;
	unsigned int i;
	unsigned int chunks_left = h.chunk_count -
			(h.chunk_count / CHUNK_BUF_SIZE) * CHUNK_BUF_SIZE;

	// Initialize DMA for chunk buffers
	DMA_init((uint32_t) ARCHIVE_ADDR, (uint32_t) chunk_buf1);

	// Start with the leftovers
	if (chunks_left) {
		DMA_start(chunks_left * (h.chunk_size >> 2));

		// Read & check the leftover chunks
		DMA_wait_and_prepare((uint32_t) chunk_buf1);
		if (!check_chunk_buf(chunk_buf1, chunks_left))
			return false;
	}

	// Read the first full chunk buffer
	DMA_start(CHUNK_BUF_SIZE * (h.chunk_size >> 2));

	// Check the footer meanwhile
#ifdef CHECK_FOOTER
	uint64_t *footer = (uint32_t) ARCHIVE_ADDR +
			h.chunk_size * h.chunk_count;

	if (*footer != FOOTER_PATTERN) // 8B footer (should be 8 0xAB bytes)
		return false;
#endif

	DMA_wait_and_prepare((uint32_t) chunk_buf2); // Next read goes to buf 2
	curr_buf = chunk_buf1; // Current buffer is buf 1

	// Proceed checking the remaining chunks
	// It is important that `i' starts from 1
	for (i = 1; i < (h.chunk_count / CHUNK_BUF_SIZE); ++i) {
		DMA_start(CHUNK_BUF_SIZE * (h.chunk_size >> 2));
		if (!check_chunk_buf(curr_buf, CHUNK_BUF_SIZE))
			return false;
		DMA_wait_and_prepare((uint32_t) curr_buf);
		curr_buf = (curr_buf == chunk_buf1) ? chunk_buf2 : chunk_buf1;
	}

	return check_chunk_buf(curr_buf, CHUNK_BUF_SIZE); // Check the last chunk
}

/**
 * Performs the archive verification.
 *
 * @return true if the archive is correct, false otherwise
 */
bool run_verification() {
	h = *((header_t *) HEADER_ADDR);

#ifdef CHECK_HEADER
	if (h.preamble != HEADER_PATTERN)
		return false;
#endif

	CHUNK_BUF_SIZE = MEMORY_AVAILABLE / h.chunk_size;
	return validate_chunks(); // Validate.
}
