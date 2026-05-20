/*
 * alu.h — ALU operations interface
 *
 * Performs the actual computation for each instruction type.
 * (Implementation will be done in a future stage)
 */

#ifndef ALU_H
#define ALU_H

#include "../include/types.h"

/*
 * Execute an ALU operation based on the decoded instruction fields.
 * Handles: ADD, SUB, MUL, AND, OR, SAL, SAR, LDI.
 * Also handles memory addressing for LB/SB, branch evaluation for BEQZ,
 * and jump address computation for JR.
 *
 * Parameters:
 *   opcode   — the instruction opcode
 *   r1_val   — value of R1 (first operand)
 *   r2_val   — value of R2 (second operand, R-type)
 *   imm      — sign-extended immediate (I-type)
 *
 * Returns:
 *   The 8-bit result of the ALU operation (cast to int8_t).
 */
int8_t alu_execute(Opcode opcode, int8_t r1_val, int8_t r2_val, int8_t imm);
#endif
