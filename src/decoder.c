/*
 * decoder.c — Instruction decoder implementation
 *
 * Extracts all fields from a 16-bit binary instruction.
 * Decodes into BOTH R-type and I-type formats simultaneously,
 * since the format isn't known until the opcode is checked.
 *
 * Instruction layout (16 bits):
 *   Bits 15–12: OPCODE (4 bits)
 *   Bits 11–6:  R1     (6 bits)
 *   Bits 5–0:   R2     (6 bits, R-type) / IMM (6 bits, I-type)
 */

#include "../include/decoder.h"

/* ─── Format Lookup ─────────────────────────────────── */

InstrFormat get_format(Opcode opcode) {
    switch (opcode) {
        /* R-Type: two register operands */
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_AND:
        case OP_OR:
        case OP_JR:
            return FMT_R_TYPE;

        /* I-Type: one register + one immediate */
        case OP_LDI:
        case OP_BEQZ:
        case OP_SAL:
        case OP_SAR:
        case OP_LB:
        case OP_SB:
            return FMT_I_TYPE;

        default:
            /* Unknown opcode — default to R-type */
            return FMT_R_TYPE;
    }
}

/* ─── Sign Extension ────────────────────────────────── */

/*
 * Sign-extend a 6-bit value to 8 bits (int8_t).
 *
 * If bit 5 (the sign bit of a 6-bit 2's complement number) is 1,
 * the value is negative and we set bits 7:6 to 1.
 *
 * 6-bit range: -32 to +31
 */
static int8_t sign_extend_6to8(uint8_t val6) {
    /* Mask to 6 bits */
    val6 &= 0x3F;

    /* Check bit 5 (sign bit of 6-bit value) */
    if (val6 & 0x20) {
        /* Negative: set upper 2 bits to 1 → 0xC0 | val6 */
        return (int8_t)(val6 | 0xC0);
    }
    /* Positive: upper bits stay 0 */
    return (int8_t)val6;
}

/* ─── Main Decoder Function ─────────────────────────── */

DecodedInstr decode_instruction(uint16_t raw) {
    DecodedInstr decoded;

    decoded.raw = raw;

    /* Extract opcode: bits 15–12 */
    decoded.opcode = (Opcode)((raw >> 12) & 0x0F);

    /* Extract R1: bits 11–6 (register index, 0–63) */
    decoded.r1 = (uint8_t)((raw >> 6) & 0x3F);

    /* Extract R2: bits 5–0 (register index for R-type) */
    decoded.r2 = (uint8_t)(raw & 0x3F);

    /* Extract IMM: bits 5–0 (signed immediate for I-type)
     * Sign-extend from 6 bits to 8 bits */
    decoded.imm = sign_extend_6to8((uint8_t)(raw & 0x3F));

    /* Determine instruction format from opcode */
    decoded.format = get_format(decoded.opcode);

    return decoded;
}
