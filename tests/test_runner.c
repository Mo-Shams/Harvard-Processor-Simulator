/*
 * test_runner.c — Comprehensive Automated Test Suite
 *
 * Package 4: Double McHarvard with cheese circular shifts
 *
 * Test Suites:
 *   1. ISA Instruction Decoding & Assembly Parsing
 *   2. General-Purpose Register File & Status Register (SREG)
 *   3. Harvard Memory Subsystem (Instruction & Data Memory)
 *   4. ALU Operations for all 12 ISA Instructions
 *   5. SREG Status Flags (C, V, N, S, Z) & Arithmetic Overflow
 *   6. 3-Stage Pipeline Forwarding & RAW Data Hazards
 *   7. Pipeline Control Hazards & Branch/Jump Flushing
 *   8. Complex End-to-End Programs & Architecture Edge Cases
 */

#include "../include/alu.h"
#include "../include/decoder.h"
#include "../include/memory.h"
#include "../include/parser.h"
#include "../include/pipeline.h"
#include "../include/registers.h"
#include "../include/types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ─── Test Framework Macros ─────────────────────────── */

static int total_passed = 0;
static int total_failed = 0;

#define COLOR_GREEN "\033[32m"
#define COLOR_RED   "\033[31m"
#define COLOR_CYAN  "\033[36m"
#define COLOR_RESET "\033[0m"

