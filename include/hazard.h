/*
 * hazard.h — Hazard detection and resolution interface
 *
 * Detects RAW data hazards and control hazards;
 * decides whether to forward, stall, or flush.
 */

#ifndef HAZARD_H
#define HAZARD_H

#include "../include/types.h"

/*
 * Check for a load-use data hazard between the instruction
 * that just executed in EX and the instruction about to be decoded in ID.
 *
 * Returns 1 if a stall is needed (LW in EX, dependent instruction in ID).
 * Returns 0 otherwise (forwarding handles it, or no hazard).
 *
 * Parameters:
 *   ex_valid  — whether the EX stage had a valid instruction
 *   ex_opcode — opcode of the instruction that was in EX
 *   ex_r1     — destination register of the EX instruction
 *   if_id     — the IF/ID pipeline register (instruction waiting in ID)
 */
int hazard_check_stall(int ex_valid, Opcode ex_opcode, uint8_t ex_r1,
                       const IF_ID_Reg *if_id);

#endif /* HAZARD_H */
