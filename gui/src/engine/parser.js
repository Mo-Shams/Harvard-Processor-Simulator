import { OPCODE_NAMES, Opcode, InstrFormat } from './types.js';
import { instr_mem_write, instr_mem_set_count, INSTR_MEM_SIZE } from './memory.js';

function mnemonic_to_opcode(mnemonic) {
  let upper = mnemonic.toUpperCase();
  for (let i = 0; i < OPCODE_NAMES.length; i++) {
    if (OPCODE_NAMES[i] === upper) {
      return i;
    }
  }
  return -1;
}

function parse_register(token) {
  if (!token) return -1;
  let upper = token.toUpperCase();
  if (upper.startsWith('R')) {
    let val = parseInt(upper.substring(1), 10);
    if (!isNaN(val) && val >= 0 && val < 64) {
      return val;
    }
  }
  return -1;
}

function parse_immediate(token) {
  if (!token) return { err: true, val: 0 };
  let val = parseInt(token, 10);
  if (isNaN(val)) return { err: true, val: 0 };
  return { err: false, val: val };
}

function is_i_type(opcode) {
  switch (opcode) {
    case Opcode.LDI:
    case Opcode.BEQZ:
    case Opcode.SAL:
    case Opcode.SAR:
    case Opcode.LB:
    case Opcode.SB:
      return true;
    default:
      return false;
  }
}

function encode_instruction(opcode, r1, r2_or_imm, imm_flag) {
  let encoded = 0;
  encoded |= ((opcode & 0x0F) << 12);
  encoded |= ((r1 & 0x3F) << 6);
  encoded |= (r2_or_imm & 0x3F);
  return encoded;
}

export function parser_load_text(text) {
  let lines = text.split('\n');
  let address = 0;
  let errors = [];

  for (let i = 0; i < lines.length; i++) {
    let line = lines[i].trim();
    if (!line || line.startsWith('#') || line.startsWith(';')) continue;

    if (address >= INSTR_MEM_SIZE) {
      errors.push(`Line ${i + 1}: Too many instructions (max ${INSTR_MEM_SIZE})`);
      break;
    }

    // Replace commas with spaces, split by spaces
    let tokens = line.replace(/,/g, ' ').split(/\s+/).filter(t => t.length > 0);
    if (tokens.length < 3) {
      errors.push(`Line ${i + 1}: Malformed instruction '${line}'`);
      continue;
    }

    let opcode = mnemonic_to_opcode(tokens[0]);
    if (opcode < 0) {
      errors.push(`Line ${i + 1}: Unknown mnemonic '${tokens[0]}'`);
      continue;
    }

    let r1 = parse_register(tokens[1]);
    if (r1 < 0) {
      errors.push(`Line ${i + 1}: Invalid register '${tokens[1]}'`);
      continue;
    }

    let r2_or_imm = 0;
    let imm_flag = is_i_type(opcode);

    if (imm_flag) {
      let res = parse_immediate(tokens[2]);
      if (res.err) {
        errors.push(`Line ${i + 1}: Invalid immediate '${tokens[2]}'`);
        continue;
      }
      r2_or_imm = res.val;
    } else {
      r2_or_imm = parse_register(tokens[2]);
      if (r2_or_imm < 0) {
        errors.push(`Line ${i + 1}: Invalid register '${tokens[2]}'`);
        continue;
      }
    }

    let encoded = encode_instruction(opcode, r1, r2_or_imm, imm_flag);
    instr_mem_write(address, encoded);
    address++;
  }

  instr_mem_set_count(address);
  return { count: address, errors };
}