#define TEST_CHECK(condition, desc)                                            \
  do {                                                                         \
    if (condition) {                                                           \
      total_passed++;                                                          \
      printf("  " COLOR_GREEN "[PASS]" COLOR_RESET " %s\n", desc);             \
    } else {                                                                   \
      total_failed++;                                                          \
      printf("  " COLOR_RED   "[FAIL]" COLOR_RESET " %s\n", desc);             \
      fprintf(stderr, "         Assertion failed: (%s) at line %d\n",         \
              #condition, __LINE__);                                           \
    }                                                                          \
  } while (0)

static void reset_processor(void) {
  memory_init();
  registers_init();
}

/* ══════════════════════════════════════════════════════════════════
 * SUITE 1: ISA Decoding & Assembly Parser
 * ══════════════════════════════════════════════════════════════════ */

static void test_suite_decoder_and_parser(void) {
  printf("\n" COLOR_CYAN "=== Suite 1: ISA Decoding & Assembly Parser ===" COLOR_RESET "\n");

  /* 1. All 12 opcodes decoded accurately */
  const Opcode opcodes[] = {
      OP_ADD, OP_SUB, OP_MUL, OP_LDI, OP_BEQZ, OP_AND,
      OP_OR,  OP_JR,  OP_SAL, OP_SAR, OP_LB,   OP_SB};

  for (size_t i = 0; i < sizeof(opcodes) / sizeof(opcodes[0]); i++) {
    uint16_t raw = (uint16_t)(opcodes[i] << 12);
    DecodedInstr d = decode_instruction(raw);
    char msg[64];
    snprintf(msg, sizeof(msg), "Decode opcode %-4s (value %d)",
             OPCODE_NAMES[opcodes[i]], (int)opcodes[i]);
    TEST_CHECK(d.opcode == opcodes[i], msg);
  }

  /* 2. Format classification */
  TEST_CHECK(get_format(OP_ADD) == FMT_R_TYPE, "Format classification: ADD is R-Type");
  TEST_CHECK(get_format(OP_SUB) == FMT_R_TYPE, "Format classification: SUB is R-Type");
  TEST_CHECK(get_format(OP_MUL) == FMT_R_TYPE, "Format classification: MUL is R-Type");
  TEST_CHECK(get_format(OP_AND) == FMT_R_TYPE, "Format classification: AND is R-Type");
  TEST_CHECK(get_format(OP_OR)  == FMT_R_TYPE, "Format classification: OR is R-Type");
  TEST_CHECK(get_format(OP_JR)  == FMT_R_TYPE, "Format classification: JR is R-Type");

  TEST_CHECK(get_format(OP_LDI)  == FMT_I_TYPE, "Format classification: LDI is I-Type");
  TEST_CHECK(get_format(OP_BEQZ) == FMT_I_TYPE, "Format classification: BEQZ is I-Type");
  TEST_CHECK(get_format(OP_SAL)  == FMT_I_TYPE, "Format classification: SAL is I-Type");
  TEST_CHECK(get_format(OP_SAR)  == FMT_I_TYPE, "Format classification: SAR is I-Type");
  TEST_CHECK(get_format(OP_LB)   == FMT_I_TYPE, "Format classification: LB is I-Type");
  TEST_CHECK(get_format(OP_SB)   == FMT_I_TYPE, "Format classification: SB is I-Type");

  /* 3. Register field extraction (R1=45, R2=18) */
  /* [Opcode=0(4b)] [R1=45(6b)=0b101101] [R2=18(6b)=0b010010] */
  uint16_t raw_r = (uint16_t)((0 << 12) | (45 << 6) | 18);
  DecodedInstr d_r = decode_instruction(raw_r);
  TEST_CHECK(d_r.r1 == 45 && d_r.r2 == 18, "Register fields decoded correctly (R1=45, R2=18)");

  /* 4. Immediate sign-extension: 6-bit positive (+15, +31) */
  uint16_t raw_pos15 = (uint16_t)((OP_LDI << 12) | (1 << 6) | 15);
  DecodedInstr d_pos15 = decode_instruction(raw_pos15);
  TEST_CHECK(d_pos15.imm == 15, "Sign extension: 6-bit positive immediate (+15)");

  uint16_t raw_pos31 = (uint16_t)((OP_LDI << 12) | (1 << 6) | 31);
  DecodedInstr d_pos31 = decode_instruction(raw_pos31);
  TEST_CHECK(d_pos31.imm == 31, "Sign extension: 6-bit maximum positive immediate (+31)");

  /* 5. Immediate sign-extension: 6-bit negative (-1, -32) */
  /* -1 in 6-bit 2's complement is 0b111111 = 0x3F = 63 */
  uint16_t raw_neg1 = (uint16_t)((OP_LDI << 12) | (2 << 6) | 0x3F);
  DecodedInstr d_neg1 = decode_instruction(raw_neg1);
  TEST_CHECK(d_neg1.imm == -1, "Sign extension: 6-bit negative immediate (-1)");

  /* -32 in 6-bit 2's complement is 0b100000 = 0x20 = 32 */
  uint16_t raw_neg32 = (uint16_t)((OP_LDI << 12) | (2 << 6) | 0x20);
  DecodedInstr d_neg32 = decode_instruction(raw_neg32);
  TEST_CHECK(d_neg32.imm == -32, "Sign extension: 6-bit minimum negative immediate (-32)");

  /* 6. In-memory string parser */
  reset_processor();
  const char *test_asm = 
      "# Test assembly program\n"
      "ldi r1, 10\n"
      "LDI R2, -5   ; comment after instruction\n"
      "add r1, r2\n";
  int loaded = parser_load_string(test_asm);
  TEST_CHECK(loaded == 3, "Parser parses string with comments, mixed case, and commas");
  TEST_CHECK(instr_mem_get_count() == 3, "Instruction memory count updated after string parsing");

  /* Verify first decoded instruction from string */
  DecodedInstr d_str0 = decode_instruction(instr_mem_read(0));
  TEST_CHECK(d_str0.opcode == OP_LDI && d_str0.r1 == 1 && d_str0.imm == 10,
             "Parsed instruction 0 matches encoded representation");
}

/* ══════════════════════════════════════════════════════════════════
 * SUITE 2: Register File & Status Register (SREG)
 * ══════════════════════════════════════════════════════════════════ */

static void test_suite_registers_and_sreg(void) {
  printf("\n" COLOR_CYAN "=== Suite 2: Register File & Status Register (SREG) ===" COLOR_RESET "\n");
  reset_processor();

  /* 1. R0 is fully writable in Package 4 */
  gpr_write(0, 42);
  TEST_CHECK(gpr_read(0) == 42, "R0 is readable and writable (Package 4 has no zero register)");

  /* 2. Highest register R63 */
  gpr_write(63, -99);
  TEST_CHECK(gpr_read(63) == -99, "R63 is readable and writable with signed 8-bit values");

  /* 3. Register value boundaries (-128, +127, 0) */
  gpr_write(10, 127);
  gpr_write(11, -128);
  TEST_CHECK(gpr_read(10) == 127, "Register preserves signed maximum positive (+127)");
  TEST_CHECK(gpr_read(11) == -128, "Register preserves signed minimum negative (-128)");

  /* 4. Register isolation: writing to one GPR does not contaminate neighbors */
  gpr_write(5, 77);
  gpr_write(6, 88);
  TEST_CHECK(gpr_read(5) == 77 && gpr_read(6) == 88, "Registers maintain data isolation");

  /* 5. SREG bits 7:5 masking */
  sreg_write(0xFF);
  TEST_CHECK(sreg_read() == SREG_VALID_MASK,
             "SREG mask enforcement: bits 7:5 cleared when writing 0xFF (result 0x1F)");

  sreg_write(0xE0);
  TEST_CHECK(sreg_read() == 0,
             "SREG mask enforcement: writing upper bits 0xE0 results in 0x00");

  /* 6. Individual flag set and clear */
  sreg_write(0);
  sreg_set_flag(SREG_C_BIT, 1);
  sreg_set_flag(SREG_V_BIT, 1);
  sreg_set_flag(SREG_N_BIT, 1);
  sreg_set_flag(SREG_S_BIT, 1);
  sreg_set_flag(SREG_Z_BIT, 1);
  TEST_CHECK(sreg_get_flag(SREG_C_BIT) == 1 && sreg_get_flag(SREG_V_BIT) == 1 &&
             sreg_get_flag(SREG_N_BIT) == 1 && sreg_get_flag(SREG_S_BIT) == 1 &&
             sreg_get_flag(SREG_Z_BIT) == 1,
             "All 5 SREG condition flags (C, V, N, S, Z) set and read successfully");

  sreg_set_flag(SREG_Z_BIT, 0);
  sreg_set_flag(SREG_C_BIT, 0);
  TEST_CHECK(sreg_get_flag(SREG_Z_BIT) == 0 && sreg_get_flag(SREG_C_BIT) == 0 &&
             sreg_get_flag(SREG_V_BIT) == 1,
             "Individual SREG condition flags cleared without disturbing others");

  /* 7. Program Counter (PC) operations */
  pc_write(0);
  TEST_CHECK(pc_read() == 0, "PC initializes to 0");
  pc_increment();
  TEST_CHECK(pc_read() == 1, "PC increments by 1");
  pc_write(512);
  TEST_CHECK(pc_read() == 512, "PC direct write and read (16-bit address)");

  /* 8. Reset restores zero state */
  reset_processor();
  TEST_CHECK(gpr_read(0) == 0 && gpr_read(63) == 0 && sreg_read() == 0 && pc_read() == 0,
             "Registers reset clears all GPRs, SREG, and PC");
}

/* ══════════════════════════════════════════════════════════════════
 * SUITE 3: Harvard Memory Subsystem
 * ══════════════════════════════════════════════════════════════════ */

static void test_suite_memory_subsystem(void) {
  printf("\n" COLOR_CYAN "=== Suite 3: Harvard Memory Subsystem ===" COLOR_RESET "\n");
  reset_processor();

  /* 1. Data memory boundary addresses (0 and 2047) */
  data_mem_write(0, 120);
  TEST_CHECK(data_mem_read(0) == 120, "Data memory byte read/write at base address 0");

  data_mem_write(2047, -75);
  TEST_CHECK(data_mem_read(2047) == -75, "Data memory byte read/write at upper boundary address 2047");

  /* 2. Data memory signed representation */
  data_mem_write(100, -128);
  data_mem_write(101, 127);
  TEST_CHECK(data_mem_read(100) == -128 && data_mem_read(101) == 127,
             "Data memory preserves 8-bit signed range (-128 to +127)");

  /* 3. Data memory isolation */
  data_mem_write(50, 42);
  data_mem_write(51, 99);
  TEST_CHECK(data_mem_read(50) == 42 && data_mem_read(51) == 99,
             "Adjacent data memory locations maintain data isolation");

  /* 4. Instruction memory boundary addresses (0 and 1023) */
  instr_mem_write(0, 0x1234);
  TEST_CHECK(instr_mem_read(0) == 0x1234, "Instruction memory word read/write at address 0");

  instr_mem_write(1023, 0xABCD);
  TEST_CHECK(instr_mem_read(1023) == 0xABCD, "Instruction memory word read/write at upper boundary address 1023");

  /* 5. Instruction count tracking */
  instr_mem_set_count(55);
  TEST_CHECK(instr_mem_get_count() == 55, "Instruction count tracker records correct count");

  /* 6. Out of bounds memory safety */
  TEST_CHECK(data_mem_read(2048) == 0, "Out-of-bounds data memory read returns 0 safely");
  TEST_CHECK(instr_mem_read(1024) == 0, "Out-of-bounds instruction memory read returns 0 safely");

  /* 7. Memory reset clears all memories */
  memory_init();
  TEST_CHECK(data_mem_read(0) == 0 && data_mem_read(2047) == 0 &&
             instr_mem_read(0) == 0 && instr_mem_read(1023) == 0 &&
             instr_mem_get_count() == 0,
             "Memory reset zeroes entire data memory and instruction memory");
}

/* ══════════════════════════════════════════════════════════════════
 * SUITE 4: ALU Operations for all 12 ISA Instructions
 * ══════════════════════════════════════════════════════════════════ */

static void test_suite_alu_operations(void) {
  printf("\n" COLOR_CYAN "=== Suite 4: ALU Operations for all 12 ISA Instructions ===" COLOR_RESET "\n");
  reset_processor();

  /* OP_ADD (Opcode 0) */
  TEST_CHECK(alu_execute(OP_ADD, 15, 25, 0) == 40, "ALU ADD: positive + positive (15 + 25 = 40)");
  TEST_CHECK(alu_execute(OP_ADD, -10, -20, 0) == -30, "ALU ADD: negative + negative (-10 + -20 = -30)");
  TEST_CHECK(alu_execute(OP_ADD, -15, 30, 0) == 15, "ALU ADD: mixed signs (-15 + 30 = 15)");
  TEST_CHECK(alu_execute(OP_ADD, 0, 42, 0) == 42, "ALU ADD: zero operand (0 + 42 = 42)");

  /* OP_SUB (Opcode 1) */
  TEST_CHECK(alu_execute(OP_SUB, 50, 20, 0) == 30, "ALU SUB: positive - positive (50 - 20 = 30)");
  TEST_CHECK(alu_execute(OP_SUB, 10, 25, 0) == -15, "ALU SUB: result negative (10 - 25 = -15)");
  TEST_CHECK(alu_execute(OP_SUB, 10, -5, 0) == 15, "ALU SUB: subtracting negative (10 - (-5) = 15)");
  TEST_CHECK(alu_execute(OP_SUB, 33, 33, 0) == 0, "ALU SUB: self subtraction (33 - 33 = 0)");

  /* OP_MUL (Opcode 2) */
  TEST_CHECK(alu_execute(OP_MUL, 6, 7, 0) == 42, "ALU MUL: positive * positive (6 * 7 = 42)");
  TEST_CHECK(alu_execute(OP_MUL, 5, -4, 0) == -20, "ALU MUL: positive * negative (5 * -4 = -20)");
  TEST_CHECK(alu_execute(OP_MUL, -6, -7, 0) == 42, "ALU MUL: negative * negative (-6 * -7 = 42)");
  TEST_CHECK(alu_execute(OP_MUL, 99, 0, 0) == 0, "ALU MUL: multiplication by zero (99 * 0 = 0)");
  TEST_CHECK(alu_execute(OP_MUL, 20, 10, 0) == (int8_t)-56,
             "ALU MUL: 8-bit signed wrap (20 * 10 = 200 = -56 mod 256)");

  /* OP_LDI (Opcode 3) */
  TEST_CHECK(alu_execute(OP_LDI, 0, 0, 31) == 31, "ALU LDI: maximum positive immediate (+31)");
  TEST_CHECK(alu_execute(OP_LDI, 0, 0, -32) == -32, "ALU LDI: minimum negative immediate (-32)");
  TEST_CHECK(alu_execute(OP_LDI, 0, 0, 0) == 0, "ALU LDI: zero immediate");

  /* OP_BEQZ (Opcode 4) */
  TEST_CHECK(alu_execute(OP_BEQZ, 0, 0, 5) == 1, "ALU BEQZ: branch condition evaluated TRUE when R1 == 0");
  TEST_CHECK(alu_execute(OP_BEQZ, 42, 0, 5) == 0, "ALU BEQZ: branch condition evaluated FALSE when R1 > 0");
  TEST_CHECK(alu_execute(OP_BEQZ, -7, 0, 5) == 0, "ALU BEQZ: branch condition evaluated FALSE when R1 < 0");

  /* OP_AND (Opcode 5) */
  TEST_CHECK(alu_execute(OP_AND, (int8_t)0xAA, (int8_t)0x55, 0) == 0,
             "ALU AND: alternating bitmasks (0xAA & 0x55 = 0)");
  TEST_CHECK(alu_execute(OP_AND, (int8_t)0xFF, (int8_t)0x3C, 0) == 0x3C,
             "ALU AND: mask with all-ones (0xFF & 0x3C = 0x3C)");

  /* OP_OR (Opcode 6) */
  TEST_CHECK(alu_execute(OP_OR, (int8_t)0xAA, (int8_t)0x55, 0) == (int8_t)0xFF,
             "ALU OR: complementary bitmasks (0xAA | 0x55 = 0xFF)");
  TEST_CHECK(alu_execute(OP_OR, (int8_t)0x3C, 0, 0) == 0x3C,
             "ALU OR: mask with zero (0x3C | 0x00 = 0x3C)");

  /* OP_JR (Opcode 7) */
  TEST_CHECK(alu_execute(OP_JR, 1, 44, 0) == 0, "ALU JR: JR opcode executes cleanly in ALU");

  /* OP_SAL (Opcode 8) */
  TEST_CHECK(alu_execute(OP_SAL, 10, 0, 0) == 10, "ALU SAL: shift left by 0 leaves value unchanged");
  TEST_CHECK(alu_execute(OP_SAL, 5, 0, 2) == 20, "ALU SAL: arithmetic shift left (5 << 2 = 20)");
  TEST_CHECK(alu_execute(OP_SAL, 64, 0, 1) == -128, "ALU SAL: shift left into sign bit (64 << 1 = -128)");

  /* OP_SAR (Opcode 9) */
  TEST_CHECK(alu_execute(OP_SAR, 40, 0, 0) == 40, "ALU SAR: shift right by 0 leaves value unchanged");
  TEST_CHECK(alu_execute(OP_SAR, 40, 0, 2) == 10, "ALU SAR: arithmetic shift right positive (40 >> 2 = 10)");
  TEST_CHECK(alu_execute(OP_SAR, -16, 0, 2) == -4,
             "ALU SAR: arithmetic shift right preserves negative sign (-16 >> 2 = -4)");
  TEST_CHECK(alu_execute(OP_SAR, -1, 0, 3) == -1,
             "ALU SAR: arithmetic shift right -1 preserves all ones (-1 >> 3 = -1)");

  /* OP_LB and OP_SB (Opcodes 10 and 11) */
  TEST_CHECK(alu_execute(OP_LB, 0, 0, 15) == 15, "ALU LB: immediate address resolved to 15");
  TEST_CHECK(alu_execute(OP_SB, 99, 0, 25) == 25, "ALU SB: immediate address resolved to 25");
}

/* ══════════════════════════════════════════════════════════════════
 * SUITE 5: SREG Status Flags (C, V, N, S, Z) & Arithmetic Overflow
 * ══════════════════════════════════════════════════════════════════ */

static void test_suite_status_flags_and_overflow(void) {
  printf("\n" COLOR_CYAN "=== Suite 5: SREG Status Flags (C, V, N, S, Z) & Arithmetic Overflow ===" COLOR_RESET "\n");
  reset_processor();

  /* 1. Carry flag (C): unsigned carry out on ADD */
  alu_execute(OP_ADD, (int8_t)200, (int8_t)60, 0); /* 200 + 60 = 260 >= 256 */
  TEST_CHECK(sreg_get_flag(SREG_C_BIT) == 1,
             "Carry Flag (C): set on unsigned addition overflow (200 + 60 >= 256)");

  alu_execute(OP_ADD, 50, 50, 0); /* 50 + 50 = 100 < 256 */
  TEST_CHECK(sreg_get_flag(SREG_C_BIT) == 0,
             "Carry Flag (C): cleared when unsigned addition does not carry (50 + 50 < 256)");

  /* 2. Overflow flag (V): ADD signed overflow */
  alu_execute(OP_ADD, 100, 50, 0); /* 100 + 50 = 150 -> -106 */
  TEST_CHECK(sreg_get_flag(SREG_V_BIT) == 1,
             "Overflow Flag (V): set when positive + positive yields negative (100 + 50 = -106)");

  alu_execute(OP_ADD, -100, -50, 0); /* -100 + -50 = -150 -> +106 */
  TEST_CHECK(sreg_get_flag(SREG_V_BIT) == 1,
             "Overflow Flag (V): set when negative + negative yields positive (-100 + -50 = +106)");

  alu_execute(OP_ADD, -128, -128, 0); /* -128 + -128 = -256 = 0 */
  TEST_CHECK(sreg_get_flag(SREG_V_BIT) == 1,
             "Overflow Flag (V): set when -128 + -128 wraps to 0");

  alu_execute(OP_ADD, 50, -30, 0); /* 50 + -30 = 20 */
  TEST_CHECK(sreg_get_flag(SREG_V_BIT) == 0,
             "Overflow Flag (V): cleared for operands with different signs (never overflows)");

  /* 3. Overflow flag (V): SUB signed overflow */
  alu_execute(OP_SUB, 100, -50, 0); /* 100 - (-50) = 150 -> -106 */
  TEST_CHECK(sreg_get_flag(SREG_V_BIT) == 1,
             "Overflow Flag (V): set when positive - negative yields negative (100 - (-50) = -106)");

  alu_execute(OP_SUB, -100, 50, 0); /* -100 - 50 = -150 -> +106 */
  TEST_CHECK(sreg_get_flag(SREG_V_BIT) == 1,
             "Overflow Flag (V): set when negative - positive yields positive (-100 - 50 = +106)");

  alu_execute(OP_SUB, 0, -128, 0); /* 0 - (-128) = +128 -> -128 */
  TEST_CHECK(sreg_get_flag(SREG_V_BIT) == 1,
             "Overflow Flag (V): set when 0 - (-128) overflows signed 8-bit range");

  alu_execute(OP_SUB, 50, 20, 0); /* 50 - 20 = 30 */
  TEST_CHECK(sreg_get_flag(SREG_V_BIT) == 0,
             "Overflow Flag (V): cleared when subtraction does not overflow (50 - 20 = 30)");

  /* 4. Negative flag (N) */
  alu_execute(OP_ADD, 10, -25, 0); /* -15 */
  TEST_CHECK(sreg_get_flag(SREG_N_BIT) == 1, "Negative Flag (N): set when result is negative (-15)");

  alu_execute(OP_ADD, 10, 25, 0); /* 35 */
  TEST_CHECK(sreg_get_flag(SREG_N_BIT) == 0, "Negative Flag (N): cleared when result is positive (35)");

  /* 5. Sign flag (S = N ^ V) */
  /* Case 1: N=1, V=0 -> S=1 */
  alu_execute(OP_ADD, 10, -20, 0); /* result=-10, no overflow -> N=1, V=0 => S=1 */
  TEST_CHECK(sreg_get_flag(SREG_S_BIT) == 1 && sreg_get_flag(SREG_V_BIT) == 0,
             "Sign Flag (S): S = N ^ V = 1 when N=1 and V=0");

  /* Case 2: N=1, V=1 -> S=0 */
  alu_execute(OP_ADD, 100, 50, 0); /* result=-106 (N=1), overflowed (V=1) => S=0 */
  TEST_CHECK(sreg_get_flag(SREG_S_BIT) == 0 && sreg_get_flag(SREG_N_BIT) == 1 &&
             sreg_get_flag(SREG_V_BIT) == 1,
             "Sign Flag (S): S = N ^ V = 0 when N=1 and V=1 (expected positive result)");

  /* 6. Zero flag (Z) */
  alu_execute(OP_SUB, 42, 42, 0);
  TEST_CHECK(sreg_get_flag(SREG_Z_BIT) == 1, "Zero Flag (Z): set when result is zero (42 - 42 = 0)");

  alu_execute(OP_MUL, 42, 0, 0);
  TEST_CHECK(sreg_get_flag(SREG_Z_BIT) == 1, "Zero Flag (Z): set on multiplication by zero");

  alu_execute(OP_AND, (int8_t)0x0F, (int8_t)0xF0, 0);
  TEST_CHECK(sreg_get_flag(SREG_Z_BIT) == 1, "Zero Flag (Z): set on bitwise AND producing zero");

  alu_execute(OP_ADD, 1, 2, 0);
  TEST_CHECK(sreg_get_flag(SREG_Z_BIT) == 0, "Zero Flag (Z): cleared when result is non-zero (1 + 2 = 3)");

  /* 7. Flag preservation across non-flag modifying instructions */
  sreg_write(0x1F); /* Set all 5 flags */
  alu_execute(OP_LDI, 0, 0, 10);
  TEST_CHECK(sreg_read() == 0x1F, "Flag Preservation: LDI does not alter any SREG condition flags");

  alu_execute(OP_LB, 0, 0, 0);
  TEST_CHECK(sreg_read() == 0x1F, "Flag Preservation: LB does not alter any SREG condition flags");

  alu_execute(OP_SB, 0, 0, 0);
  TEST_CHECK(sreg_read() == 0x1F, "Flag Preservation: SB does not alter any SREG condition flags");

  alu_execute(OP_BEQZ, 0, 0, 0);
  TEST_CHECK(sreg_read() == 0x1F, "Flag Preservation: BEQZ does not alter any SREG condition flags");

  alu_execute(OP_JR, 0, 0, 0);
  TEST_CHECK(sreg_read() == 0x1F, "Flag Preservation: JR does not alter any SREG condition flags");
}

/* ══════════════════════════════════════════════════════════════════
 * SUITE 6: 3-Stage Pipeline Forwarding & RAW Data Hazards
 * ══════════════════════════════════════════════════════════════════ */

static void test_suite_pipeline_forwarding_and_raw(void) {
  printf("\n" COLOR_CYAN "=== Suite 6: 3-Stage Pipeline Forwarding & RAW Data Hazards ===" COLOR_RESET "\n");
  pipeline_set_verbose(0);
  parser_set_verbose(0);

  /* 1. Back-to-back RAW hazard (EX writes register before ID reads it in same cycle) */
  reset_processor();
  parser_load_string(
      "LDI R1 10\n"
      "ADD R1 R1\n");
  pipeline_run();
  PipelineMetrics m = pipeline_get_metrics();
  TEST_CHECK(gpr_read(1) == 20, "Pipeline Forwarding: back-to-back RAW hazard resolves to R1 = 20");
  TEST_CHECK(m.data_hazard_stalls == 0, "Pipeline Forwarding: no stalls inserted for same-cycle forwarding");

  /* 2. Three-instruction RAW dependency chain */
  reset_processor();
  parser_load_string(
      "LDI R1 5\n"
      "ADD R2 R1\n"
      "ADD R3 R2\n");
  pipeline_run();
  TEST_CHECK(gpr_read(2) == 5 && gpr_read(3) == 5,
             "Pipeline Forwarding: 3-instruction RAW chain (R1 -> R2 -> R3) propagates correctly");

  /* 3. Deep RAW dependency chain: 5 consecutive dependent instructions */
  reset_processor();
  parser_load_string(
      "LDI R1 2\n"
      "ADD R1 R1\n"  /* 4 */
      "ADD R1 R1\n"  /* 8 */
      "ADD R1 R1\n"  /* 16 */
      "ADD R1 R1\n"  /* 32 */
      "ADD R1 R1\n"); /* 64 */
  pipeline_run();
  TEST_CHECK(gpr_read(1) == 64,
             "Pipeline Forwarding: deep RAW dependency chain doubles R1 correctly to 64");

  /* 4. Forwarding into second operand (R2) of R-type instruction */
  reset_processor();
  parser_load_string(
      "LDI R1 15\n"
      "LDI R2 25\n"
      "SUB R3 R2\n"); /* R3 = 0 - 25 = -25 */
  pipeline_run();
  TEST_CHECK(gpr_read(3) == -25,
             "Pipeline Forwarding: forwarded value read into R2 operand of SUB");

  /* 5. Load-use RAW dependency: LB followed immediately by dependent ADD */
  reset_processor();
  data_mem_write(4, 33);
  parser_load_string(
      "LB R1 4\n"
      "ADD R2 R1\n");
  pipeline_run();
  TEST_CHECK(gpr_read(1) == 33 && gpr_read(2) == 33,
             "Pipeline Forwarding: Load-use RAW hazard (LB -> ADD) forwards without stalls");

  /* 6. Store-then-load memory dependency round-trip in pipeline */
  reset_processor();
  parser_load_string(
      "LDI R1 25\n"
      "SB R1 12\n"
      "LB R2 12\n");
  pipeline_run();
  TEST_CHECK(data_mem_read(12) == 25 && gpr_read(2) == 25,
             "Pipeline Memory: Store-Byte immediately followed by Load-Byte completes round trip");

  /* 7. Pipeline timing for N independent instructions: cycles = 3 + (N - 1) */
  reset_processor();
  parser_load_string(
      "LDI R1 1\n"
      "LDI R2 2\n"
      "LDI R3 3\n"
      "LDI R4 4\n"
      "LDI R5 5\n");
  pipeline_run();
  m = pipeline_get_metrics();
  TEST_CHECK(m.total_cycles == 7, "Pipeline Timing: 5 independent instructions complete in exactly 7 cycles (3 + 4)");
  TEST_CHECK(m.retired_instructions == 5, "Pipeline Metrics: retired instructions count equals 5");

  /* 8. Single instruction pipeline timing: exactly 3 cycles */
  reset_processor();
  parser_load_string("LDI R1 30\n");
  pipeline_run();
  m = pipeline_get_metrics();
  TEST_CHECK(m.total_cycles == 3 && m.retired_instructions == 1,
             "Pipeline Timing: single instruction requires exactly 3 clock cycles (IF, ID, EX)");
}

/* ══════════════════════════════════════════════════════════════════
 * SUITE 7: Pipeline Control Hazards & Branch/Jump Flushing
 * ══════════════════════════════════════════════════════════════════ */

static void test_suite_pipeline_control_hazards(void) {
  printf("\n" COLOR_CYAN "=== Suite 7: Pipeline Control Hazards & Branch/Jump Flushing ===" COLOR_RESET "\n");
  pipeline_set_verbose(0);
  parser_set_verbose(0);

  /* 1. Taken BEQZ flushes younger instructions */
  reset_processor();
  parser_load_string(
      "LDI R1 0\n"       /* [0] R1 = 0 */
      "BEQZ R1 2\n"      /* [1] Taken! Target = 1 + 1 + 2 = 4 */
      "LDI R2 15\n"      /* [2] Flushed (in ID) */
      "LDI R3 15\n"      /* [3] Flushed (in IF) */
      "LDI R4 25\n");    /* [4] Target instruction */
  pipeline_run();
  PipelineMetrics m = pipeline_get_metrics();
  TEST_CHECK(gpr_read(2) == 0, "Control Hazard: flushed instruction in ID does NOT write register R2");
  TEST_CHECK(gpr_read(3) == 0, "Control Hazard: flushed instruction in IF does NOT write register R3");
  TEST_CHECK(gpr_read(4) == 25, "Control Hazard: branch target instruction at PC=4 executes and writes R4 = 25");
  TEST_CHECK(m.branch_flushes == 1, "Control Hazard: metrics record exactly 1 branch flush");

  /* 2. Flushed instruction does not alter data memory */
  reset_processor();
  parser_load_string(
      "LDI R1 0\n"
      "BEQZ R1 1\n"
      "SB R1 5\n"         /* Flushed store instruction */
      "LDI R2 10\n");
  data_mem_write(5, 27);
  pipeline_run();
  TEST_CHECK(data_mem_read(5) == 27, "Control Hazard: flushed SB instruction does NOT modify data memory");

  /* 3. Untaken BEQZ continues sequentially without flushing */
  reset_processor();
  parser_load_string(
      "LDI R1 10\n"      /* R1 != 0 */
      "BEQZ R1 2\n"      /* Branch NOT taken */
      "LDI R2 20\n"      /* Executes sequentially */
      "LDI R3 30\n");     /* Executes sequentially */
  pipeline_run();
  m = pipeline_get_metrics();
  TEST_CHECK(gpr_read(2) == 20 && gpr_read(3) == 30,
             "Control Hazard: untaken BEQZ executes sequential instructions without skipping");
  TEST_CHECK(m.branch_flushes == 0, "Control Hazard: untaken BEQZ produces 0 branch flushes");

  /* 4. Unconditional JR (Jump Register) computes PC = R1 || R2 and flushes pipeline */
  reset_processor();
  /* PC target = 0x00 || 0x05 = 5 */
  parser_load_string(
      "LDI R1 0\n"       /* [0] High byte = 0 */
      "LDI R2 5\n"       /* [1] Low byte = 5 */
      "JR R1 R2\n"       /* [2] Jump to PC=5, flushes [3] and [4] */
      "LDI R3 10\n"      /* [3] Flushed in ID */
      "LDI R4 10\n"      /* [4] Flushed in IF */
      "LDI R5 25\n");    /* [5] Target PC=5: executes */
  pipeline_run();
  m = pipeline_get_metrics();
  TEST_CHECK(gpr_read(3) == 0, "Jump Register: instruction in ID following JR is flushed (R3 = 0)");
  TEST_CHECK(gpr_read(4) == 0, "Jump Register: instruction in IF following JR is flushed (R4 = 0)");
  TEST_CHECK(gpr_read(5) == 25, "Jump Register: reaches target instruction PC=5 and writes R5 = 25");
  TEST_CHECK(m.branch_flushes == 1, "Jump Register: metrics record exactly 1 branch flush for JR");
}

/* ══════════════════════════════════════════════════════════════════
 * SUITE 8: Complex End-to-End Programs & Architecture Edge Cases
 * ══════════════════════════════════════════════════════════════════ */

static void test_suite_complex_programs_and_edge_cases(void) {
  printf("\n" COLOR_CYAN "=== Suite 8: Complex End-to-End Programs & Architecture Edge Cases ===" COLOR_RESET "\n");
  pipeline_set_verbose(0);
  parser_set_verbose(0);

  /* 1. Countdown Loop program: sum numbers 4 down to 1 into R2 */
  reset_processor();
  int count = parser_load("programs/test_loop.txt");
  TEST_CHECK(count == 8, "Program Loader: successfully parsed 'programs/test_loop.txt' (8 instructions)");
  pipeline_run();
  PipelineMetrics m = pipeline_get_metrics();
  TEST_CHECK(gpr_read(1) == 0, "Countdown Loop: counter R1 decrements to 0");
  TEST_CHECK(gpr_read(2) == 10, "Countdown Loop: accumulated sum R2 = 4 + 3 + 2 + 1 = 10");
  TEST_CHECK(gpr_read(4) == 20, "Countdown Loop: loop exit reached (R4 = 20)");
  TEST_CHECK(m.branch_flushes == 4, "Countdown Loop: exactly 4 taken branch flushes during loop execution");

  /* 2. Memory array write, read, and accumulated sum */
  reset_processor();
  count = parser_load("programs/test_memory_array.txt");
  TEST_CHECK(count == 11, "Program Loader: successfully parsed 'programs/test_memory_array.txt' (11 instructions)");
  pipeline_run();
  TEST_CHECK(data_mem_read(0) == 10 && data_mem_read(1) == 20 && data_mem_read(2) == 30,
             "Memory Array: data memory correctly initialized with array elements [10, 20, 30]");
  TEST_CHECK(gpr_read(10) == 60, "Memory Array: loaded and accumulated sum R10 = 10 + 20 + 30 = 60");

  /* 3. Comprehensive test file validation */
  reset_processor();
  count = parser_load("programs/test_comprehensive.txt");
  TEST_CHECK(count == 23, "Program Loader: successfully parsed 'programs/test_comprehensive.txt' (23 instructions)");
  pipeline_run();
  TEST_CHECK(gpr_read(1) == 10 && gpr_read(2) == 3, "Comprehensive Test: base values loaded (R1=10, R2=3)");
  TEST_CHECK(gpr_read(3) == 13, "Comprehensive Test: ADD result R3 = 10 + 3 = 13");
  TEST_CHECK(gpr_read(4) == 7,  "Comprehensive Test: SUB result R4 = 10 - 3 = 7");
  TEST_CHECK(gpr_read(5) == 30, "Comprehensive Test: MUL result R5 = 10 * 3 = 30");
  TEST_CHECK(gpr_read(6) == 2,  "Comprehensive Test: AND result R6 = 10 & 3 = 2");
  TEST_CHECK(gpr_read(7) == 11, "Comprehensive Test: OR result R7 = 10 | 3 = 11");
  TEST_CHECK(gpr_read(8) == 40, "Comprehensive Test: SAL result R8 = 10 << 2 = 40");
  TEST_CHECK(gpr_read(9) == 5,  "Comprehensive Test: SAR result R9 = 10 >> 1 = 5");
  TEST_CHECK(gpr_read(11) == 30, "Comprehensive Test: LB loaded value R11 = DataMem[5] = 30");
  TEST_CHECK(gpr_read(12) == 0,  "Comprehensive Test: flushed instruction verified unexecuted (R12 = 0)");
  TEST_CHECK(gpr_read(20) == 7,  "Comprehensive Test: untaken branch fallthrough executed (R20 = 7)");

  /* 4. Multi-register boundary stress test (R0, R15, R31, R47, R63) */
  reset_processor();
  parser_load_string(
      "LDI R0 1\n"
      "LDI R15 15\n"
      "LDI R31 31\n"
      "LDI R47 -17\n"
      "LDI R63 -32\n"
      "ADD R0 R15\n"
      "ADD R31 R47\n");
  pipeline_run();
  TEST_CHECK(gpr_read(0) == 16, "Register Stress Test: R0 computed result 1 + 15 = 16");
  TEST_CHECK(gpr_read(31) == 14, "Register Stress Test: R31 computed result 31 + (-17) = 14");
  TEST_CHECK(gpr_read(63) == -32, "Register Stress Test: R63 holds boundary immediate -32");

  /* 5. Branch jumping over another branch */
  reset_processor();
  parser_load_string(
      "LDI R1 0\n"       /* [0] */
      "BEQZ R1 2\n"      /* [1] Target = 1 + 1 + 2 = 4 */
      "BEQZ R1 5\n"      /* [2] Flushed branch */
      "LDI R2 10\n"      /* [3] Flushed */
      "LDI R3 20\n");    /* [4] Target reached */
  pipeline_run();
  TEST_CHECK(gpr_read(2) == 0 && gpr_read(3) == 20,
             "Branch Combinations: branch successfully skips over intervening branch instruction");

  /* 7. Existing program file validation: programs/test1.txt */
  reset_processor();
  count = parser_load("programs/test1.txt");
  TEST_CHECK(count == 11, "Program Loader: successfully parsed 'programs/test1.txt' (11 instructions)");
  pipeline_run();
  m = pipeline_get_metrics();
  TEST_CHECK(gpr_read(1) == 6 && gpr_read(3) == 6 && data_mem_read(0) == 6,
             "Program test1.txt: registers R1=6, R3=6 and DataMem[0]=6 verified");
  TEST_CHECK(m.total_cycles == 13 && m.retired_instructions == 11,
             "Program test1.txt: completed in exactly 13 cycles with 11 retired instructions");

  /* 8. Existing program file validation: programs/test_hazard_flush.txt */
  reset_processor();
  count = parser_load("programs/test_hazard_flush.txt");
  TEST_CHECK(count == 7, "Program Loader: successfully parsed 'programs/test_hazard_flush.txt' (7 instructions)");
  pipeline_run();
  m = pipeline_get_metrics();
  TEST_CHECK(gpr_read(3) == 5 && gpr_read(4) == 0 && gpr_read(5) == 7,
             "Program test_hazard_flush.txt: branch taken (R3=5, flushed R4=0, target R5=7)");
  TEST_CHECK(m.branch_flushes == 1 && m.total_cycles == 10,
             "Program test_hazard_flush.txt: exactly 1 branch flush and 10 total cycles");

  /* 9. Existing program file validation: programs/test_jr.txt */
  reset_processor();
  count = parser_load("programs/test_jr.txt");
  TEST_CHECK(count == 6, "Program Loader: successfully parsed 'programs/test_jr.txt' (6 instructions)");
  pipeline_run();
  m = pipeline_get_metrics();
  TEST_CHECK(gpr_read(3) == 0 && gpr_read(4) == 0 && gpr_read(5) == 7,
             "Program test_jr.txt: younger instructions flushed (R3=0, R4=0) and target executed (R5=7)");
  TEST_CHECK(m.branch_flushes == 1 && m.retired_instructions == 4,
             "Program test_jr.txt: exactly 1 branch flush and 4 retired instructions");
}

/* ══════════════════════════════════════════════════════════════════
 * Main Runner Entry Point
 * ══════════════════════════════════════════════════════════════════ */

int main(void) {
  printf("======================================================================\n");
  printf(" Harvard Processor Simulator (Package 4) - Automated Test Suite\n");
  printf(" Architecture: 16-bit Harvard, 3-Stage Pipeline (IF -> ID -> EX)\n");
  printf("======================================================================\n");

  test_suite_decoder_and_parser();
  test_suite_registers_and_sreg();
  test_suite_memory_subsystem();
  test_suite_alu_operations();
  test_suite_status_flags_and_overflow();
  test_suite_pipeline_forwarding_and_raw();
  test_suite_pipeline_control_hazards();
  test_suite_complex_programs_and_edge_cases();

  printf("\n======================================================================\n");
  if (total_failed == 0) {
    printf(COLOR_GREEN "Test Summary: %d passed, %d failed (Total: %d)\n" COLOR_RESET,
           total_passed, total_failed, total_passed + total_failed);
    printf(COLOR_GREEN "Status: ALL TESTS PASSED\n" COLOR_RESET);
  } else {
    printf(COLOR_RED "Test Summary: %d passed, %d failed (Total: %d)\n" COLOR_RESET,
           total_passed, total_failed, total_passed + total_failed);
    printf(COLOR_RED "Status: SOME TESTS FAILED\n" COLOR_RESET);
  }
  printf("======================================================================\n");

  return total_failed != 0;
}
