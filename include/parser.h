/*
 * parser.h — Assembly file parser interface
 *
 * Reads assembly instructions from a text file (one per line),
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
 *
 * Example usage:
 *   int count = parser_load("programs/test1.txt");
 *   if (count < 0) { handle error }
 */
int parser_load(const char *filepath);

#endif /* PARSER_H */
