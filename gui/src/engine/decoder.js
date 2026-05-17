import { Opcode, InstrFormat } from './types.js';

export function get_format(opcode) {
  switch (opcode) {
    case Opcode.ADD:
    case Opcode.SUB:
    case Opcode.MUL:
    case Opcode.AND:
    case Opcode.OR:
    case Opcode.JR:
      return InstrFormat.FMT_R_TYPE;
    case Opcode.LDI:
    case Opcode.BEQZ:
    case Opcode.SAL:
    case Opcode.SAR:
    case Opcode.LW:
    case Opcode.SW:
      return InstrFormat.FMT_I_TYPE;
    default:
      return InstrFormat.FMT_R_TYPE;
  }
}

function sign_extend_6to8(val6) {
  val6 &= 0x3F;
  if (val6 & 0x20) {
    return (val6 | 0xC0) << 24 >> 24; // Ensure proper JS 8-bit sign extension
  }
  return val6 << 24 >> 24;
}

export function decode_instruction(raw) {
  let opcode = (raw >> 12) & 0x0F;
  let r1 = (raw >> 6) & 0x3F;
  let r2 = raw & 0x3F;
  let imm = sign_extend_6to8(raw & 0x3F);
  let format = get_format(opcode);

  return {
    raw,
    opcode,
    r1,
    r2,
    imm,
    format
  };
}
