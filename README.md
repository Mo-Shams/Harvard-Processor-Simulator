# 🖥️ Package 4: Double McHarvard with Cheese Circular Shifts

> **CSEN 601 – Computer Systems Architecture – Spring 2026**
> Faculty of Media Engineering and Technology

A cycle-accurate **Harvard-architecture pipelined processor simulator** written in C. This project implements **Package 4** of the CSEN601 course project — a 3-stage pipelined processor with separate instruction/data memories, 64 general-purpose 8-bit registers, a status register, and 12 instructions including circular (arithmetic) shifts.

---

## 📑 Table of Contents

- [Architecture Overview](#architecture-overview)
- [Memory Architecture](#memory-architecture)
- [Register File](#register-file)
- [Instruction Set Architecture (ISA)](#instruction-set-architecture-isa)
- [Pipeline / Data Path](#pipeline--data-path)
- [Hazard Handling](#hazard-handling)
- [Branches and Jumps](#branches-and-jumps)
- [Status Register (SREG) Flags](#status-register-sreg-flags)
- [Program Flow & Implementation Stages](#program-flow--implementation-stages)
- [Output / Printing Requirements](#output--printing-requirements)
- [Project Structure](#project-structure)
- [Building & Running](#building--running)
- [Important Rules & Gotchas](#important-rules--gotchas)

---

## Architecture Overview

| Property | Value |
|---|---|
| **Architecture** | Harvard (separate instruction & data memories) |
| **Pipeline Stages** | 3 (IF → ID → EX) |
| **Max Parallel Instructions** | 3 |
| **Instruction Size** | 16 bits |
| **Register Size** | 8 bits (GPRs & SREG) / 16 bits (PC) |
| **Instruction Count** | 12 |

**Why Harvard?** Harvard Architecture uses separate storage and separate buses for instructions and data. This eliminates the Von Neumann bottleneck where instructions and data compete for the same memory bus. In Package 4, instruction fetch and data access can happen simultaneously since they use different memories.

---

## Memory Architecture

### Instruction Memory — `1024 × 16 bits`

| Property | Detail |
|---|---|
| **Address Range** | 0 to 1023 (2^10 − 1) |
| **Word Size** | 16 bits (2 bytes) |
| **Addressability** | Word addressable |
| **Contents** | Program instructions only |
| **C Data Type** | `int16_t` or `uint16_t` (use `short`) |

**What to implement:** An array of 1024 entries, each 16 bits wide. During parsing, assembled binary instructions are stored here. During the IF stage, the PC indexes into this array to fetch the next instruction.

### Data Memory — `2048 × 8 bits`

| Property | Detail |
|---|---|
| **Address Range** | 0 to 2047 (2^11 − 1) |
| **Word Size** | 8 bits (1 byte) |
| **Addressability** | Word/Byte addressable (1 word = 1 byte) |
| **Contents** | Program data only |
| **C Data Type** | `int8_t` or `uint8_t` (use `char` or `int8_t`) |

**What to implement:** An array of 2048 entries, each 8 bits wide. `LW` reads from this memory into a register; `SW` writes a register value into this memory.

---

## Register File

Package 4 has **66 registers** total:

### 64 General-Purpose Registers (R0–R63)

| Property | Detail |
|---|---|
| **Names** | R0 through R63 |
| **Size** | 8 bits each |
| **C Data Type** | `int8_t` (signed 8-bit) |

> ⚠️ Unlike Packages 1 & 2, there is **no hardwired zero register** in Package 4. All 64 GPRs (R0–R63) are fully writable.

### 1 Status Register (SREG)

| Bit | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| **Flag** | 0 | 0 | 0 | C | V | N | S | Z |

- **Size:** 8 bits, but only the lower 5 bits (bits 4:0) are used for flags.
- **Bits 7:5 must always be `0`.**
- See the [SREG Flags section](#status-register-sreg-flags) for detailed flag semantics.

### 1 Program Counter (PC)

| Property | Detail |
|---|---|
| **Size** | **16 bits** (NOT 8 bits — special-purpose) |
| **Initial Value** | 0 |
| **Behavior** | Incremented by 1 each fetch; updated on branches/jumps |
| **C Data Type** | `int16_t` or `uint16_t` |

---

## Instruction Set Architecture (ISA)

### Instruction Size: 16 bits

### Instruction Formats (2 types)

#### R-Type Format

| Bits | 15–12 | 11–6 | 5–0 |
|---|---|---|---|
| **Field** | OPCODE (4 bits) | R1 (6 bits) | R2 (6 bits) |

- Used by instructions that operate on two registers.
- R1 is typically the destination **and** first source operand.
- R2 is the second source operand.

#### I-Type Format

| Bits | 15–12 | 11–6 | 5–0 |
|---|---|---|---|
| **Field** | OPCODE (4 bits) | R1 (6 bits) | IMM (6 bits) |

- Used by instructions with an immediate value.
- IMM is a **6-bit signed** (2's complement) value → range: **-32 to +31**.
- **Exception:** For shift instructions (`SAL`, `SAR`), the immediate is always **positive**.

### Instruction Table

| Opcode | Mnemonic | Type | Format | Operation | Description |
|---|---|---|---|---|---|
| 0 | `ADD` | R | `ADD R1 R2` | `R1 = R1 + R2` | Adds R1 and R2, stores result in R1 |
| 1 | `SUB` | R | `SUB R1 R2` | `R1 = R1 - R2` | Subtracts R2 from R1, stores in R1 |
| 2 | `MUL` | R | `MUL R1 R2` | `R1 = R1 * R2` | Multiplies R1 by R2, stores lower 8 bits in R1 |
| 3 | `LDI` | I | `LDI R1 IMM` | `R1 = IMM` | Loads 6-bit sign-extended immediate into R1 |
| 4 | `BEQZ` | I | `BEQZ R1 IMM` | `if (R1 == 0) PC = PC+1+IMM` | Branch if R1 equals zero |
| 5 | `AND` | R | `AND R1 R2` | `R1 = R1 AND R2` | Bitwise AND of R1 and R2 |
| 6 | `OR` | R | `OR R1 R2` | `R1 = R1 OR R2` | Bitwise OR of R1 and R2 |
| 7 | `JR` | R | `JR R1 R2` | `PC = R1 \|\| R2` | Jump to address formed by concatenating R1 (MSB) and R2 (LSB) |
| 8 | `SAL` | I | `SAL R1 IMM` | `R1 = R1 << IMM` (arithmetic) | Shift Arithmetic Left by IMM positions |
| 9 | `SAR` | I | `SAR R1 IMM` | `R1 = R1 >> IMM` (arithmetic) | Shift Arithmetic Right by IMM positions (sign-extending) |
| 10 | `LW` | I | `LW R1 IMM` | `R1 = DataMem[R1 + IMM]` | Load word from data memory at address (R1 + IMM) |
| 11 | `SW` | I | `SW R1 IMM` | `DataMem[R1 + IMM] = R1` | Store R1's value into data memory at address (R1 + IMM) |

> **`||` means concatenation:** `JR R1 R2` → `PC = (R1 << 8) | (R2 & 0xFF)` forming a 16-bit address.

### Encoding Example

To encode `ADD R5 R10` (opcode = 0):
```
OPCODE = 0000 (4 bits)
R1     = 000101 (6 bits → register 5)
R2     = 001010 (6 bits → register 10)
Binary = 0000 000101 001010 = 0x014A
```

To encode `LDI R3 -5` (opcode = 3):
```
OPCODE = 0011 (4 bits)
R1     = 000011 (6 bits → register 3)
IMM    = 111011 (6 bits → -5 in 2's complement)
Binary = 0011 000011 111011 = 0x30FB
```

---

## Pipeline / Data Path

### 3 Stages

Package 4 uses a **3-stage pipeline**. Every instruction must pass through all 3 stages, even if a stage has no work for that instruction type.

#### Stage 1: Instruction Fetch (IF)

| What it does | Details |
|---|---|
| **Fetches** the next instruction | Reads `InstructionMemory[PC]` |
| **Increments** PC | `PC = PC + 1` |

**Purpose:** Retrieves the raw 16-bit binary instruction from instruction memory.

**To implement:**
1. Read the 16-bit value at `InstructionMemory[PC]`.
2. Store it in the IF/ID pipeline register.
3. Increment PC by 1.
4. Store the **original PC** (before increment) with the instruction for branch address calculation later.

#### Stage 2: Instruction Decode (ID)

| What it does | Details |
|---|---|
| **Decodes** the instruction | Extracts opcode, R1, R2/IMM fields |
| **Reads** register operands | Fetches values from register file |

**Purpose:** Breaks the 16-bit binary into its constituent fields and reads register values.

**To implement:**
1. Extract bits 15–12 → opcode.
2. Extract bits 11–6 → R1 (register index).
3. Extract bits 5–0 → R2 (register index) **AND** IMM (signed immediate).
4. **Decode into ALL possible formats** — you must extract both R-type and I-type fields regardless of actual type.
5. Read `RegisterFile[R1]` and `RegisterFile[R2]` values.
6. Sign-extend the 6-bit immediate to 8 bits.

> 🚫 **Critical Rule:** You must NOT carry over parsed field names from the parser stage. The decode stage must work purely from the binary instruction stored in memory.

#### Stage 3: Execute (EX)

| What it does | Details |
|---|---|
| **ALU operations** | Performs the computation (ADD, SUB, MUL, AND, OR, SAL, SAR) |
| **Memory access** | LW loads from data memory; SW stores to data memory |
| **Write-back** | Writes result to destination register |
| **Branch/Jump resolution** | Evaluates conditions and updates PC |
| **SREG update** | Updates status flags for applicable instructions |

**Purpose:** This is the "do-everything" stage in Package 4. Since there are no separate MEM or WB stages, the EX stage handles ALU, memory access, register write-back, and flag updates all at once.

**To implement:**
1. Perform the ALU operation based on the opcode.
2. For `LW`: compute address `R1_value + sign_extended_IMM`, read `DataMemory[address]`, write to `R1`.
3. For `SW`: compute address `R1_value + sign_extended_IMM`, write `R1_value` to `DataMemory[address]`.
4. For `BEQZ`: if `R1_value == 0`, set `PC = stored_PC + 1 + sign_extended_IMM`, flush pipeline.
5. For `JR`: set `PC = (R1_value << 8) | (R2_value & 0xFF)`, flush pipeline.
6. Write result back to the destination register.
7. Update SREG flags as applicable.

### Pipeline Timing Diagram

Each stage takes exactly **1 clock cycle**. A new instruction is fetched **every clock cycle**.

```
Cycle:    1   2   3   4   5   6   7   8   9
Inst 1:  IF  ID  EX
Inst 2:      IF  ID  EX
Inst 3:          IF  ID  EX
Inst 4:              IF  ID  EX
Inst 5:                  IF  ID  EX
Inst 6:                      IF  ID  EX
Inst 7:                          IF  ID  EX
```

### Clock Cycle Formula

```
Total Cycles = 3 + (n - 1) × 1
```

Where `n` = number of instructions.

| Example | Calculation | Result |
|---|---|---|
| 1 instruction | 3 + 0 = 3 | 3 cycles |
| 7 instructions | 3 + 6 = 9 | 9 cycles |
| 10 instructions | 3 + 9 = 12 | 12 cycles |

> ⚠️ **Do NOT pre-calculate total cycles as a stopping condition.** Stop when there are no more instructions to fetch and all pipeline stages are empty.

---

## Hazard Handling

### Data Hazards

You **must** handle data hazards. Two techniques:

#### 1. Forwarding (Data Forwarding / Bypassing)

If an instruction in EX produces a result that the instruction in ID needs, forward the result directly from EX output to ID input without waiting for it to be written to the register file.

**Example:**
```
ADD R1 R2    ; writes to R1 in EX (cycle 3)
SUB R1 R3    ; needs R1 in ID  (cycle 3) → forward from EX
```

#### 2. Stalling

If forwarding cannot resolve the hazard (e.g., a `LW` followed immediately by a dependent instruction — the loaded value isn't available until end of EX), insert a stall (bubble/NOP) to delay the dependent instruction by one cycle.

**What to detect:**
- Read-After-Write (RAW) hazards: an instruction reads a register that a preceding instruction writes to.
- Check if the destination register of the instruction in EX matches a source register of the instruction in ID.

### Control Hazards

When a branch (`BEQZ`) or jump (`JR`) reaches the EX stage:
1. If the branch is **taken** (or it's an unconditional `JR`):
   - Update the PC with the target address.
   - **Flush** all instructions currently in IF and ID stages (they entered after the branch and must be discarded).
   - Fetch the new target instruction in the **very next cycle** after EX.
2. If the branch is **not taken**: continue normally, no flush needed.

**Flush example for Packages 3 & 4:**
```
Cycle:    1   2   3   4   5   6   7   8
BEQZ:    IF  ID  EX                         ← branch resolved in cycle 3
ADD:         IF  ID  ← FLUSHED (removed)
SUB:             IF  ← FLUSHED (removed)
NEW_INST:            IF  ID  EX             ← fetch target in cycle 4
NEXT:                    IF  ID  EX
```

**How to flush:** Clear all pipeline register data for the flushed stages, or use valid/invalid flags.

---

## Branches and Jumps

### BEQZ (Branch if Equal Zero) — Opcode 4

```
BEQZ R1 IMM
```
- **Condition:** `R1 == 0`
- **If true:** `PC = PC_of_BEQZ + 1 + sign_extended(IMM)`
  - The `+1` is already done during fetch, so effectively: `PC = fetched_PC + IMM`
  - Store the PC value at fetch time with the instruction for accurate calculation.
- **If false:** Continue normally (PC already incremented).

### JR (Jump Register) — Opcode 7

```
JR R1 R2
```
- **Always jumps** (unconditional).
- **Target:** `PC = R1_value (MSB) || R2_value (LSB)` → 16-bit concatenation.
- `PC = (unsigned)(R1_value) << 8 | (unsigned)(R2_value & 0xFF)`

### Important Notes on Branches

1. The PC used in branch calculation is the PC of the branch instruction itself (store it during fetch).
2. `PC + 1 + IMM` means: address of branch + 1 (already incremented in fetch) + immediate offset.
3. All instructions fetched after the branch but before resolution must be flushed if branch/jump is taken.
4. In Package 4, fetch the target instruction in the **immediately next cycle** after EX resolves the branch.

---

## Status Register (SREG) Flags

### Flag Definitions

| Bit | Flag | Name | Description |
|---|---|---|---|
| 4 | **C** | Carry | Set when unsigned arithmetic generates a carry out of bit 7 |
| 3 | **V** | Overflow | Set when signed result overflows (can't fit in 8 bits) |
| 2 | **N** | Negative | Set when result's MSB (bit 7) is 1 |
| 1 | **S** | Sign | Expected sign: `S = N ⊕ V` |
| 0 | **Z** | Zero | Set when result is exactly 0 |

### Which Instructions Affect Which Flags

| Flag | Updated by |
|---|---|
| **C** (Carry) | `ADD` only |
| **V** (Overflow) | `ADD`, `SUB` |
| **N** (Negative) | `ADD`, `SUB`, `MUL`, `AND`, `OR`, `SAL`, `SAR` |
| **S** (Sign) | `ADD`, `SUB` |
| **Z** (Zero) | `ADD`, `SUB`, `MUL`, `AND`, `OR`, `SAL`, `SAR` |

> A flag can **only** be changed by instructions listed for it. Other instructions leave it unchanged.

### Carry Flag Calculation

```c
// Get unsigned 8-bit values into wider integers
int temp1 = (uint8_t)R1_value;  // zero-extend to int
int temp2 = (uint8_t)R2_value;  // zero-extend to int
int result = temp1 + temp2;     // for ADD

// Check bit 8 (the 9th bit)
if (result & 0x100) {
    C = 1;  // carry occurred
} else {
    C = 0;
}
```

### Overflow Flag Calculation

**For ADD:** Overflow if both operands have the same sign but the result has the opposite sign.
```c
int8_t a = R1_value, b = R2_value, res = (int8_t)(a + b);
V = ((a > 0 && b > 0 && res < 0) || (a < 0 && b < 0 && res > 0)) ? 1 : 0;
```

**For SUB:** Overflow if operands have different signs and the result has the same sign as the subtrahend.
```c
int8_t a = R1_value, b = R2_value, res = (int8_t)(a - b);
V = ((a >= 0 && b < 0 && res < 0) || (a < 0 && b >= 0 && res > 0)) ? 1 : 0;
```

**XOR trick:** `V = carry_into_bit7 XOR carry_out_of_bit7`

### Negative & Zero Flags

```c
N = (result & 0x80) ? 1 : 0;       // bit 7 of result
Z = ((int8_t)result == 0) ? 1 : 0;  // result is zero
S = N ^ V;                           // sign = N XOR V
```

---

## Program Flow & Implementation Stages

### Stage A — Parser (`parser.c`)

**What:** Read an assembly `.txt` file line by line, parse each instruction into its 16-bit binary encoding, and store it in instruction memory.

**Steps:**
1. Open the text file containing assembly instructions (one per line).
2. For each line:
   - Tokenize by spaces/commas.
   - Map the mnemonic to its opcode (0–11).
   - Extract register numbers (strip the 'R' prefix) and/or immediate values.
   - Encode into a 16-bit binary: `[OPCODE(4)][R1(6)][R2/IMM(6)]`.
   - For immediate values, convert to 6-bit 2's complement.
3. Store the 16-bit value in `InstructionMemory[address]`, incrementing the address for each instruction.

**Key Rule:** After parsing, you must **not** retain any symbolic information (mnemonics, register names). Only the binary 16-bit word goes into memory. The decode stage must re-extract all fields from the binary.

### Stage B — Pipeline Simulation (`pipeline.c`)

**What:** Simulate the 3-stage pipeline cycle by cycle.

**Steps:**
1. Initialize: `PC = 0`, all registers = 0, all memory = 0, cycle = 1.
2. Each cycle, execute stages **in reverse order** (EX → ID → IF) to avoid overwriting pipeline registers prematurely:
   - **EX:** Execute the instruction from the ID/EX pipeline register.
   - **ID:** Decode the instruction from the IF/ID pipeline register; read register values.
   - **IF:** Fetch `InstructionMemory[PC]` into IF/ID register; increment PC.
3. Check for hazards between stages and apply forwarding/stalling.
4. Check for branch/jump resolution in EX; flush if taken.
5. Print cycle info (see [Printing Requirements](#output--printing-requirements)).
6. Increment cycle counter.
7. **Stop** when no instruction remains in any stage and there are no more instructions to fetch.

### Stage C — ALU (`alu.c`)

**What:** Perform the actual computation for each instruction type.

**Implement:** A function that takes the opcode and operands, performs the operation, and returns the result along with flag updates. Handle:
- Arithmetic: ADD, SUB, MUL
- Logical: AND, OR
- Shifts: SAL (arithmetic left), SAR (arithmetic right, sign-extending)
- Memory addressing for LW/SW
- Branch condition evaluation for BEQZ
- Jump address computation for JR

### Stage D — Hazard Detection (`hazard.c`)

**What:** Detect RAW data hazards and control hazards; decide whether to forward, stall, or flush.

**Implement:**
- Compare destination register of EX-stage instruction with source registers of ID-stage instruction.
- If hazard detected: try forwarding first; if not possible, insert stall.
- For branches/jumps in EX: signal the pipeline to flush IF and ID stages.

### Stage E — Decoder (`decoder.c`)

**What:** Extract all fields from a 16-bit binary instruction.

**Implement:**
- Extract opcode: `(instruction >> 12) & 0xF`
- Extract R1: `(instruction >> 6) & 0x3F`
- Extract R2: `instruction & 0x3F`
- Extract IMM: `instruction & 0x3F` then sign-extend from 6 bits to 8 bits
- Decode into **both** R-type and I-type formats simultaneously (you don't know the type until you check the opcode).

### Stage F — Printer (`printer.c`)

**What:** Print pipeline state and system state each cycle, plus final memory/register dump.

---

## Output / Printing Requirements

### After Every Clock Cycle, Print:

1. **Clock cycle number**
2. **Pipeline stage contents:**
   - Which instruction is in IF, ID, and EX
   - Input parameters/values for each active stage
3. **Register updates:** If any register value changed, print which register and its new value
4. **Memory updates:** If any data memory location changed, print which address and the new value

### After the Last Clock Cycle, Print:

1. **All register values:** R0–R63, SREG (showing individual flags), and PC
2. **Full instruction memory** contents (all 1024 words)
3. **Full data memory** contents (all 2048 bytes)

---

## Project Structure

```
harvard-processor-simulator/
├── include/               # Header files
│   ├── types.h            # Data types, structs, constants
│   ├── memory.h           # Memory (instruction + data) interface
│   ├── registers.h        # Register file interface
│   ├── parser.h           # Assembly parser interface
│   ├── decoder.h          # Instruction decoder interface
│   ├── alu.h              # ALU operations interface
│   ├── pipeline.h         # Pipeline simulation interface
│   ├── hazard.h           # Hazard detection interface
│   └── printer.h          # Output/printing interface
├── src/                   # Source files
│   ├── main.c             # Entry point
│   ├── memory.c           # Memory implementation
│   ├── registers.c        # Register file implementation
│   ├── parser.c           # Assembly parser implementation
│   ├── decoder.c          # Decoder implementation
│   ├── alu.c              # ALU implementation
│   ├── pipeline.c         # Pipeline simulation
│   ├── hazard.c           # Hazard detection & resolution
│   └── printer.c          # Printing functions
├── programs/              # Assembly test programs (.txt)
├── tests/                 # Test cases
├── resources/             # Project description PDF
├── gui/                   # Optional GUI (bonus)
├── Makefile               # Build system
└── README.md              # This file
```

---

## Building & Running

### Prerequisites
- GCC compiler
- Make

### Build
```bash
make
```

### Run
```bash
./simulator.exe <path_to_assembly_file.txt>
```

### Example Assembly Program

Create a file `programs/test1.txt`:
```
LDI R1 5
LDI R2 3
ADD R1 R2
SUB R1 R2
MUL R1 R2
AND R1 R2
OR R1 R2
SAL R1 2
SAR R1 1
SW R1 0
LW R3 0
```

Run: `./simulator.exe programs/test1.txt`

---

## Important Rules & Gotchas

### Data Types Must Match Specification

| Element | Required C Type |
|---|---|
| Instructions (16-bit) | `int16_t` / `uint16_t` / `short` |
| Register values (8-bit) | `int8_t` / `char` |
| Data memory entries (8-bit) | `int8_t` / `uint8_t` |
| PC (16-bit) | `uint16_t` / `int16_t` |
| SREG (8-bit) | `uint8_t` |

### Key Rules

1. **PC starts at 0.** First instruction is at address 0.
2. **Immediates are signed** (6-bit 2's complement, range -32 to +31). Exception: shift amounts are always positive.
3. **Parsing must produce binary only.** No symbolic info may be carried into the pipeline. The decoder must re-extract fields from raw bits.
4. **All instructions pass through all 3 stages**, even if a stage has nothing to do for that instruction.
5. **Do NOT pre-calculate total cycles.** Stop when the pipeline is fully drained and no more instructions remain.
6. **SREG bits 7:5 must always be 0.**
7. **Flags are only updated by their associated instructions** (see the table above). Other instructions must not touch those flags.
8. **Branch PC calculation:** Use the PC value stored at fetch time. `PC_target = PC_of_branch + 1 + IMM`.
9. **Pipeline stages execute in reverse order** each cycle (EX → ID → IF) to prevent data corruption.
10. **Flushing:** When a branch is taken or JR executes, clear/invalidate instructions in IF and ID. Fetch the target in the very next cycle.

---

## Submission Deliverables

| # | Deliverable | Naming Convention |
|---|---|---|
| 1 | 3-min demo video (narrated) | `Project_Team_m_Video.mp4` |
| 2 | Project report (with cover page) | `Project_Team_m_Report.pdf` |
| 3 | C source code (zipped) | `Project_Team_m_Code.zip` |

**Zip file name:** `CSEN601_S26_Proj_Package_4_Team_m.zip`

**Submission deadline:** Saturday 16 May 2026 at 11:59 PM

**Evaluation:** 19–21 May 2026 (revision week)

> **Bonus:** +0.75% of total course grade for implementing a GUI.

---

## Quick Reference Card

```
┌─────────────────────────────────────────────┐
│           INSTRUCTION ENCODING              │
│  15  14  13  12  11  10  9  8  7  6  5...0  │
│  ├──OPCODE──┤  ├──────R1──────┤  R2/IMM     │
│    (4 bits)       (6 bits)     (6 bits)      │
├─────────────────────────────────────────────┤
│  Opcode Map:                                │
│  0=ADD  1=SUB  2=MUL   3=LDI                │
│  4=BEQZ 5=AND  6=OR    7=JR                 │
│  8=SAL  9=SAR  10=LW   11=SW                │
├─────────────────────────────────────────────┤
│  Pipeline:  IF ──► ID ──► EX                │
│  Cycles:    3 + (n-1)                        │
├─────────────────────────────────────────────┤
│  Memory:    InstrMem[1024×16]               │
│             DataMem[2048×8]                  │
│  Regs:      R0-R63 (8-bit), SREG, PC(16)   │
└─────────────────────────────────────────────┘
```