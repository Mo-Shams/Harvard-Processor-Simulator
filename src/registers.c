/*
 * registers.c — Register file implementation
 *
 * 64 GPRs (R0–R63):  8-bit signed (int8_t)
 * 1 SREG:            8-bit unsigned, bits 7:5 always 0
 * 1 PC:              16-bit unsigned
 *
 * Note: Unlike Packages 1 & 2, there is NO hardwired zero register
 * in Package 4. All 64 GPRs (R0–R63) are fully writable.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "../include/registers.h"

/* ─── Internal State ────────────────────────────────── */

static int8_t   gprs[NUM_GPRS];   /* R0–R63, 8-bit signed each     */
static uint8_t  sreg;             /* Status register (bits 7:5 = 0) */
static uint16_t pc;               /* Program counter, 16-bit        */

/* ─── Initialization ────────────────────────────────── */

void registers_init(void) {
    memset(gprs, 0, sizeof(gprs));
    sreg = 0;
    pc   = 0;
}

/* ─── General-Purpose Registers (R0–R63) ────────────── */

int8_t gpr_read(uint8_t index) {
    if (index >= NUM_GPRS) {
        fprintf(stderr, "ERROR: GPR read out of bounds (R%u, max=R%d)\n",
                index, NUM_GPRS - 1);
        return 0;
    }
    return gprs[index];
}

void gpr_write(uint8_t index, int8_t value) {
    if (index >= NUM_GPRS) {
        fprintf(stderr, "ERROR: GPR write out of bounds (R%u, max=R%d)\n",
                index, NUM_GPRS - 1);
        return;
    }
    gprs[index] = value;
}

const int8_t *gpr_get_array(void) {
    return gprs;
}

/* ─── Status Register (SREG) ────────────────────────── */

uint8_t sreg_read(void) {
    return sreg;
}

void sreg_write(uint8_t value) {
    /* Force bits 7:5 to zero — only lower 5 bits are valid flags */
    sreg = value & SREG_VALID_MASK;
}

int sreg_get_flag(int bit_position) {
    if (bit_position < 0 || bit_position > 4) {
        fprintf(stderr, "ERROR: Invalid SREG flag bit position (%d)\n",
                bit_position);
        return 0;
    }
    return (sreg >> bit_position) & 1;
}

void sreg_set_flag(int bit_position, int value) {
    if (bit_position < 0 || bit_position > 4) {
        fprintf(stderr, "ERROR: Invalid SREG flag bit position (%d)\n",
                bit_position);
        return;
    }
    if (value) {
        sreg |= (1 << bit_position);  /* Set bit */
    } else {
        sreg &= ~(1 << bit_position); /* Clear bit */
    }
    /* Ensure bits 7:5 remain 0 */
    sreg &= SREG_VALID_MASK;
}

/* ─── Program Counter (PC) ──────────────────────────── */

uint16_t pc_read(void) {
    return pc;
}

void pc_write(uint16_t value) {
    pc = value;
}

void pc_increment(void) {
    pc++;
}
