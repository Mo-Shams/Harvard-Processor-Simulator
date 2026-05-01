/*
 * memory.c — Memory subsystem implementation
 *
 * Instruction Memory: 1024 × 16-bit words
 * Data Memory:        2048 × 8-bit bytes
 */

#include <stdio.h>
#include <string.h>
#include "../include/memory.h"

/* ─── Internal State ────────────────────────────────── */

static uint16_t instr_mem[INSTR_MEM_SIZE];  /* 1024 × 16-bit instruction words */
static int8_t   data_mem[DATA_MEM_SIZE];    /* 2048 × 8-bit data bytes         */
static int      instr_count = 0;            /* Number of loaded instructions    */

/* ─── Initialization ────────────────────────────────── */

void memory_init(void) {
    memset(instr_mem, 0, sizeof(instr_mem));
    memset(data_mem,  0, sizeof(data_mem));
    instr_count = 0;
}

/* ─── Instruction Memory ────────────────────────────── */

void instr_mem_write(uint16_t address, uint16_t value) {
    if (address >= INSTR_MEM_SIZE) {
        fprintf(stderr, "ERROR: Instruction memory write out of bounds "
                "(address=%u, max=%d)\n", address, INSTR_MEM_SIZE - 1);
        return;
    }
    instr_mem[address] = value;
}

uint16_t instr_mem_read(uint16_t address) {
    if (address >= INSTR_MEM_SIZE) {
        fprintf(stderr, "ERROR: Instruction memory read out of bounds "
                "(address=%u, max=%d)\n", address, INSTR_MEM_SIZE - 1);
        return 0;
    }
    return instr_mem[address];
}

int instr_mem_get_count(void) {
    return instr_count;
}

void instr_mem_set_count(int count) {
    instr_count = count;
}

/* ─── Data Memory ───────────────────────────────────── */

void data_mem_write(uint16_t address, int8_t value) {
    if (address >= DATA_MEM_SIZE) {
        fprintf(stderr, "ERROR: Data memory write out of bounds "
                "(address=%u, max=%d)\n", address, DATA_MEM_SIZE - 1);
        return;
    }
    data_mem[address] = value;
}

int8_t data_mem_read(uint16_t address) {
    if (address >= DATA_MEM_SIZE) {
        fprintf(stderr, "ERROR: Data memory read out of bounds "
                "(address=%u, max=%d)\n", address, DATA_MEM_SIZE - 1);
        return 0;
    }
    return data_mem[address];
}

/* ─── Direct Array Access ───────────────────────────── */

const uint16_t *instr_mem_get_array(void) {
    return instr_mem;
}

const int8_t *data_mem_get_array(void) {
    return data_mem;
}
