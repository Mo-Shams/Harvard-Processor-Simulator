export const INSTR_MEM_SIZE = 1024;
export const DATA_MEM_SIZE = 2048;
export const NUM_GPRS = 64;

export const SREG_C_BIT = 4;
export const SREG_V_BIT = 3;
export const SREG_N_BIT = 2;
export const SREG_S_BIT = 1;
export const SREG_Z_BIT = 0;
export const SREG_VALID_MASK = 0x1F;

export const Opcode = {
  ADD: 0,
  SUB: 1,
  MUL: 2,
  LDI: 3,
  BEQZ: 4,
  AND: 5,
  OR: 6,
  JR: 7,
  SAL: 8,
  SAR: 9,
  LB: 10,
  SB: 11
};

export const InstrFormat = {
  FMT_R_TYPE: 0,
  FMT_I_TYPE: 1
};

export const OPCODE_NAMES = [
  "ADD", "SUB", "MUL", "LDI", "BEQZ", "AND",
  "OR", "JR", "SAL", "SAR", "LB", "SB"
];
