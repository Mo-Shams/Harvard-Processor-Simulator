/*
 * main.c — Entry point for the Harvard Pipeline Simulator
 *
 * Package 4: Double McHarvard with cheese circular shifts
 */

#include <stdio.h>
#include <stdint.h>
#include "../include/types.h"
#include "../include/memory.h"
#include "../include/registers.h"
#include "../include/parser.h"
#include "../include/decoder.h"
#include "../include/pipeline.h"
#include "../include/printer.h"

int main(int argc, char *argv[]) {
    /* Check command-line arguments */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <assembly_file.txt>\n", argv[0]);
        return 1;
    }

    printf("=== Harvard Pipeline Simulator (Package 4) ===\n\n");

    /* Initialize all subsystems */
    memory_init();
    registers_init();

    /* Parse assembly file into instruction memory */
    int instr_count = parser_load(argv[1]);
    if (instr_count < 0) {
        fprintf(stderr, "Failed to load assembly file.\n");
        return 1;
    }

    /* Verify: print loaded instructions in binary */
    printf("\n--- Instruction Memory (loaded) ---\n");
    const uint16_t *imem = instr_mem_get_array();
    for (int i = 0; i < instr_count; i++) {
        /* Decode each instruction to verify parsing */
        DecodedInstr d = decode_instruction(imem[i]);
        printf("  [%3d] 0x%04X  |  %-4s  R%d  ",
               i, imem[i], OPCODE_NAMES[d.opcode], d.r1);
        if (d.format == FMT_R_TYPE) {
            printf("R%d", d.r2);
        } else {
            printf("%d", d.imm);
        }
        printf("\n");
    }

    /* Run the pipeline simulation */
    pipeline_run();
    
    /* Print the final state */
    printer_final();

    return 0;
}