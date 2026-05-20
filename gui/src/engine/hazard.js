import { Opcode, InstrFormat } from './types.js';
import { decode_instruction } from './decoder.js';

/*
 * In this 3-stage pipeline (IF → ID → EX), the EX stage performs
 * ALU computation, memory access, AND register writeback all in one stage.
 * Stages execute in reverse order (EX → ID → IF), so by the time ID
 * reads the register file, EX has already written its result.
 *
 * All data hazards are resolved via implicit forwarding. No stalls needed.
 */
export function hazard_check_stall(ex_valid, ex_opcode, ex_r1, if_id) {
  return false;
}
