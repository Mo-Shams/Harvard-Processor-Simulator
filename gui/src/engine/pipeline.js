import { Opcode, NUM_GPRS, DATA_MEM_SIZE } from './types.js';
import { gpr_read, gpr_write, pc_read, pc_write, pc_increment, gpr_get_array, sreg_read } from './registers.js';
import { instr_mem_read, instr_mem_get_count, data_mem_read, data_mem_write, data_mem_get_array } from './memory.js';
import { decode_instruction } from './decoder.js';
import { alu_execute } from './alu.js';
import { hazard_check_stall } from './hazard.js';

export class PipelineEngine {
  constructor() {
    this.if_id = { valid: false, pc: 0, instruction: 0 };
    this.id_ex = { valid: false, pc: 0, opcode: 0, r1: 0, r2: 0, r1_val: 0, r2_val: 0, imm: 0 };
    this.cycle = 1;
    this.fetching_done = false;
    this.history = [];
    
    // Snapshots
    this.gpr_snap = new Int8Array(NUM_GPRS);
    this.dmem_snap = new Int8Array(DATA_MEM_SIZE);
    this.sreg_snap = 0;
    this.pc_snap = 0;
  }

  take_snapshot() {
    this.gpr_snap.set(gpr_get_array());
    this.dmem_snap.set(data_mem_get_array());
    this.sreg_snap = sreg_read();
    this.pc_snap = pc_read();
  }

  get_changes(stage_name) {
    let changes = { registers: [], memory: [], sreg: null, pc: null };
    
    let gprs = gpr_get_array();
    for (let i = 0; i < NUM_GPRS; i++) {
      if (gprs[i] !== this.gpr_snap[i]) {
        changes.registers.push({ index: i, oldVal: this.gpr_snap[i], newVal: gprs[i], stage: stage_name });
      }
    }

    let dmem = data_mem_get_array();
    for (let i = 0; i < DATA_MEM_SIZE; i++) {
      if (dmem[i] !== this.dmem_snap[i]) {
        changes.memory.push({ address: i, oldVal: this.dmem_snap[i], newVal: dmem[i], stage: stage_name });
      }
    }

    let current_sreg = sreg_read();
    if (current_sreg !== this.sreg_snap) {
      changes.sreg = { oldVal: this.sreg_snap, newVal: current_sreg, stage: stage_name };
    }

    let current_pc = pc_read();
    if (current_pc !== this.pc_snap) {
      changes.pc = { oldVal: this.pc_snap, newVal: current_pc, stage: stage_name };
    }

    return changes;
  }

  stage_execute() {
    if (!this.id_ex.valid) return { flush: false, output_text: null };

    let flush = false;
    let result = 0;
    let output_text = "";
    let target = 0;
    let addr = 0;

    switch (this.id_ex.opcode) {
      case Opcode.ADD:
      case Opcode.SUB:
      case Opcode.MUL:
      case Opcode.AND:
      case Opcode.OR:
      case Opcode.SAL:
      case Opcode.SAR:
        result = alu_execute(this.id_ex.opcode, this.id_ex.r1_val, this.id_ex.r2_val, this.id_ex.imm);
        gpr_write(this.id_ex.r1, result);
        output_text = `R${this.id_ex.r1} = ${result}`;
        break;

      case Opcode.LDI:
        result = alu_execute(Opcode.LDI, this.id_ex.r1_val, this.id_ex.r2_val, this.id_ex.imm);
        gpr_write(this.id_ex.r1, result);
        output_text = `R${this.id_ex.r1} = ${result} (loaded imm)`;
        break;

      case Opcode.LB:
        addr = this.id_ex.imm & 0xFFFF;
        if (addr >= DATA_MEM_SIZE) addr = 0; // Prevent out of bounds
        result = data_mem_read(addr);
        gpr_write(this.id_ex.r1, result);
        alu_execute(Opcode.LB, this.id_ex.r1_val, this.id_ex.r2_val, this.id_ex.imm);
        output_text = `R${this.id_ex.r1} = DataMem[${addr}] = ${result}`;
        break;

      case Opcode.SB:
        addr = this.id_ex.imm & 0xFFFF;
        if (addr >= DATA_MEM_SIZE) addr = 0;
        data_mem_write(addr, this.id_ex.r1_val);
        output_text = `DataMem[${addr}] = ${this.id_ex.r1_val}`;
        break;

      case Opcode.BEQZ:
        if (this.id_ex.r1_val === 0) {
          target = (this.id_ex.pc + 1 + this.id_ex.imm) & 0xFFFF;
          pc_write(target);
          flush = true;
          output_text = `Branch TAKEN → PC = ${target}`;
        } else {
          output_text = `Branch NOT taken (R${this.id_ex.r1}=${this.id_ex.r1_val}!=0)`;
        }
        break;

      case Opcode.JR:
        target = ((this.id_ex.r1_val & 0xFF) << 8) | (this.id_ex.r2_val & 0xFF);
        pc_write(target);
        flush = true;
        output_text = `Jump to PC = R${this.id_ex.r1}||R${this.id_ex.r2} = ${target}`;
        break;
    }

    return { flush, output_text };
  }

