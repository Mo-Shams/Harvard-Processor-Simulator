/*
 * parser.c — Assembly file parser implementation
 *
 * Reads assembly instructions from a text file (one per line),
 * encodes each into a 16-bit binary word, and stores them in
 * instruction memory starting at address 0.
 *
 * Supported instruction formats (Package 4):
 *   R-Type: MNEMONIC R<n> R<n>       e.g. "ADD R1 R2"
 *   I-Type: MNEMONIC R<n> <imm>      e.g. "LDI R3 -5"
 *
 * Encoding layout (16 bits):
 *   [OPCODE(4)] [R1(6)] [R2/IMM(6)]
 *
 * After parsing, NO symbolic information is retained — only the
 * raw binary instruction words remain in instruction memory.
 */

#include "../include/parser.h"
#include "../include/memory.h"
#include "../include/types.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* ─── Maximum line length for reading assembly files ── */

#define MAX_LINE_LEN 256

/* ─── Mnemonic → Opcode Mapping ─────────────────────── */

/*
 * Map a mnemonic string (case-insensitive) to its opcode.
 * Returns the opcode (0–11) on success, or -1 if unknown.
 */
static int mnemonic_to_opcode(const char *mnemonic) {
  /* Table of mnemonics in opcode order (0–11) */
  static const char *mnemonics[] = {"ADD", "SUB", "MUL", "LDI", "BEQZ", "AND",
                                    "OR",  "JR",  "SAL", "SAR", "LW",   "SW"};
  static const int num_mnemonics = 12;

  /* Case-insensitive comparison */
  for (int i = 0; i < num_mnemonics; i++) {
    /* Compare character by character, case-insensitive */
    const char *a = mnemonic;
    const char *b = mnemonics[i];
    int match = 1;

    while (*a && *b) {
      if (toupper((unsigned char)*a) != toupper((unsigned char)*b)) {
        match = 0;
        break;
      }
      a++;
      b++;
    }
    /* Both must end at the same time for a full match */
    if (match && *a == '\0' && *b == '\0') {
      return i;
    }
  }
  return -1; /* Unknown mnemonic */
}

/* ─── Register Number Extraction ────────────────────── */

/*
 * Parse a register operand string like "R5" or "r63".
 * Returns the register index (0–63), or -1 on error.
 */
static int parse_register(const char *token) {
  if (token == NULL)
    return -1;

  /* Must start with 'R' or 'r' */
  if (token[0] != 'R' && token[0] != 'r')
    return -1;

  /* Parse the number after 'R' */
  char *endptr;
  long val = strtol(token + 1, &endptr, 10);

  /* Must consume all characters and be in range 0–63 */
  if (*endptr != '\0' || val < 0 || val > 63)
    return -1;

  return (int)val;
}

/* ─── Immediate Value Extraction ────────────────────── */

/*
 * Parse an immediate value string like "5", "-32", "+31".
 * Returns the value in range [-32, +31] for signed immediates.
 * Shift instructions (SAL, SAR) use positive-only immediates.
 * Returns value via pointer, returns 0 on success, -1 on error.
 */
static int parse_immediate(const char *token, int *value) {
  if (token == NULL)
    return -1;

  char *endptr;
  long val = strtol(token, &endptr, 10);

  /* Must consume all characters */
  if (*endptr != '\0')
    return -1;

  /* 6-bit signed range: -32 to +31 */
  if (val < -32 || val > 31) {
    fprintf(stderr,
            "WARNING: Immediate value %ld out of 6-bit range "
            "[-32, 31]\n",
            val);
  }

  *value = (int)val;
  return 0;
}

/* ─── Instruction Encoding ──────────────────────────── */

/*
 * Encode an instruction into a 16-bit binary word.
 *
 * Layout: [OPCODE(4)] [R1(6)] [R2_or_IMM(6)]
 *
 * For I-type: the immediate is truncated to 6 bits (2's complement).
 */
static uint16_t encode_instruction(int opcode, int r1, int r2_or_imm,
                                   int is_immediate) {
  uint16_t encoded = 0;

  /* Opcode: bits 15–12 (4 bits) */
  encoded |= (uint16_t)((opcode & 0x0F) << 12);

  /* R1: bits 11–6 (6 bits) */
  encoded |= (uint16_t)((r1 & 0x3F) << 6);

  /* R2 or IMM: bits 5–0 (6 bits) */
  if (is_immediate) {
    /* Truncate immediate to 6 bits (preserves 2's complement) */
    encoded |= (uint16_t)(r2_or_imm & 0x3F);
  } else {
    encoded |= (uint16_t)(r2_or_imm & 0x3F);
  }

  return encoded;
}

/* ─── Line Trimming Helper ──────────────────────────── */

