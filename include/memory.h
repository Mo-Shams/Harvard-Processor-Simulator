/*
 * memory.h — Memory subsystem interface
 *
 * Instruction Memory: 1024 × 16-bit words (word-addressable)
 * Data Memory:        2048 × 8-bit bytes  (byte-addressable)
 */

#ifndef MEMORY_H
#define MEMORY_H

#include "../include/types.h"

/* ─── Initialization ────────────────────────────────── */

/* Zero-out both instruction and data memory arrays */
void memory_init(void);

/* ─── Instruction Memory ────────────────────────────── */

/*
 * Write a 16-bit instruction into instruction memory at the given address.
 * address must be in [0, INSTR_MEM_SIZE-1].
 */
void instr_mem_write(uint16_t address, uint16_t value);

/*
 * Read a 16-bit instruction from instruction memory at the given address.
 * address must be in [0, INSTR_MEM_SIZE-1].
 */
uint16_t instr_mem_read(uint16_t address);

/* Return the total number of instructions loaded (set during parsing) */
int instr_mem_get_count(void);

/* Set the number of instructions loaded */
void instr_mem_set_count(int count);

/* ─── Data Memory ───────────────────────────────────── */

/*
 * Write an 8-bit value into data memory at the given address.
 * address must be in [0, DATA_MEM_SIZE-1].
 */
void data_mem_write(uint16_t address, int8_t value);

/*
 * Read an 8-bit value from data memory at the given address.
 * address must be in [0, DATA_MEM_SIZE-1].
 */
int8_t data_mem_read(uint16_t address);

/* ─── Direct Array Access (for printing full memory dump) ── */

/* Return pointer to the instruction memory array */
const uint16_t *instr_mem_get_array(void);

/* Return pointer to the data memory array */
const int8_t *data_mem_get_array(void);

#endif /* MEMORY_H */
