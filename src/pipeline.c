/*
 * pipeline.c — 3-stage pipeline simulation (IF → ID → EX)
 *
 * Executes stages in reverse order each cycle (EX → ID → IF)
 * to prevent pipeline register corruption.
 *
 * Handles:
 *   - Normal instruction flow
 *   - Data hazards: forwarding (implicit via reverse order) + LW stalls
 *   - Control hazards: flush on taken BEQZ or JR
 */

#include "../include/pipeline.h"
#include "../include/alu.h"
#include "../include/decoder.h"
#include "../include/hazard.h"
#include "../include/memory.h"
#include "../include/registers.h"
#include "../include/types.h"
#include <stdio.h>

/* ─── Pipeline Registers ────────────────────────────── */

static IF_ID_Reg if_id;
static ID_EX_Reg id_ex;

/* ─── Stage: Execute (EX) ───────────────────────────── */
/*
 * Execute the instruction in id_ex.
 * Returns 1 if a flush is needed (branch taken or JR), 0 otherwise.
 */
static int stage_execute(void) {
  if (!id_ex.valid)
    return 0;

  int flush = 0;
  int8_t result;

  switch (id_ex.opcode) {

  /* ── Arithmetic / Logic / Shift: compute + writeback + flags ── */
  case OP_ADD:
  case OP_SUB:
  case OP_MUL:
  case OP_AND:
  case OP_OR:
  case OP_SAL:
  case OP_SAR:
    result = alu_execute(id_ex.opcode, id_ex.r1_val, id_ex.r2_val, id_ex.imm);
    gpr_write(id_ex.r1, result);
    break;

  /* ── Load Immediate ── */
  case OP_LDI:
    result = alu_execute(OP_LDI, id_ex.r1_val, id_ex.r2_val, id_ex.imm);
    gpr_write(id_ex.r1, result);
    break;

  /* ── Load Word: read data memory → register ── */
  case OP_LW: {
    uint16_t addr = (uint16_t)((int16_t)id_ex.r1_val + (int16_t)id_ex.imm);
    int8_t loaded = data_mem_read(addr);
    gpr_write(id_ex.r1, loaded);
    /* ALU call for flag purposes (LW doesn't update flags, but keeps it consistent) */
    (void)alu_execute(OP_LW, id_ex.r1_val, id_ex.r2_val, id_ex.imm);
    break;
  }

  /* ── Store Word: register → data memory ── */
  case OP_SW: {
    uint16_t addr = (uint16_t)((int16_t)id_ex.r1_val + (int16_t)id_ex.imm);
    data_mem_write(addr, id_ex.r1_val);
    break;
  }

  /* ── Branch if Equal Zero ── */
  case OP_BEQZ:
    if (id_ex.r1_val == 0) {
      /* PC = PC_of_branch + 1 + IMM */
      uint16_t target = (uint16_t)(id_ex.pc + 1 + id_ex.imm);
      pc_write(target);
      flush = 1;
    }
    break;

  /* ── Jump Register (unconditional) ── */
  case OP_JR: {
    uint16_t target =
        (uint16_t)(((uint8_t)id_ex.r1_val << 8) | ((uint8_t)id_ex.r2_val));
    pc_write(target);
    flush = 1;
    break;
  }

  default:
    break;
  }

  return flush;
}

/* ─── Stage: Decode (ID) ────────────────────────────── */
/*
 * Decode the instruction in if_id and populate id_ex.
 */
static void stage_decode(void) {
  if (!if_id.valid) {
    id_ex.valid = 0;
    return;
  }

  DecodedInstr d = decode_instruction(if_id.instruction);

  id_ex.opcode = d.opcode;
  id_ex.r1 = d.r1;
  id_ex.r2 = d.r2;
  id_ex.r1_val = gpr_read(d.r1);
  id_ex.r2_val = gpr_read(d.r2);
  id_ex.imm = d.imm;
  id_ex.pc = if_id.pc; /* carry forward for branch calculation */
  id_ex.valid = 1;
}

/* ─── Stage: Fetch (IF) ─────────────────────────────── */
/*
 * Fetch the instruction at PC into if_id.
 * Returns 1 if there are no more instructions to fetch, 0 otherwise.
 */
