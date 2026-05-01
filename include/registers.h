/*
 * registers.h — Register file interface
 *
 * 64 General-Purpose Registers (R0–R63): 8-bit each (int8_t)
 * 1 Status Register (SREG):              8-bit (uint8_t), bits 7:5 always 0
 * 1 Program Counter (PC):                16-bit (uint16_t)
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include "../include/types.h"

/* ─── Initialization ────────────────────────────────── */

/* Zero-out all GPRs, SREG, and PC */
void registers_init(void);

/* ─── General-Purpose Registers (R0–R63) ────────────── */

/*
 * Read the value of GPR at the given index.
 * index must be in [0, NUM_GPRS-1].
 */
int8_t gpr_read(uint8_t index);

/*
 * Write a value to the GPR at the given index.
 * index must be in [0, NUM_GPRS-1].
 * Note: Unlike Packages 1 & 2, there is NO hardwired zero register
 * in Package 4 — all 64 GPRs are fully writable.
 */
void gpr_write(uint8_t index, int8_t value);

/* Return pointer to the GPR array (for printing full register dump) */
const int8_t *gpr_get_array(void);

/* ─── Status Register (SREG) ────────────────────────── */

/* Read the full SREG byte */
uint8_t sreg_read(void);

/* Write the full SREG byte (bits 7:5 are forced to 0) */
void sreg_write(uint8_t value);

/* Read a single SREG flag (returns 0 or 1) */
int sreg_get_flag(int bit_position);

/*
 * Set a single SREG flag to the given value (0 or 1).
 * bit_position should be one of SREG_C_BIT, SREG_V_BIT, etc.
 */
void sreg_set_flag(int bit_position, int value);

/* ─── Program Counter (PC) ──────────────────────────── */

/* Read the current PC value */
uint16_t pc_read(void);

/* Write a new PC value */
void pc_write(uint16_t value);

/* Increment PC by 1 (used during fetch) */
void pc_increment(void);

#endif /* REGISTERS_H */
