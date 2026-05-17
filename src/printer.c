/*
 * printer.c — Output/printing functions
 *
 * Prints pipeline state and system state each cycle,
 * plus final memory/register dump after the last cycle.
 *
 * Uses range-compression for memory dumps: consecutive zero entries
 * are collapsed into a single summary line, while non-zero entries
 * are printed individually. This keeps the output complete (every
 * address accounted for) without flooding the terminal.
 */

#include "../include/printer.h"
#include "../include/memory.h"
#include "../include/registers.h"
#include <stdio.h>

/* ─── Helper: flush a pending zero-range ────────────── */

static void flush_zero_range(int start, int end) {
  if (start < 0)
    return; /* no pending range */
  if (start == end) {
    /* single zero entry */
    printf("  [%4d] 0\n", start);
  } else {
    /* range of consecutive zeros */
    printf("  [%4d .. %4d] = 0  (%d locations)\n", start, end,
           end - start + 1);
  }
}

static void flush_zero_range_hex(int start, int end) {
  if (start < 0)
    return;
  if (start == end) {
    printf("  [%4d] 0x0000\n", start);
  } else {
    printf("  [%4d .. %4d] = 0x0000  (%d locations)\n", start, end,
           end - start + 1);
  }
}

/*
 * Print the final state after the last clock cycle:
 *   - All register values (R0–R63, SREG flags, PC)
 *   - Full instruction memory (all 1024 words)
 *   - Full data memory (all 2048 bytes)
 */
void printer_final(void) {
  printf("\n==================== FINAL STATE ====================\n");

  /* ── Registers R0-R63 ── */
  printf("\n--- Registers (R0-R63) ---\n");
  const int8_t *gprs = gpr_get_array();
  for (int i = 0; i < NUM_GPRS; i++) {
    printf("R%-2d: %-4d ", i, gprs[i]);
    if ((i + 1) % 8 == 0) {
      printf("\n");
    }
  }

  /* ── SREG ── */
  printf("\n--- Status Register (SREG) ---\n");
  printf("SREG = 0x%02X  |  C=%d  V=%d  N=%d  S=%d  Z=%d\n", sreg_read(),
         sreg_get_flag(SREG_C_BIT), sreg_get_flag(SREG_V_BIT),
         sreg_get_flag(SREG_N_BIT), sreg_get_flag(SREG_S_BIT),
         sreg_get_flag(SREG_Z_BIT));

  /* ── PC ── */
  printf("\n--- Program Counter ---\n");
  printf("PC: %d\n", pc_read());

  /* ── Instruction Memory (range-compressed) ── */
  printf("\n--- Instruction Memory (1024 words) ---\n");
  const uint16_t *imem = instr_mem_get_array();
  int zero_start = -1;
  for (int i = 0; i < INSTR_MEM_SIZE; i++) {
    if (imem[i] == 0) {
      if (zero_start < 0)
        zero_start = i;
    } else {
      flush_zero_range_hex(zero_start, i - 1);
      zero_start = -1;
      printf("  [%4d] 0x%04X\n", i, imem[i]);
    }
  }
  flush_zero_range_hex(zero_start, INSTR_MEM_SIZE - 1);

  /* ── Data Memory (range-compressed) ── */
  printf("\n--- Data Memory (2048 bytes) ---\n");
  const int8_t *dmem = data_mem_get_array();
  zero_start = -1;
  for (int i = 0; i < DATA_MEM_SIZE; i++) {
    if (dmem[i] == 0) {
      if (zero_start < 0)
        zero_start = i;
    } else {
      flush_zero_range(zero_start, i - 1);
      zero_start = -1;
      printf("  [%4d] %d\n", i, dmem[i]);
    }
  }
  flush_zero_range(zero_start, DATA_MEM_SIZE - 1);

  printf("\n=====================================================\n");
}
