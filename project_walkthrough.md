# 🖥️ Harvard Pipeline Simulator — Project Walkthrough

## How the Program Flows (Big Picture)

```
Assembly File (.txt)
       │
       ▼
  ┌─────────┐
  │ parser.c│  → reads text, encodes each instruction into 16-bit binary
  └────┬────┘
       │ stores into
       ▼
  ┌──────────┐
  │ memory.c │  → holds InstructionMemory[1024] + DataMemory[2048]
  └────┬─────┘
       │ fed into
       ▼
  ┌───────────┐
  │pipeline.c │  → runs cycle-by-cycle (IF → ID → EX), in reverse (EX→ID→IF)
  └─┬───┬───┬─┘
    │   │   │
    │   │   └──▶ decoder.c  (ID stage: breaks 16-bit binary back into fields)
    │   └──────▶ alu.c      (EX stage: performs computation, updates SREG)
    └──────────▶ hazard.c   (detects load-use stalls between EX and ID)
       │
       ▼
  ┌───────────┐
  │registers.c│  → holds R0–R63, SREG, PC (read/written by pipeline stages)
  └───────────┘
       │
       ▼
  ┌──────────┐
  │ printer.c│  → dumps final register + memory state after simulation ends
  └──────────┘
```

---

## What Each `.c` File Does

### 1. `main.c` — Entry Point
**Role:** Orchestrates everything in order.

```
1. Parse command-line argument (filename)
2. memory_init()    ← zero out all memory
3. registers_init() ← zero out all registers + PC
4. parser_load()    ← load assembly into instruction memory
5. Print instruction memory (verification step)
6. pipeline_run()   ← run the full simulation
7. printer_final()  ← dump final state
```

> Think of it as the "main menu" that calls each module in sequence.

---

### 2. `parser.c` — Assembly → Binary
**Role:** Reads the `.txt` assembly file and converts each instruction into a 16-bit binary word stored in instruction memory.

**How it works:**
1. Opens the file line by line
2. Strips comments (`#`, `;`) and blank lines
3. Tokenizes each line: `MNEMONIC`, `OPERAND1`, `OPERAND2`
4. Maps mnemonic → opcode number (0–11)
5. Extracts register indices (strips the `R` prefix)
6. Encodes into 16 bits: `[OPCODE(4)][R1(6)][R2/IMM(6)]`
7. Writes the binary word to `InstructionMemory[address]`

**Key rule:** After parsing, **no symbolic info is kept** — only the raw binary. The decode stage must re-extract everything from the bits.

**Example:**
```
"ADD R1 R2"  →  opcode=0, r1=1, r2=2
             →  0000 000001 000010  =  0x0042
```

---

### 3. `memory.c` — Memory Subsystem
**Role:** Provides the two separate memories of the Harvard architecture.

| Memory | Size | Type | Used by |
|---|---|---|---|
| Instruction Memory | 1024 × 16-bit | `uint16_t[]` | IF stage reads from it |
| Data Memory | 2048 × 8-bit | `int8_t[]` | LW reads, SW writes |

**Key functions:**
- `instr_mem_read(addr)` — fetch instruction at address
- `data_mem_read(addr)` / `data_mem_write(addr, val)` — used by LW/SW in EX stage

---

### 4. `registers.c` — Register File
**Role:** Holds all CPU state that persists across cycles.

| Register | Count | Size | Purpose |
|---|---|---|---|
| R0–R63 (GPRs) | 64 | 8-bit `int8_t` | General computation |
| SREG | 1 | 8-bit `uint8_t` | Status flags: C,V,N,S,Z |
| PC | 1 | 16-bit `uint16_t` | Points to next instruction to fetch |

**Note:** Unlike other architectures, **all 64 registers are writable** (no hardwired R0=0).

---

### 5. `decoder.c` — Instruction Decoder (ID Stage)
**Role:** Takes a raw 16-bit binary and extracts all fields.

```
Raw:  0000 000001 000010  (0x0042)
           │      │    │
      opcode=0   r1=1  r2=2
      (ADD)
```

**What it extracts (always, regardless of type):**
- `opcode` = bits 15–12
- `r1` = bits 11–6
- `r2` = bits 5–0 (R-type interpretation)
- `imm` = bits 5–0, **sign-extended** to 8 bits (I-type interpretation)
- `format` = R-type or I-type (determined by opcode)

It always decodes **both** r2 and imm — the pipeline decides which one to use based on the format.

---

### 6. `alu.c` — Arithmetic Logic Unit (EX Stage)
**Role:** Performs the actual computation for each instruction.

