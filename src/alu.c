/*
 * alu.c — ALU operations implementation
 *
 * Performs the actual computation for each instruction type
 * and updates SREG flags according to the specification:
 *
 *   C (Carry):    ADD only
 *   V (Overflow): ADD, SUB
 *   N (Negative): ADD, SUB, MUL, AND, OR, SAL, SAR
 *   S (Sign):     ADD, SUB
 *   Z (Zero):     ADD, SUB, MUL, AND, OR, SAL, SAR
 */

#include "../include/alu.h"
#include "../include/registers.h"

/* ─── Flag Update Helper ────────────────────────────── */

/*
 * Update SREG flags based on the opcode, operands, and result.
 * Only the flags listed for each instruction are modified;
 * all other flags are left unchanged.
 */
static void update_flags(Opcode opcode, int8_t r1_val, int8_t r2_val,
                         int8_t result) {
  int N, Z, V, C, S;

  switch (opcode) {

  case OP_ADD:
    /* C (Carry): check if unsigned addition overflows past 8 bits */
    C = (((int)(uint8_t)r1_val + (int)(uint8_t)r2_val) & 0x100) ? 1 : 0;
    sreg_set_flag(SREG_C_BIT, C);

    /* V (Overflow): same-sign operands produce opposite-sign result */
    V = ((r1_val > 0 && r2_val > 0 && result < 0) ||
         (r1_val < 0 && r2_val < 0 && result > 0))
            ? 1
            : 0;
    sreg_set_flag(SREG_V_BIT, V);

    /* N (Negative): MSB of result */
    N = (result & 0x80) ? 1 : 0;
    sreg_set_flag(SREG_N_BIT, N);

    /* S (Sign): N XOR V */
    S = N ^ V;
    sreg_set_flag(SREG_S_BIT, S);

    /* Z (Zero) */
    Z = (result == 0) ? 1 : 0;
    sreg_set_flag(SREG_Z_BIT, Z);
    break;

  case OP_SUB:
    /* V (Overflow): different-sign operands, result sign matches subtrahend */
    V = ((r1_val >= 0 && r2_val < 0 && result < 0) ||
         (r1_val < 0 && r2_val >= 0 && result > 0))
            ? 1
            : 0;
    sreg_set_flag(SREG_V_BIT, V);

    /* N (Negative) */
    N = (result & 0x80) ? 1 : 0;
    sreg_set_flag(SREG_N_BIT, N);

    /* S (Sign): N XOR V */
    S = N ^ V;
    sreg_set_flag(SREG_S_BIT, S);

    /* Z (Zero) */
    Z = (result == 0) ? 1 : 0;
    sreg_set_flag(SREG_Z_BIT, Z);
    break;

  case OP_MUL:
  case OP_AND:
  case OP_OR:
  case OP_SAL:
  case OP_SAR:
    /* These instructions only update N and Z */
    N = (result & 0x80) ? 1 : 0;
    sreg_set_flag(SREG_N_BIT, N);

    Z = (result == 0) ? 1 : 0;
    sreg_set_flag(SREG_Z_BIT, Z);
    break;

  default:
    /* LDI, BEQZ, JR, LB, SB — do NOT update any flags */
    break;
  }
}

/* ─── Main ALU Function ─────────────────────────────── */

int8_t alu_execute(Opcode opcode, int8_t r1_val, int8_t r2_val, int8_t imm) {
  int8_t result = 0;

  switch (opcode) {
  case OP_ADD:
    result = (int8_t)(r1_val + r2_val);
    break;

  case OP_SUB:
    result = (int8_t)(r1_val - r2_val);
    break;

  case OP_MUL:
    result = (int8_t)(r1_val * r2_val);
    break;

  case OP_LDI:
    result = imm;
    break;

  case OP_BEQZ:
    /* Return whether R1 == 0 (used by pipeline for branch decision) */
    result = (r1_val == 0) ? 1 : 0;
    break;

  case OP_AND:
    result = (int8_t)(r1_val & r2_val);
    break;

  case OP_OR:
    result = (int8_t)(r1_val | r2_val);
    break;

  case OP_JR:
    /* No ALU computation — pipeline handles PC = R1||R2 directly */
    result = 0;
    break;

  case OP_SAL:
    /* Arithmetic shift left (IMM is always positive for shifts) */
    result = (int8_t)(r1_val << imm);
    break;

  case OP_SAR:
    /* Arithmetic shift right — sign-extending
     * C guarantees arithmetic right shift on signed types */
    result = (int8_t)(r1_val >> imm);
    break;

  case OP_LB:
  case OP_SB:
    /* Memory address is just the sign-extended immediate */
    result = imm;
    break;

  default:
    result = 0;
    break;
  }

  /* Update SREG flags for applicable instructions */
  update_flags(opcode, r1_val, r2_val, result);

  return result;
}