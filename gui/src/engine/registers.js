import { NUM_GPRS, SREG_VALID_MASK } from './types.js';

let gprs = new Int8Array(NUM_GPRS);
let sreg = 0;
let pc = 0;

export function registers_init() {
  gprs.fill(0);
  sreg = 0;
  pc = 0;
}

export function gpr_read(index) {
  if (index >= 0 && index < NUM_GPRS) {
    return gprs[index];
  }
  return 0;
}

export function gpr_write(index, value) {
  if (index >= 0 && index < NUM_GPRS) {
    gprs[index] = value;
  }
}

export function gpr_get_array() {
  return new Int8Array(gprs);
}

export function sreg_read() {
  return sreg;
}

export function sreg_write(value) {
  sreg = value & SREG_VALID_MASK;
}

export function sreg_get_flag(bit_position) {
  return (sreg >> bit_position) & 1;
}

export function sreg_set_flag(bit_position, value) {
  if (value) {
    sreg |= (1 << bit_position);
  } else {
    sreg &= ~(1 << bit_position);
  }
  sreg &= SREG_VALID_MASK;
}

export function pc_read() {
  return pc;
}

export function pc_write(value) {
  pc = value;
}

export function pc_increment() {
  pc++;
}
