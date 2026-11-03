/*
 * parser.h — Assembly file parser interface
 *
 * Reads assembly instructions from a text file or in-memory string,
 * encodes each into a 16-bit binary word, and stores them
 * sequentially in instruction memory starting at address 0.
 *
 * After parsing, NO symbolic information (mnemonics, register names)
 * is retained — only the raw binary instruction words remain in memory.
 */

#ifndef PARSER_H
#define PARSER_H

/*
 * Parse the assembly file at the given path and load encoded
 * instructions into instruction memory.
 *
 * Returns the number of instructions successfully parsed and loaded,
 * or -1 on error (file not found, malformed instruction, etc.).
 */
int parser_load(const char *filepath);

/*
 * Parse assembly instructions directly from an in-memory null-terminated string.
 *
 * Returns the number of instructions successfully parsed and loaded,
 * or -1 on error.
 */
int parser_load_string(const char *assembly_text);

/*
 * Control parser console output (1 = enabled, 0 = silent).
 */
void parser_set_verbose(int verbose);

#endif /* PARSER_H */
