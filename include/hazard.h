/*
 * hazard.h — Hazard detection interface
 *
 * In this 3-stage pipeline (IF → ID → EX), the EX stage performs
 * ALU computation, memory access, AND register writeback in one stage.
 * Stages execute in reverse order (EX → ID → IF), so all data hazards
 * are resolved via implicit forwarding — no stalls are ever needed.
 *
 * Only control hazards (BEQZ taken, JR) require flushing.
 */

#ifndef HAZARD_H
#define HAZARD_H

#include "../include/types.h"

/*
 * Check for data hazards between EX and ID stages.
 * Always returns 0 (no stall) because reverse execution order
 * provides implicit forwarding for all instructions, including LB.
 *
 * Parameters:
 *   ex_valid  — whether EX had a real instruction this cycle
 *   ex_opcode — the opcode that just executed in EX
 *   ex_r1     — destination register of the EX instruction
 *   if_id     — pointer to the IF/ID pipeline register (next instruction)
 *
 * Returns:
 *   Always 0 — no stalls needed in this 3-stage pipeline.
 */
int hazard_check_stall(int ex_valid, Opcode ex_opcode, uint8_t ex_r1,
                       const IF_ID_Reg *if_id);

#endif /* HAZARD_H */
