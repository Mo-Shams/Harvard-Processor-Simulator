# Harvard Processor Simulator

A cycle-accurate **Harvard-architecture pipelined processor simulator** built in C, with an interactive web-based GUI for real-time pipeline visualization.

## Overview

This simulator models a 16-bit Harvard-architecture processor featuring a **3-stage pipeline** (IF → ID → EX), separate instruction and data memories, 64 general-purpose registers, and a full status register with condition flags. It parses assembly programs, encodes them into binary, and executes them cycle-by-cycle with accurate hazard detection and resolution.

## Architecture

| Property | Specification |
|---|---|
| Architecture | Harvard (separate instruction & data memories) |
| Pipeline | 3-stage — Fetch, Decode, Execute |
| Instruction Size | 16-bit |
| Registers | 64 × 8-bit GPRs, 8-bit SREG, 16-bit PC |
| Instruction Memory | 1024 × 16-bit (word-addressable) |
| Data Memory | 2048 × 8-bit (byte-addressable) |
| Instruction Count | 12 |

## Features

- **Assembly Parser** — Reads `.txt` assembly files and encodes instructions into 16-bit binary
- **3-Stage Pipeline Simulation** — Cycle-accurate IF → ID → EX execution
- **Hazard Detection & Resolution** — Data forwarding and pipeline stalling for RAW hazards
- **Control Hazard Handling** — Branch/jump detection with pipeline flushing
- **Status Register Flags** — Carry, Overflow, Negative, Sign, and Zero flags
- **Full ISA Support** — `ADD`, `SUB`, `MUL`, `AND`, `OR`, `LDI`, `BEQZ`, `JR`, `SAL`, `SAR`, `LW`, `SW`
- **Interactive Web GUI** — Real-time pipeline visualization with step-through and auto-play controls

## Tech Stack

| Component | Technology |
|---|---|
| Simulator Backend | C |
| GUI Frontend | HTML, CSS, JavaScript |
| Build System | GCC + Make |

## Project Structure

```
Harvard-Processor-Simulator/
├── include/            # Header files
│   ├── types.h         # Data types & constants
│   ├── memory.h        # Memory interface
│   ├── registers.h     # Register file interface
│   ├── parser.h        # Assembly parser interface
│   ├── decoder.h       # Instruction decoder interface
│   ├── alu.h           # ALU operations interface
│   ├── pipeline.h      # Pipeline simulation interface
│   ├── hazard.h        # Hazard detection interface
│   └── printer.h       # Output interface
├── src/                # Source files
│   ├── main.c          # Entry point
│   ├── memory.c        # Memory implementation
│   ├── registers.c     # Register file
│   ├── parser.c        # Assembly parser
│   ├── decoder.c       # Instruction decoder
│   ├── alu.c           # ALU operations
│   ├── pipeline.c      # Pipeline simulation
│   ├── hazard.c        # Hazard detection & resolution
│   └── printer.c       # Output formatting
├── gui/                # Web-based GUI
├── programs/           # Sample assembly programs
└── README.md
```

## Getting Started

### Prerequisites

- GCC compiler
- Make

### Build

```bash
make
```

### Run

```bash
./simulator.exe <path_to_assembly_file>
```

### Example

```bash
./simulator.exe programs/test1.txt
```

Sample assembly program (`programs/test1.txt`):

```asm
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

## Collaborators

| Name | GitHub |
|---|---|
| Mohamed Enan | [@Mo7amed3nan](https://github.com/Mo7amed3nan) |
| Ziad Abdou | [@Ziad-Abdou1](https://github.com/Ziad-Abdou1) |
| Mohamed Shams | [@Mo-Shams](https://github.com/Mo-Shams) |
| Adham Jalaal | [@profadham](https://github.com/profadham) |