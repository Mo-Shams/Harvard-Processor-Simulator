/*
 * hazard.c — Hazard detection & resolution
 *
 * In this 3-stage pipeline (IF → ID → EX), the EX stage performs
 * ALU computation, memory access, AND register writeback all in one stage.
 * Stages execute in reverse order (EX → ID → IF) each cycle, so by the
 * time ID reads the register file, EX has already written its result.
 *
 * This means ALL data hazards are resolved via implicit forwarding
 * (reverse execution order). No stalls are ever needed — including
 * for LB (Load Byte), since the loaded value is written to the
 * register file before ID reads it in the same cycle.
 *
 * Only control hazards (BEQZ taken, JR) require flushing, which
 * is handled in pipeline.c.
 */

#include <stddef.h>

#include "../include/hazard.h"

int hazard_check_stall(int ex_valid, Opcode ex_opcode, uint8_t ex_r1,
                       const IF_ID_Reg *if_id) {
  /* No stalls needed in a 3-stage pipeline with reverse execution order.
   * All forwarding is implicit: EX writes the register file before
   * ID reads it in the same cycle. */
  (void)ex_valid;
  (void)ex_opcode;
  (void)ex_r1;
  (void)if_id;
  return 0;
}