static int stage_fetch(void) {
  uint16_t current_pc = pc_read();

  if (current_pc >= (uint16_t)instr_mem_get_count()) {
    if_id.valid = 0;
    return 1; /* fetching done */
  }

  if_id.instruction = instr_mem_read(current_pc);
  if_id.pc = current_pc;
  if_id.valid = 1;
  pc_increment();
  return 0;
}

/* ─── Print helpers for cycle output ────────────────── */

static void print_stage_instr(const char *stage, int valid, uint16_t pc,
                              uint16_t raw) {
  if (!valid) {
    printf("  %s: [bubble]\n", stage);
    return;
  }
  DecodedInstr d = decode_instruction(raw);
  printf("  %s: [PC=%d] %s R%d ", stage, pc, OPCODE_NAMES[d.opcode], d.r1);
  if (d.format == FMT_R_TYPE)
    printf("R%d", d.r2);
  else
    printf("%d", d.imm);
  printf("\n");
}

/* ─── Main Pipeline Loop ────────────────────────────── */

void pipeline_run(void) {
  /* Initialize pipeline registers to empty */
  if_id.valid = 0;
  id_ex.valid = 0;

  int cycle = 1;
  int fetching_done = 0;

  printf("\n--- Pipeline Simulation ---\n");

  while (1) {
    printf("\n=== Cycle %d ===\n", cycle);

    /* Save EX instruction info before it executes (for hazard detection) */
    int ex_was_valid = id_ex.valid;
    Opcode ex_opcode = id_ex.opcode;
    uint8_t ex_r1 = id_ex.r1;

    /* Also save if_id and id_ex for printing (before they get overwritten) */
    IF_ID_Reg if_id_snap = if_id;
    ID_EX_Reg id_ex_snap = id_ex;

    /* ── 1. EXECUTE (EX) ── */
    int flush = stage_execute();

    /* ── 2. HAZARD DETECTION ── */
    int stall = 0;
    if (!flush) {
      stall = hazard_check_stall(ex_was_valid, ex_opcode, ex_r1, &if_id);
    }

    /* ── 3. DECODE (ID) ── */
    if (flush) {
      id_ex.valid = 0; /* flush: discard the instruction in ID */
    } else if (stall) {
      id_ex.valid = 0; /* stall: insert bubble, keep if_id for next cycle */
      printf("  ** STALL (load-use hazard) **\n");
    } else {
      stage_decode();
    }

    /* ── 4. FETCH (IF) ── */
    if (flush) {
      if_id.valid = 0; /* flush: discard, PC already set to target */
      fetching_done = 0; /* target might be valid, re-check next cycle */
      printf("  ** FLUSH (branch/jump taken) **\n");
    } else if (stall) {
      /* Don't fetch — keep if_id and PC unchanged */
    } else {
      int done = stage_fetch();
      if (done)
        fetching_done = 1;
    }

    /* ── Print pipeline state (using snapshots from before execution) ── */
    /* EX shows what was in id_ex (now executed) */
    if (id_ex_snap.valid) {
      DecodedInstr d = decode_instruction(
          instr_mem_read(id_ex_snap.pc));
      printf("  EX: [PC=%d] %s R%d ", id_ex_snap.pc,
             OPCODE_NAMES[d.opcode], d.r1);
      if (d.format == FMT_R_TYPE)
        printf("R%d", d.r2);
      else
        printf("%d", d.imm);
      printf("\n");
    } else {
      printf("  EX: [bubble]\n");
    }
    /* ID shows what was in if_id (now decoded or flushed) */
    print_stage_instr("ID", if_id_snap.valid, if_id_snap.pc,
                      if_id_snap.instruction);
    /* IF shows what was just fetched (current if_id, unless stall/flush) */
    if (!flush && !stall && if_id.valid) {
      print_stage_instr("IF", if_id.valid, if_id.pc, if_id.instruction);
    } else if (stall) {
      print_stage_instr("IF", if_id_snap.valid, if_id_snap.pc,
                        if_id_snap.instruction);
    } else {
      printf("  IF: [bubble]\n");
    }

    cycle++;

    /* ── Stop when pipeline is fully drained ── */
    if (!if_id.valid && !id_ex.valid && fetching_done)
      break;
  }

  printf("\n--- Pipeline complete: %d cycles ---\n", cycle - 1);
}
