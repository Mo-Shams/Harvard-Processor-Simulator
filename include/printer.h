/*
 * printer.h — Output/printing interface
 *
 * Prints pipeline state and system state each cycle,
 * plus final memory/register dump after the last cycle.
 * (Implementation will be done in a future stage)
 */

#ifndef PRINTER_H
#define PRINTER_H

#include "../include/types.h"

/*
 * Print the state of the pipeline for the current clock cycle:
 *   - Cycle number
 *   - Which instruction is in each stage (IF, ID, EX)
 *   - Register/memory updates that occurred
 */
/* void printer_cycle(int cycle, const IF_ID_Reg *if_id, const ID_EX_Reg *id_ex); */

/*
 * Print the final state after the last clock cycle:
 *   - All register values (R0–R63, SREG flags, PC)
 *   - Full instruction memory (all 1024 words)
 *   - Full data memory (all 2048 bytes)
 */
void printer_final(void);

#endif /* PRINTER_H */