| Instruction | What ALU does |
|---|---|
| ADD, SUB, MUL | Arithmetic on R1, R2 |
| AND, OR | Bitwise on R1, R2 |
| SAL, SAR | Shift R1 by IMM positions |
| LDI | Returns IMM directly |
| LW, SW | Computes address = R1 + IMM |
| BEQZ | Returns 1 if R1==0, else 0 |
| JR | Returns 0 (pipeline handles PC) |

**Also updates SREG flags** based on the result:
- **C** (Carry): ADD only
- **V** (Overflow): ADD, SUB
- **N** (Negative): ADD, SUB, MUL, AND, OR, SAL, SAR
- **S** (Sign = N⊕V): ADD, SUB
- **Z** (Zero): ADD, SUB, MUL, AND, OR, SAL, SAR

> Flags not listed for an instruction are **never touched**.

---

### 7. `hazard.c` — Hazard Detection
**Role:** Detects when a stall is needed due to a **load-use hazard**.

**What is a load-use hazard?**
When `LW` is in EX and the very next instruction in ID needs the loaded value:
```
LW  R1 0     ← loads R1 from memory during EX (Cycle N)
ADD R1 R2    ← needs R1 during ID  (Cycle N) — too soon!
```
The value isn't available until the end of EX, so we must **stall 1 cycle**.

**For all other RAW hazards** (non-LW), forwarding is **implicit** because EX runs before ID each cycle (reverse order), so the register file is already updated when ID reads it.

**What it does on stall:**
- Inserts a bubble into EX (id_ex.valid = 0)
- Keeps IF/ID frozen (doesn't advance the pipeline)
- Doesn't fetch a new instruction

---

### 8. `pipeline.c` — Pipeline Simulation Engine
**Role:** The heart of the simulator. Runs the 3-stage pipeline cycle by cycle.

**The main loop each cycle (in reverse order):**
```
Cycle N:
  1. EX  — execute instruction from id_ex register
  2. ID  — decode instruction from if_id register → populate id_ex
  3. IF  — fetch InstructionMemory[PC] → populate if_id, PC++
```

**Why reverse order?** Prevents one stage from overwriting data the next stage still needs to read in the same cycle.

**Special cases handled:**
- **Stall (load-use):** Skip ID and IF; insert bubble into EX
- **Flush (branch/jump taken):** Invalidate IF and ID; set PC to target address
- **Drain:** After all instructions fetched, keep running until pipeline is empty

**Pipeline registers used:**
```c
IF_ID_Reg if_id;  // sits between IF and ID
ID_EX_Reg id_ex;  // sits between ID and EX
```
Each has a `valid` flag (1 = real instruction, 0 = bubble/NOP).

---

### 9. `printer.c` — Output
**Role:** Prints the final state of the entire system after the last cycle.

**Prints:**
- All 64 registers (R0–R63) with their values
- SREG flag breakdown: `C=0 V=0 N=0 S=0 Z=0`
- PC value
- All non-zero instruction memory entries
- All non-zero data memory entries

---

## Execution Timeline — `test1.txt` (11 instructions)

```
Cycle:  1    2    3    4    5    6    7    8    9   10   11   12   13
LDI R1: IF   ID   EX
LDI R2:      IF   ID   EX
ADD R1:           IF   ID   EX
SUB R1:                IF   ID   EX
MUL R1:                     IF   ID   EX
AND R1:                          IF   ID   EX
OR  R1:                               IF   ID   EX
SAL R1:                                    IF   ID   EX
SAR R1:                                         IF   ID   EX
SW  R1:                                              IF   ID   EX
LW  R3:                                                   IF   ID   EX
```

**Total = 13 cycles** (formula: 3 + (n−1) = 3 + 10 = 13) ✅

---

## Final Verified Results for `test1.txt`

| Step | Instruction | Operation | R1 value |
|---|---|---|---|
| 1 | LDI R1 5 | R1 = 5 | 5 |
| 2 | LDI R2 3 | R2 = 3 | 5 |
| 3 | ADD R1 R2 | R1 = 5+3 | 8 |
| 4 | SUB R1 R2 | R1 = 8-3 | 5 |
| 5 | MUL R1 R2 | R1 = 5×3 | 15 |
| 6 | AND R1 R2 | R1 = 15&3 | 3 |
| 7 | OR R1 R2 | R1 = 3\|3 | 3 |
| 8 | SAL R1 2 | R1 = 3<<2 | 12 |
| 9 | SAR R1 1 | R1 = 12>>1 | **6** ✅ |
| 10 | SW R1 0 | DataMem[6+0] = 6 | 6 |
| 11 | LW R3 0 | R3 = DataMem[0+0] = 0 | — |
