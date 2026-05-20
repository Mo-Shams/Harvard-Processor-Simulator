import { Opcode, SREG_C_BIT, SREG_V_BIT, SREG_N_BIT, SREG_S_BIT, SREG_Z_BIT } from './types.js';
import { sreg_set_flag } from './registers.js';

function update_flags(opcode, r1_val, r2_val, result) {
  let N, Z, V, C, S;

  // Convert to proper 8-bit signed values for comparisons
  let r1_signed = r1_val << 24 >> 24;
  let r2_signed = r2_val << 24 >> 24;
  let res_signed = result << 24 >> 24;

  switch (opcode) {
    case Opcode.ADD:
      C = (((r1_val & 0xFF) + (r2_val & 0xFF)) & 0x100) ? 1 : 0;
      sreg_set_flag(SREG_C_BIT, C);

      V = ((r1_signed >= 0 && r2_signed >= 0 && res_signed < 0) ||
           (r1_signed < 0 && r2_signed < 0 && res_signed >= 0)) ? 1 : 0;
      sreg_set_flag(SREG_V_BIT, V);

      N = (result & 0x80) ? 1 : 0;
      sreg_set_flag(SREG_N_BIT, N);

      S = N ^ V;
      sreg_set_flag(SREG_S_BIT, S);

      Z = (res_signed === 0) ? 1 : 0;
      sreg_set_flag(SREG_Z_BIT, Z);
      break;

    case Opcode.SUB:
      V = ((r1_signed >= 0 && r2_signed < 0 && res_signed < 0) ||
           (r1_signed < 0 && r2_signed >= 0 && res_signed > 0)) ? 1 : 0;
      sreg_set_flag(SREG_V_BIT, V);

      N = (result & 0x80) ? 1 : 0;
      sreg_set_flag(SREG_N_BIT, N);

      S = N ^ V;
      sreg_set_flag(SREG_S_BIT, S);

      Z = (res_signed === 0) ? 1 : 0;
      sreg_set_flag(SREG_Z_BIT, Z);
      break;

    case Opcode.MUL:
    case Opcode.AND:
    case Opcode.OR:
    case Opcode.SAL:
    case Opcode.SAR:
      N = (result & 0x80) ? 1 : 0;
      sreg_set_flag(SREG_N_BIT, N);

      Z = (res_signed === 0) ? 1 : 0;
      sreg_set_flag(SREG_Z_BIT, Z);
      break;

    default:
      break;
  }
}

export function alu_execute(opcode, r1_val, r2_val, imm) {
  let result = 0;
  
  // ensure values are 8-bit signed behavior
  let r1_signed = r1_val << 24 >> 24;
  let r2_signed = r2_val << 24 >> 24;
  let imm_signed = imm << 24 >> 24;

  switch (opcode) {
    case Opcode.ADD:
      result = (r1_signed + r2_signed);
      break;
    case Opcode.SUB:
      result = (r1_signed - r2_signed);
      break;
    case Opcode.MUL:
      result = (r1_signed * r2_signed);
      break;
    case Opcode.LDI:
      result = imm_signed;
      break;
    case Opcode.BEQZ:
      result = (r1_signed === 0) ? 1 : 0;
      break;
    case Opcode.AND:
      result = (r1_signed & r2_signed);
      break;
    case Opcode.OR:
      result = (r1_signed | r2_signed);
      break;
    case Opcode.JR:
      result = 0;
      break;
    case Opcode.SAL:
      result = (r1_signed << imm);
      break;
    case Opcode.SAR:
      result = (r1_signed >> imm);
      break;
    case Opcode.LB:
    case Opcode.SB:
      result = imm_signed;
      break;
    default:
      result = 0;
      break;
  }

  result = result << 24 >> 24; // Force to 8-bit signed

  update_flags(opcode, r1_val, r2_val, result);

  return result;
}