/*
 * Trim leading and trailing whitespace from a string in-place.
 * Returns pointer to the trimmed start (within the same buffer).
 */
static char *trim(char *str) {
  /* Trim leading whitespace */
  while (isspace((unsigned char)*str))
    str++;

  if (*str == '\0')
    return str;

  /* Trim trailing whitespace */
  char *end = str + strlen(str) - 1;
  while (end > str && isspace((unsigned char)*end))
    end--;
  *(end + 1) = '\0';

  return str;
}

/* ─── Determine if Opcode Uses Immediate ────────────── */

/*
 * Returns 1 if the opcode takes an immediate operand (I-type),
 * 0 if it takes two register operands (R-type).
 */
static int is_i_type(int opcode) {
  switch (opcode) {
  case OP_LDI:
  case OP_BEQZ:
  case OP_SAL:
  case OP_SAR:
  case OP_LW:
  case OP_SW:
    return 1;
  default:
    return 0;
  }
}

/* ─── Main Parser Function ──────────────────────────── */

int parser_load(const char *filepath) {
  FILE *file = fopen(filepath, "r");
  if (file == NULL) {
    fprintf(stderr, "ERROR: Cannot open file '%s'\n", filepath);
    return -1;
  }

  char line[MAX_LINE_LEN];
  int address = 0;  /* Current instruction memory address */
  int line_num = 0; /* Source file line number (for error messages) */

  while (fgets(line, sizeof(line), file) != NULL) {
    line_num++;

    /* Trim whitespace */
    char *trimmed = trim(line);

    /* Skip empty lines and comment lines (starting with # or ;) */
    if (trimmed[0] == '\0' || trimmed[0] == '#' || trimmed[0] == ';') {
      continue;
    }

    /* Check instruction memory bounds */
    if (address >= INSTR_MEM_SIZE) {
      fprintf(stderr, "ERROR: Too many instructions (max %d) at line %d\n",
              INSTR_MEM_SIZE, line_num);
      fclose(file);
      return -1;
    }

    /* Tokenize the line: MNEMONIC OPERAND1 OPERAND2 */
    char *tokens[3] = {NULL, NULL, NULL};
    int token_count = 0;

    /* Make a working copy since strtok modifies the string */
    char work[MAX_LINE_LEN];
    strncpy(work, trimmed, MAX_LINE_LEN - 1);
    work[MAX_LINE_LEN - 1] = '\0';

    /* Tokenize by spaces, tabs, and commas */
    char *tok = strtok(work, " \t,");
    while (tok != NULL && token_count < 3) {
      tokens[token_count++] = tok;
      tok = strtok(NULL, " \t,");
    }

    /* Need at least mnemonic + 2 operands */
    if (token_count < 3) {
      fprintf(stderr, "ERROR: Malformed instruction at line %d: '%s'\n",
              line_num, trimmed);
      fclose(file);
      return -1;
    }

    /* Parse mnemonic → opcode */
    int opcode = mnemonic_to_opcode(tokens[0]);
    if (opcode < 0) {
      fprintf(stderr, "ERROR: Unknown mnemonic '%s' at line %d\n", tokens[0],
              line_num);
      fclose(file);
      return -1;
    }

    /* Parse first operand: always a register (R1) */
    int r1 = parse_register(tokens[1]);
    if (r1 < 0) {
      fprintf(stderr, "ERROR: Invalid register '%s' at line %d\n", tokens[1],
              line_num);
      fclose(file);
      return -1;
    }

    /* Parse second operand: register (R-type) or immediate (I-type) */
    int r2_or_imm = 0;
    int imm_flag = is_i_type(opcode);

    if (imm_flag) {
      /* I-Type: second operand is an immediate value */
      if (parse_immediate(tokens[2], &r2_or_imm) < 0) {
        fprintf(stderr, "ERROR: Invalid immediate '%s' at line %d\n", tokens[2],
                line_num);
        fclose(file);
        return -1;
      }
    } else {
      /* R-Type: second operand is a register (R2) */
      r2_or_imm = parse_register(tokens[2]);
      if (r2_or_imm < 0) {
        fprintf(stderr, "ERROR: Invalid register '%s' at line %d\n", tokens[2],
                line_num);
        fclose(file);
        return -1;
      }
    }

    /* Encode into 16-bit binary and store in instruction memory */
    uint16_t encoded = encode_instruction(opcode, r1, r2_or_imm, imm_flag);
    instr_mem_write((uint16_t)address, encoded);

    address++;
  }

  fclose(file);

  /* Record how many instructions were loaded */
  instr_mem_set_count(address);

  printf("Parser: loaded %d instruction(s) from '%s'\n", address, filepath);
  return address;
}
