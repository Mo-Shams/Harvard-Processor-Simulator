import { INSTR_MEM_SIZE, DATA_MEM_SIZE } from './types.js';

let instr_mem = new Uint16Array(INSTR_MEM_SIZE);
let data_mem = new Int8Array(DATA_MEM_SIZE);
let instr_count = 0;

export function memory_init() {
  instr_mem.fill(0);
  data_mem.fill(0);
  instr_count = 0;
}

export function instr_mem_write(address, value) {
  if (address >= 0 && address < INSTR_MEM_SIZE) {
    instr_mem[address] = value;
  }
}

export function instr_mem_read(address) {
  if (address >= 0 && address < INSTR_MEM_SIZE) {
    return instr_mem[address];
  }
  return 0;
}

export function instr_mem_get_count() {
  return instr_count;
}

export function instr_mem_set_count(count) {
  instr_count = count;
}

export function data_mem_write(address, value) {
  if (address >= 0 && address < DATA_MEM_SIZE) {
    data_mem[address] = value;
  }
}

export function data_mem_read(address) {
  if (address >= 0 && address < DATA_MEM_SIZE) {
    return data_mem[address];
  }
  return 0;
}

export function instr_mem_get_array() {
  return new Uint16Array(instr_mem);
}

export function data_mem_get_array() {
  return new Int8Array(data_mem);
}
