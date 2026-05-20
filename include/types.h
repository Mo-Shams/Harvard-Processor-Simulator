/*
 * types.h — Core data types, constants, and structures
 *
 * Package 4: Double McHarvard with cheese circular shifts
 * Harvard architecture, 3-stage pipeline (IF → ID → EX)
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

/* ─── Memory Constants ──────────────────────────────── */

#define INSTR_MEM_SIZE 1024 /* Instruction memory: 1024 × 16 bits */
#define DATA_MEM_SIZE 2048  /* Data memory:        2048 × 8 bits  */

/* ─── Register Constants ────────────────────────────── */

#define NUM_GPRS 64 /* General-purpose registers R0–R63   */

/* ─── SREG Flag Bit Positions ───────────────────────── */
/*
 * SREG layout (8 bits):
 *   Bit:  7  6  5  4  3  2  1  0
 *   Flag: 0  0  0  C  V  N  S  Z
 *
 * Bits 7:5 must always be 0.
 */
#define SREG_C_BIT 4 /* Carry flag    */
#define SREG_V_BIT 3 /* Overflow flag */
#define SREG_N_BIT 2 /* Negative flag */
#define SREG_S_BIT 1 /* Sign flag     */
#define SREG_Z_BIT 0 /* Zero flag     */

#define SREG_C_MASK (1 << SREG_C_BIT)
#define SREG_V_MASK (1 << SREG_V_BIT)
#define SREG_N_MASK (1 << SREG_N_BIT)
#define SREG_S_MASK (1 << SREG_S_BIT)
#define SREG_Z_MASK (1 << SREG_Z_BIT)

/* Mask for valid SREG bits (bits 4:0 only) */
#define SREG_VALID_MASK 0x1F

/* ─── Opcodes (0–11) ───────────────────────────────── */

typedef enum {
  OP_ADD = 0,
  OP_SUB = 1,
  OP_MUL = 2,
  OP_LDI = 3,
  OP_BEQZ = 4,
  OP_AND = 5,
  OP_OR = 6,
  OP_JR = 7,
  OP_SAL = 8,
  OP_SAR = 9,
  OP_LB = 10,
  OP_SB = 11
} Opcode;

/* ─── Instruction Format ────────────────────────────── */

typedef enum {
  FMT_R_TYPE, /* R-Type: OPCODE(4) | R1(6) | R2(6)  */
  FMT_I_TYPE  /* I-Type: OPCODE(4) | R1(6) | IMM(6) */
} InstrFormat;

/* ─── Decoded Instruction ───────────────────────────── */
/*
 * The decode stage extracts ALL fields from the 16-bit binary,
 * regardless of actual instruction format. Both R-type and I-type
 * fields are populated simultaneously.
 */
typedef struct {
  uint16_t raw;       /* Original 16-bit binary instruction          */
  Opcode opcode;      /* Bits 15–12: opcode (0–11)                   */
  uint8_t r1;         /* Bits 11–6:  R1 register index (0–63)        */
  uint8_t r2;         /* Bits 5–0:   R2 register index (R-type)      */
  int8_t imm;         /* Bits 5–0:   immediate, sign-extended to 8b  */
  InstrFormat format; /* Determined by opcode: R-type or I-type      */
} DecodedInstr;

/* ─── Pipeline Registers ────────────────────────────── */
/*
 * Pipeline registers sit between stages to pass data from one
 * stage to the next. They act like hardware latches — each stage
 * reads its input register and writes its output register.
 *
 * In software, we simulate latch behavior by executing stages
 * in reverse order (EX → ID → IF) so that each stage reads
 * its input before the previous stage overwrites it.
 */

/* IF/ID pipeline register — between Fetch and Decode */
typedef struct {
  uint16_t instruction; /* Raw 16-bit binary from InstructionMemory[PC] */
  uint16_t pc;          /* PC at fetch time (before increment)          */
  int valid;            /* 1 = contains a real instruction, 0 = bubble  */
} IF_ID_Reg;

/* ID/EX pipeline register — between Decode and Execute */
typedef struct {
  Opcode opcode; /* Decoded opcode                                */
  uint8_t r1;    /* R1 register index (destination / source)      */
  uint8_t r2;    /* R2 register index (source, R-type only)       */
  int8_t r1_val; /* Value read from RegisterFile[r1] during ID    */
  int8_t r2_val; /* Value read from RegisterFile[r2] during ID    */
  int8_t imm;    /* Sign-extended 6-bit immediate                 */
  uint16_t pc;   /* Fetch-time PC, carried forward for BEQZ       */
  int valid;     /* 1 = contains a real instruction, 0 = bubble   */
} ID_EX_Reg;

extern const char *OPCODE_NAMES[];

#endif /* TYPES_H */
