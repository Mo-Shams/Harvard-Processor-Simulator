/*
 * printer.c — Output/printing functions
 *
 * Prints pipeline state and system state each cycle,
 * plus final memory/register dump after the last cycle.
 */

#include "../include/printer.h"
#include "../include/memory.h"
#include "../include/registers.h"
#include <stdio.h>

/*
 * Print the final state after the last clock cycle:
 *   - All register values (R0–R63, SREG flags, PC)
 *   - Full instruction memory (all 1024 words)
 *   - Full data memory (all 2048 bytes)
 */
void printer_final(void) {
  printf("\n=== Final State ===\n");

  /* Print Registers R0-R63 */
  printf("\nRegisters (R0-R63):\n");
  const int8_t *gprs = gpr_get_array();
  for (int i = 0; i < NUM_GPRS; i++) {
    printf("R%-2d: %-4d ", i, gprs[i]);
    if ((i + 1) % 8 == 0) {
      printf("\n");
    }
  }

  /* Print SREG Flags */
  printf("\nSREG: C=%d V=%d N=%d S=%d Z=%d\n", sreg_get_flag(SREG_C_BIT),
         sreg_get_flag(SREG_V_BIT), sreg_get_flag(SREG_N_BIT),
         sreg_get_flag(SREG_S_BIT), sreg_get_flag(SREG_Z_BIT));

  /* Print PC */
  printf("PC: %d\n", pc_read());

  /* Print Instruction Memory */
  printf("\nInstruction Memory (1024 words):\n");
  const uint16_t *imem = instr_mem_get_array();
  for (int i = 0; i < INSTR_MEM_SIZE; i++) {
    if (imem[i] != 0) {
      printf("  [%3d] 0x%04X\n", i, imem[i]);
    }
  }

  /* Print Data Memory */
  printf("\nData Memory (2048 bytes):\n");
  const int8_t *dmem = data_mem_get_array();
  for (int i = 0; i < DATA_MEM_SIZE; i++) {
    if (dmem[i] != 0) {
      printf("  [%4d] %d\n", i, dmem[i]);
    }
  }
}
