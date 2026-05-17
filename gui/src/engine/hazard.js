import { Opcode, InstrFormat } from './types.js';
import { decode_instruction } from './decoder.js';

function writes_to_r1(opcode) {
  switch (opcode) {
    case Opcode.ADD:
    case Opcode.SUB:
    case Opcode.MUL:
    case Opcode.LDI:
    case Opcode.AND:
    case Opcode.OR:
    case Opcode.SAL:
    case Opcode.SAR:
    case Opcode.LW:
      return true;
    default:
      return false;
  }
}

export function hazard_check_stall(ex_valid, ex_opcode, ex_r1, if_id) {
  if (!ex_valid || !if_id || !if_id.valid) return false;

  if (ex_opcode !== Opcode.LW) return false;

  if (!writes_to_r1(ex_opcode)) return false;

  let next = decode_instruction(if_id.instruction);

  let reads_r1_as_source = true;
  if (next.opcode === Opcode.LDI) reads_r1_as_source = false;

  let reads_r2 = (next.format === InstrFormat.FMT_R_TYPE);

  if (reads_r1_as_source && next.r1 === ex_r1) return true;
  if (reads_r2 && next.r2 === ex_r1) return true;

  return false;
}
