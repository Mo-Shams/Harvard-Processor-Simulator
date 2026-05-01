/*
 * decoder.h — Instruction decoder interface
 *
 * Extracts all fields from a 16-bit binary instruction:
 *   - Opcode:  bits 15–12
 *   - R1:      bits 11–6
 *   - R2:      bits 5–0  (R-type interpretation)
 *   - IMM:     bits 5–0  (I-type interpretation, sign-extended to 8 bits)
 *
 * The decoder populates BOTH R-type and I-type fields simultaneously
 * because the instruction type isn't known until the opcode is checked.
 * The format field indicates which interpretation is correct.
 */

#ifndef DECODER_H
#define DECODER_H

#include "../include/types.h"

/*
 * Decode a raw 16-bit instruction into a DecodedInstr struct.
 *
 * All fields (opcode, r1, r2, imm, format) are populated.
 * The 6-bit immediate is sign-extended to 8 bits for I-type instructions.
 *
 * Parameters:
 *   raw  — the 16-bit binary instruction from instruction memory
 *
 * Returns:
 *   A fully populated DecodedInstr struct.
 */
DecodedInstr decode_instruction(uint16_t raw);

/*
 * Determine the instruction format (R-type or I-type) from the opcode.
 *
 * R-type: ADD, SUB, MUL, AND, OR, JR  (opcodes 0,1,2,5,6,7)
 * I-type: LDI, BEQZ, SAL, SAR, LW, SW (opcodes 3,4,8,9,10,11)
 */
InstrFormat get_format(Opcode opcode);

#endif /* DECODER_H */
