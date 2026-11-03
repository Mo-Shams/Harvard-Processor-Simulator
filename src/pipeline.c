/*
 * pipeline.c — 3-stage pipeline simulation (IF → ID → EX)
 *
 * Executes stages in reverse order each cycle (EX → ID → IF)
 * to prevent pipeline register corruption.
 *
 * Handles:
 *   - Normal instruction flow
 *   - Data hazards: forwarding (implicit via reverse order) + LB stalls
 *   - Control hazards: flush on taken BEQZ or JR
 *
 * Prints per-cycle:
 *   - Cycle number
 *   - Which instruction is in each stage with input/output values
 *   - Register changes (which register, new value, which stage)
 *   - Data memory changes (which address, new value, which stage)
 */

#include "../include/pipeline.h"
#include "../include/alu.h"
#include "../include/decoder.h"
#include "../include/hazard.h"
#include "../include/memory.h"
#include "../include/registers.h"
#include "../include/types.h"
#include <stdio.h>
#include <string.h>

const char *OPCODE_NAMES[] = {"ADD", "SUB", "MUL", "LDI", "BEQZ", "AND",
                              "OR",  "JR",  "SAL", "SAR", "LB",   "SB"};

/* ─── Pipeline Registers ────────────────────────────── */

static IF_ID_Reg if_id;
static ID_EX_Reg id_ex;
static int verbose_mode = 1;

void pipeline_set_verbose(int verbose) {
  verbose_mode = verbose;
}

/* ─── Snapshot arrays for tracking register/memory changes ─── */

static int8_t   gpr_snapshot[NUM_GPRS];
static int8_t   dmem_snapshot[DATA_MEM_SIZE];
static uint8_t  sreg_snapshot;
static uint16_t pc_snapshot;
static PipelineMetrics metrics;

/*
 * Take a snapshot of all registers and data memory
 * so we can diff after each cycle to detect changes.
 */
static void take_snapshot(void) {
  const int8_t *gprs = gpr_get_array();
  memcpy(gpr_snapshot, gprs, sizeof(gpr_snapshot));

  const int8_t *dmem = data_mem_get_array();
  memcpy(dmem_snapshot, dmem, sizeof(dmem_snapshot));

  sreg_snapshot = sreg_read();
  pc_snapshot = pc_read();
}

/*
 * Print any register/memory changes that occurred since the snapshot.
 */
