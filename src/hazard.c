/*
 * hazard.c — Hazard detection & resolution
 *
 * Detects load-use (RAW) data hazards between the EX and ID stages.
 * In our reverse-order simulation, non-LW forwarding is implicit
 * (EX writes to the register file before ID reads it).
 * LW requires an explicit 1-cycle stall because in real hardware
 * the loaded value isn't available for forwarding until after EX.
 */

#include "../include/hazard.h"
#include "../include/decoder.h"

/*
 * Check whether the given opcode writes to its R1 register.
 * Most instructions write to R1 except BEQZ, JR, and SW.
 */
static int writes_to_r1(Opcode opcode) {
  switch (opcode) {
  case OP_ADD:
  case OP_SUB:
  case OP_MUL:
  case OP_LDI:
  case OP_AND:
  case OP_OR:
  case OP_SAL:
  case OP_SAR:
  case OP_LW:
    return 1;
  default:
    return 0; /* BEQZ, JR, SW don't write to R1 */
  }
}

int hazard_check_stall(int ex_valid, Opcode ex_opcode, uint8_t ex_r1,
                       const IF_ID_Reg *if_id) {
  /* No hazard if EX had no valid instruction or ID has no instruction */
  if (!ex_valid || if_id == NULL || !if_id->valid)
    return 0;

  /* Only LW in EX can cause a stall (load-use hazard) */
  if (ex_opcode != OP_LW)
    return 0;

  /* EX must write to a register for there to be a hazard */
  if (!writes_to_r1(ex_opcode))
    return 0;

  /* Peek-decode the instruction waiting in ID to find its sources */
  DecodedInstr next = decode_instruction(if_id->instruction);

  /* Check if the ID instruction reads from EX's destination register */
  /* All instructions read R1 as a source (except LDI which just overwrites) */
  int reads_r1_as_source = 1;
  if (next.opcode == OP_LDI)
    reads_r1_as_source = 0; /* LDI doesn't read R1, just writes it */

  /* R-type instructions also read R2 */
  int reads_r2 = (next.format == FMT_R_TYPE);

  /* Check for dependency */
  if (reads_r1_as_source && next.r1 == ex_r1)
    return 1; /* stall needed */

  if (reads_r2 && next.r2 == ex_r1)
    return 1; /* stall needed */

  return 0; /* no stall */
}