  stage_decode() {
    if (!this.if_id.valid) {
      this.id_ex.valid = false;
      return;
    }
    let d = decode_instruction(this.if_id.instruction);
    this.id_ex.opcode = d.opcode;
    this.id_ex.r1 = d.r1;
    this.id_ex.r2 = d.r2;
    this.id_ex.r1_val = gpr_read(d.r1);
    this.id_ex.r2_val = gpr_read(d.r2);
    this.id_ex.imm = d.imm;
    this.id_ex.pc = this.if_id.pc;
    this.id_ex.valid = true;
  }

  stage_fetch() {
    let current_pc = pc_read();
    if (current_pc >= instr_mem_get_count()) {
      this.if_id.valid = false;
      return true;
    }
    this.if_id.instruction = instr_mem_read(current_pc);
    this.if_id.pc = current_pc;
    this.if_id.valid = true;
    pc_increment();
    return false;
  }

  step() {
    if (this.is_done()) return null;

    let snapshot = {
      cycle: this.cycle,
      stages: { IF: {}, ID: {}, EX: {} },
      changes: { registers: [], memory: [], sreg: null, pc: null },
      hazard: null
    };

    let if_id_snap = { ...this.if_id };
    let id_ex_snap = { ...this.id_ex };

    let ex_was_valid = this.id_ex.valid;
    let ex_opcode = this.id_ex.opcode;
    let ex_r1 = this.id_ex.r1;

    // --- STAGE: EX ---
    this.take_snapshot();
    let ex_res = this.stage_execute();
    
    if (id_ex_snap.valid) {
      snapshot.stages.EX = {
        valid: true,
        pc: id_ex_snap.pc,
        instruction: instr_mem_read(id_ex_snap.pc),
        decoded: decode_instruction(instr_mem_read(id_ex_snap.pc)),
        r1_val: id_ex_snap.r1_val,
        r2_val: id_ex_snap.r2_val,
        imm: id_ex_snap.imm,
        output_text: ex_res.output_text
      };
      
      let changes = this.get_changes("EX");
      snapshot.changes.registers.push(...changes.registers);
      snapshot.changes.memory.push(...changes.memory);
      if (changes.sreg) snapshot.changes.sreg = changes.sreg;
      if (changes.pc && !snapshot.changes.pc) snapshot.changes.pc = changes.pc;
    } else {
      snapshot.stages.EX = { valid: false };
    }

    this.take_snapshot();

    // --- HAZARD DETECTION ---
    let stall = false;
    let flush = ex_res.flush;
    if (!flush) {
      stall = hazard_check_stall(ex_was_valid, ex_opcode, ex_r1, this.if_id);
    }

    // --- STAGE: ID ---
    if (flush) {
      this.id_ex.valid = false;
      snapshot.hazard = { type: 'flush', reason: 'branch-taken' };
      snapshot.stages.ID = { valid: if_id_snap.valid, pc: if_id_snap.pc, instruction: if_id_snap.instruction, status: 'FLUSHED' };
    } else if (stall) {
      this.id_ex.valid = false;
      snapshot.hazard = { type: 'stall', reason: 'load-use' };
      snapshot.stages.ID = { valid: if_id_snap.valid, pc: if_id_snap.pc, instruction: if_id_snap.instruction, status: 'STALLED' };
    } else {
      if (if_id_snap.valid) {
        this.stage_decode();
        snapshot.stages.ID = {
          valid: true,
          pc: if_id_snap.pc,
          instruction: if_id_snap.instruction,
          decoded: decode_instruction(if_id_snap.instruction),
          r1_val: this.id_ex.r1_val,
          r2_val: this.id_ex.r2_val,
          imm: this.id_ex.imm,
          status: 'NORMAL'
        };
      } else {
        this.id_ex.valid = false;
        snapshot.stages.ID = { valid: false };
      }
    }

    this.take_snapshot();

    // --- STAGE: IF ---
    if (flush) {
      this.if_id.valid = false;
      this.fetching_done = false;
      snapshot.stages.IF = { valid: false, status: 'FLUSHED' };
    } else if (stall) {
      snapshot.stages.IF = { valid: this.if_id.valid, pc: this.if_id.pc, instruction: this.if_id.instruction, status: 'STALLED' };
    } else {
      let done = this.stage_fetch();
      if (done) {
        this.fetching_done = true;
        snapshot.stages.IF = { valid: false, status: 'DONE' };
      } else {
        snapshot.stages.IF = { valid: true, pc: this.if_id.pc, instruction: this.if_id.instruction, status: 'NORMAL' };
        
        let changes = this.get_changes("IF");
        if (changes.pc) snapshot.changes.pc = changes.pc;
      }
    }

    this.take_snapshot();

    this.cycle++;
    
    // Check if fully done next cycle
    snapshot.done = this.is_done();
    this.history.push(snapshot);
    return snapshot;
  }

  is_done() {
    return !this.if_id.valid && !this.id_ex.valid && this.fetching_done;
  }
}