static void print_changes(const char *stage_name) {
  if (!verbose_mode)
    return;

  const int8_t *gprs = gpr_get_array();
  const int8_t *dmem = data_mem_get_array();

  /* Check GPR changes */
  for (int i = 0; i < NUM_GPRS; i++) {
    if (gprs[i] != gpr_snapshot[i]) {
      printf("    [%s] R%d changed to %d\n", stage_name, i, gprs[i]);
    }
  }

  /* Check SREG changes */
  uint8_t current_sreg = sreg_read();
  if (current_sreg != sreg_snapshot) {
    printf("    [%s] SREG changed to 0x%02X (C=%d V=%d N=%d S=%d Z=%d)\n",
           stage_name,
           current_sreg,
           sreg_get_flag(SREG_C_BIT),
           sreg_get_flag(SREG_V_BIT),
           sreg_get_flag(SREG_N_BIT),
           sreg_get_flag(SREG_S_BIT),
           sreg_get_flag(SREG_Z_BIT));
  }

  /* Check PC changes */
  uint16_t current_pc = pc_read();
  if (current_pc != pc_snapshot) {
    printf("    [%s] PC changed to %d\n", stage_name, current_pc);
  }

  /* Check data memory changes */
  for (int i = 0; i < DATA_MEM_SIZE; i++) {
    if (dmem[i] != dmem_snapshot[i]) {
      printf("    [%s] DataMem[%d] changed to %d\n", stage_name, i, dmem[i]);
    }
  }
}

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

  /* ── Load Byte: read data memory → register ── */
  case OP_LB: {
    uint16_t addr = (uint16_t)((int16_t)id_ex.imm);
    int8_t loaded = data_mem_read(addr);
    gpr_write(id_ex.r1, loaded);
    /* ALU call for flag purposes (LB doesn't update flags, but keeps it consistent) */
    (void)alu_execute(OP_LB, id_ex.r1_val, id_ex.r2_val, id_ex.imm);
    break;
  }

  /* ── Store Byte: register → data memory ── */
  case OP_SB: {
    uint16_t addr = (uint16_t)((int16_t)id_ex.imm);
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

/* ─── Print helpers for instruction display ─────────── */

static void print_instr_name(uint16_t raw) {
  if (!verbose_mode)
    return;
  DecodedInstr d = decode_instruction(raw);
  printf("%s R%d ", OPCODE_NAMES[d.opcode], d.r1);
  if (d.format == FMT_R_TYPE)
    printf("R%d", d.r2);
  else
    printf("%d", d.imm);
}

/* ─── Main Pipeline Loop ────────────────────────────── */

#define PPRINT(...) do { if (verbose_mode) printf(__VA_ARGS__); } while (0)

void pipeline_run(void) {
  /* Initialize pipeline registers to empty */
  if_id.valid = 0;
  id_ex.valid = 0;

  int cycle = 1;
  int fetching_done = 0;
  metrics = (PipelineMetrics){0, 0, 0, 0};

  PPRINT("\n==================== PIPELINE SIMULATION ====================\n");

  while (1) {
    PPRINT("\n================ Cycle %d ================\n", cycle);

    /* Save snapshots of pipeline registers for printing (before they change) */
    IF_ID_Reg if_id_snap = if_id;
    ID_EX_Reg id_ex_snap = id_ex;

    /* Save EX instruction info before it executes (for hazard detection) */
    int ex_was_valid = id_ex.valid;
    Opcode ex_opcode = id_ex.opcode;
    uint8_t ex_r1 = id_ex.r1;

    /* ═══════════════════════════════════════════════════════════════
     * STAGE 1: EXECUTE (EX) — runs first to avoid pipeline corruption
     * ═══════════════════════════════════════════════════════════════ */
    take_snapshot();
    int flush = stage_execute();
    if (id_ex.valid)
      metrics.retired_instructions++;
    if (flush)
      metrics.branch_flushes++;

    /* Print EX stage info */
    PPRINT("\n  [EX Stage]\n");
    if (id_ex_snap.valid) {
      PPRINT("    Instruction: [PC=%d] ", id_ex_snap.pc);
      print_instr_name(instr_mem_read(id_ex_snap.pc));
      PPRINT("\n");

      /* Print input values */
      DecodedInstr d_ex = decode_instruction(instr_mem_read(id_ex_snap.pc));
      PPRINT("    Inputs: R%d_val=%d", d_ex.r1, id_ex_snap.r1_val);
      if (d_ex.format == FMT_R_TYPE) {
        PPRINT(", R%d_val=%d", d_ex.r2, id_ex_snap.r2_val);
      } else {
        PPRINT(", IMM=%d", id_ex_snap.imm);
      }
      PPRINT("\n");

      /* Print output / action */
      switch (id_ex_snap.opcode) {
      case OP_ADD:
      case OP_SUB:
      case OP_MUL:
      case OP_AND:
      case OP_OR:
      case OP_SAL:
      case OP_SAR:
        PPRINT("    Output: R%d = %d\n", id_ex_snap.r1,
               gpr_read(id_ex_snap.r1));
        break;
      case OP_LDI:
        PPRINT("    Output: R%d = %d (loaded immediate)\n", id_ex_snap.r1,
               gpr_read(id_ex_snap.r1));
        break;
      case OP_LB: {
        uint16_t addr = (uint16_t)((uint8_t)id_ex_snap.imm & 0x3F);
        PPRINT("    Output: R%d = DataMem[%d] = %d\n", id_ex_snap.r1, addr,
               gpr_read(id_ex_snap.r1));
        break;
      }
      case OP_SB: {
        uint16_t addr = (uint16_t)((uint8_t)id_ex_snap.imm & 0x3F);
        PPRINT("    Output: DataMem[%d] = %d\n", addr, id_ex_snap.r1_val);
        break;
      }
      case OP_BEQZ:
        if (flush) {
          uint16_t target =
              (uint16_t)(id_ex_snap.pc + 1 + id_ex_snap.imm);
          PPRINT("    Output: Branch TAKEN → PC = %d + 1 + %d = %d\n",
                 id_ex_snap.pc, id_ex_snap.imm, target);
        } else {
          PPRINT("    Output: Branch NOT taken (R%d = %d != 0)\n",
                 id_ex_snap.r1, id_ex_snap.r1_val);
        }
        break;
      case OP_JR: {
        uint16_t target = (uint16_t)(((uint8_t)id_ex_snap.r1_val << 8) |
                                     ((uint8_t)id_ex_snap.r2_val));
        PPRINT("    Output: Jump to PC = R%d||R%d = %d\n", id_ex_snap.r1,
               id_ex_snap.r2, target);
        break;
      }
      default:
        break;
      }

      /* Print register/memory changes from EX */
      print_changes("EX");
    } else {
      PPRINT("    [bubble]\n");
    }

    /* Update snapshot after EX changes */
    take_snapshot();

    /* ═══════════════════════════════════════════════════════════════
     * HAZARD DETECTION
     * ═══════════════════════════════════════════════════════════════ */
    int stall = 0;
    if (!flush) {
      stall = hazard_check_stall(ex_was_valid, ex_opcode, ex_r1, &if_id);
    }
    if (stall)
      metrics.data_hazard_stalls++;

    /* ═══════════════════════════════════════════════════════════════
     * STAGE 2: DECODE (ID)
     * ═══════════════════════════════════════════════════════════════ */
    PPRINT("\n  [ID Stage]\n");
    if (flush) {
      id_ex.valid = 0; /* flush: discard the instruction in ID */
      if (if_id_snap.valid) {
        PPRINT("    Instruction: [PC=%d] ", if_id_snap.pc);
        print_instr_name(if_id_snap.instruction);
        PPRINT(" → FLUSHED\n");
      } else {
        PPRINT("    [bubble]\n");
      }
    } else if (stall) {
      id_ex.valid = 0; /* stall: insert bubble, keep if_id for next cycle */
      PPRINT("    Instruction: [PC=%d] ", if_id_snap.pc);
      print_instr_name(if_id_snap.instruction);
      PPRINT(" → STALLED (load-use hazard)\n");
    } else {
      if (if_id_snap.valid) {
        stage_decode();
        PPRINT("    Instruction: [PC=%d] ", if_id_snap.pc);
        print_instr_name(if_id_snap.instruction);
        PPRINT("\n");
        /* Print decoded values (outputs of decode stage) */
        PPRINT("    Decoded: opcode=%s, R1=R%d(val=%d)",
               OPCODE_NAMES[id_ex.opcode], id_ex.r1, id_ex.r1_val);
        DecodedInstr d_id = decode_instruction(if_id_snap.instruction);
        if (d_id.format == FMT_R_TYPE) {
          PPRINT(", R2=R%d(val=%d)", id_ex.r2, id_ex.r2_val);
        } else {
          PPRINT(", IMM=%d", id_ex.imm);
        }
        PPRINT("\n");
      } else {
        id_ex.valid = 0;
        PPRINT("    [bubble]\n");
      }
    }

    /* Update snapshot after ID changes */
    take_snapshot();

    /* ═══════════════════════════════════════════════════════════════
     * STAGE 3: FETCH (IF)
     * ═══════════════════════════════════════════════════════════════ */
    PPRINT("\n  [IF Stage]\n");
    if (flush) {
      if_id.valid = 0; /* flush: discard, PC already set to target */
      fetching_done = 0; /* target might be valid, re-check next cycle */
      PPRINT("    → FLUSHED (branch/jump taken)\n");
    } else if (stall) {
      /* Don't fetch — keep if_id and PC unchanged */
      PPRINT("    Instruction: [PC=%d] ", if_id.pc);
      print_instr_name(if_id.instruction);
      PPRINT(" → STALLED (waiting)\n");
    } else {
      int done = stage_fetch();
      if (done) {
        fetching_done = 1;
        PPRINT("    [no more instructions to fetch]\n");
      } else {
        PPRINT("    Fetched: [PC=%d] ", if_id.pc);
        print_instr_name(if_id.instruction);
        PPRINT(" (raw=0x%04X)\n", if_id.instruction);
        /* Print PC change from fetch */
        print_changes("IF");
      }
    }

    /* Update snapshot for next cycle */
    take_snapshot();

    cycle++;

    /* ── Stop when pipeline is fully drained ── */
    if (!if_id.valid && !id_ex.valid && fetching_done)
      break;
  }

  PPRINT("\n================ Pipeline complete: %d cycles ================\n",
         cycle - 1);
  metrics.total_cycles = cycle - 1;
  PPRINT("Metrics: cycles=%d retired=%d CPI=%.2f stalls=%d flushes=%d\n",
         metrics.total_cycles, metrics.retired_instructions,
         metrics.retired_instructions
             ? (double)metrics.total_cycles / metrics.retired_instructions
             : 0.0,
         metrics.data_hazard_stalls, metrics.branch_flushes);
}

PipelineMetrics pipeline_get_metrics(void) { return metrics; }
