import os
from PIL import Image, ImageDraw, ImageFont

def render_ui_screenshot(output_path, is_flush=False):
    width, height = 1600, 960
    img = Image.new("RGBA", (width, height), (11, 17, 32, 255))
    draw = ImageDraw.Draw(img)

    # Fonts
    font_sans = "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf"
    font_sans_b = "/usr/share/fonts/truetype/ubuntu/Ubuntu-B.ttf"
    font_mono = "/usr/share/fonts/truetype/ubuntu/UbuntuMono[wght].ttf"
    font_mono_b = "/usr/share/fonts/truetype/ubuntu/UbuntuMono-B.ttf"

    f_title = ImageFont.truetype(font_sans_b, 19)
    f_badge = ImageFont.truetype(font_sans_b, 12)
    f_btn = ImageFont.truetype(font_sans_b, 13)
    f_sec_h = ImageFont.truetype(font_sans_b, 13)
    f_body = ImageFont.truetype(font_sans, 13)
    f_mono = ImageFont.truetype(font_mono, 13)
    f_mono_b = ImageFont.truetype(font_mono_b, 15)
    f_mono_card = ImageFont.truetype(font_mono_b, 16)
    f_mono_sm = ImageFont.truetype(font_mono, 12)
    f_big = ImageFont.truetype(font_mono_b, 26)
    f_banner = ImageFont.truetype(font_sans_b, 14)

    # Palette
    c_bg0 = (11, 17, 32)
    c_bg1 = (15, 23, 42)
    c_bg2 = (30, 41, 59)
    c_bg3 = (51, 65, 85)
    c_cyan = (6, 182, 212)
    c_teal = (20, 184, 166)
    c_green = (34, 197, 94)
    c_yellow = (234, 179, 8)
    c_orange = (249, 115, 22)
    c_red = (239, 68, 68)
    c_t1 = (241, 245, 249)
    c_t2 = (148, 163, 184)
    c_t3 = (100, 116, 139)
    c_brd = (51, 65, 85)

    # 1. Header (y=0 to 56)
    draw.rectangle([(0, 0), (width, 56)], fill=c_bg2)
    draw.line([(0, 56), (width, 56)], fill=c_brd, width=1)

    # Logo
    draw.rounded_rectangle([(16, 12), (48, 44)], radius=6, fill=(6, 182, 212, 45), outline=c_cyan, width=2)
    draw.rectangle([(23, 19), (41, 37)], outline=c_cyan, width=2)
    draw.rectangle([(28, 24), (36, 32)], fill=c_cyan)
    draw.text((58, 16), "Harvard Simulator", font=f_title, fill=c_t1)
    draw.rounded_rectangle([(224, 17), (274, 39)], radius=11, fill=c_cyan)
    draw.text((233, 21), "Pkg 4", font=f_badge, fill=(0, 0, 0))

    # Context Header Pill
    context_text = "SCENARIO: Control Hazard (Taken Branch Flushes Pipeline)" if is_flush else "SCENARIO: Cycle-by-Cycle Pipeline Execution & Data Forwarding"
    pill_color = (239, 68, 68, 35) if is_flush else (34, 197, 94, 35)
    pill_border = c_red if is_flush else c_green
    pill_text_color = c_red if is_flush else c_green
    draw.rounded_rectangle([(300, 13), (760, 43)], radius=8, fill=pill_color, outline=pill_border, width=1)
    draw.text((315, 19), context_text, font=f_banner, fill=pill_text_color)

    # Toolbar
    tb_x, tb_y = 800, 11
    draw.rounded_rectangle([(tb_x, tb_y), (tb_x + 360, tb_y + 34)], radius=8, fill=c_bg3)
    draw.text((tb_x + 14, tb_y + 8), "↺ Reset", font=f_btn, fill=c_t2)
    draw.rounded_rectangle([(tb_x + 85, tb_y + 4), (tb_x + 165, tb_y + 30)], radius=6, fill=c_cyan)
    draw.text((tb_x + 104, tb_y + 8), "▶ Run", font=f_btn, fill=(0, 0, 0))
    draw.rounded_rectangle([(tb_x + 175, tb_y + 4), (tb_x + 250, tb_y + 30)], radius=6, fill=c_bg2, outline=c_brd)
    draw.text((tb_x + 192, tb_y + 8), "⏭ Step", font=f_btn, fill=c_t1)
    draw.text((tb_x + 265, tb_y + 9), "Speed", font=f_badge, fill=c_t3)
    draw.rounded_rectangle([(tb_x + 305, tb_y + 14), (tb_x + 348, tb_y + 18)], radius=2, fill=c_cyan)
    draw.ellipse([(tb_x + 322, tb_y + 11), (tb_x + 332, tb_y + 21)], fill=c_t1)

    # Cycle counter badge
    cycle_str = "Cycle 7" if is_flush else "Cycle 6"
    draw.rounded_rectangle([(width - 150, 10), (width - 20, 46)], radius=8, fill=c_bg3, outline=c_teal, width=2)
    draw.text((width - 132, 16), cycle_str, font=f_mono_b, fill=c_teal)

    # 2. Main content area (y=66 to 770)
    p_code = [(12, 66), (360, 770)]
    p_pipe = [(372, 66), (1030, 770)]
    p_metr = [(1042, 66), (1270, 770)]
    p_state = [(1282, 66), (1588, 770)]

    for p in [p_code, p_pipe, p_metr, p_state]:
        draw.rounded_rectangle(p, radius=10, fill=c_bg2, outline=c_brd, width=1)

    def draw_phead(panel, title, right_text=""):
        (x1, y1), (x2, _) = panel
        draw.rounded_rectangle([(x1, y1), (x2, y1 + 38)], radius=10, fill=c_bg2)
        draw.rectangle([(x1, y1 + 28), (x2, y1 + 38)], fill=c_bg2)
        draw.line([(x1, y1 + 38), (x2, y1 + 38)], fill=c_brd, width=1)
        draw.text((x1 + 14, y1 + 11), title.upper(), font=f_sec_h, fill=c_t2)
        if right_text:
            draw.text((x2 - 80, y1 + 12), right_text, font=f_badge, fill=c_teal)

    draw_phead(p_code, "Assembly Program")
    draw.rounded_rectangle([(p_code[1][0] - 110, 74), (p_code[1][0] - 10, 98)], radius=6, fill=c_cyan)
    draw.text((p_code[1][0] - 98, 79), "Parse & Load", font=f_badge, fill=(0, 0, 0))

    draw_phead(p_pipe, "3-Stage Execution Pipeline")
    draw_phead(p_metr, "Performance", "C Model")
    draw_phead(p_state, "Processor State")

    # --- CODE PANEL ---
    draw.rounded_rectangle([(20, 112), (352, 142)], radius=6, fill=c_bg0, outline=c_brd)
    preset_label = "Preset: Control Hazard Flush" if is_flush else "Preset: Comprehensive Test"
    draw.text((30, 120), preset_label, font=f_mono_sm, fill=c_t1)

    draw.rounded_rectangle([(20, 150), (352, 758)], radius=6, fill=c_bg0, outline=c_brd)
    draw.rectangle([(20, 150), (55, 758)], fill=c_bg1)
    draw.line([(55, 150), (55, 758)], fill=c_brd)

    code_lines = [
        "LDI R1 0",
        "BEQZ R1 2",
        "LDI R2 15",
        "LDI R3 15",
        "LDI R4 25",
        "ADD R4 R4",
        "SB R4 10",
        "LB R5 10"
    ] if is_flush else [
        "LDI R1 10",
        "LDI R2 3",
        "LDI R3 10",
        "ADD R3 R2",
        "LDI R4 10",
        "SUB R4 R2",
        "LDI R5 10",
        "MUL R5 R2",
        "LDI R6 10",
        "AND R6 R2",
        "LDI R7 10",
        "OR R7 R2",
        "LDI R8 10",
        "SAL R8 2",
        "SAR R9 1",
        "SB R5 5",
        "LB R11 5",
        "BEQZ R10 1",
        "LDI R12 1",
        "LDI R10 0"
    ]

    for i, line in enumerate(code_lines[:20]):
        y = 160 + i * 28
        is_active = (i == 1) if is_flush else (i == 3)
        if is_active:
            draw.rectangle([(21, y - 2), (351, y + 24)], fill=(239, 68, 68, 50) if is_flush else (6, 182, 212, 45))
        draw.text((28, y + 2), str(i + 1), font=f_mono_sm, fill=c_t3)
        parts = line.split(" ")
        mnemonic = parts[0]
        operands = " ".join(parts[1:])
        m_color = c_yellow if mnemonic in ["BEQZ", "JR"] else (c_teal if mnemonic in ["LB", "SB"] else c_cyan)
        draw.text((65, y + 2), mnemonic, font=f_mono_b, fill=m_color)
        draw.text((125, y + 2), operands, font=f_mono, fill=c_t1)

    # --- PIPELINE PANEL ---
    card_y = 115
    stages = [
        ("IF", "Instruction Fetch", c_green, 385),
        ("ID", "Instruction Decode", c_yellow, 600),
        ("EX", "Execute / Writeback", c_orange, 815)
    ]

    for code, label, col, cx in stages:
        draw.text((cx + 10, card_y), code, font=f_title, fill=col)
        draw.text((cx + 40, card_y + 3), f"· {label}", font=f_badge, fill=c_t3)

    # Arrows between cards
    draw.text((574, card_y + 60), "➔", font=f_title, fill=c_t3)
    draw.text((789, card_y + 60), "➔", font=f_title, fill=c_t3)

    if is_flush:
        # Card IF: FLUSHED
        draw.rounded_rectangle([(385, card_y + 30), (565, card_y + 155)], radius=10, fill=(239, 68, 68, 25), outline=c_red, width=2)
        draw.rounded_rectangle([(400, card_y + 42), (550, card_y + 68)], radius=6, fill=c_red)
        draw.text((418, card_y + 46), "✕ FLUSHED", font=f_mono_card, fill=(255, 255, 255))
        draw.text((400, card_y + 80), "Younger than branch", font=f_mono_sm, fill=c_t2)
        draw.text((400, card_y + 102), "PC redirected to 4", font=f_mono_b, fill=c_teal)
        draw.text((400, card_y + 125), "Next fetch: [PC=4]", font=f_mono_sm, fill=c_green)

        # Card ID: FLUSHED
        draw.rounded_rectangle([(600, card_y + 30), (780, card_y + 155)], radius=10, fill=(239, 68, 68, 25), outline=c_red, width=2)
        draw.rounded_rectangle([(615, card_y + 42), (765, card_y + 68)], radius=6, fill=c_red)
        draw.text((633, card_y + 46), "✕ FLUSHED", font=f_mono_card, fill=(255, 255, 255))
        draw.text((615, card_y + 80), "Instruction in ID discarded", font=f_mono_sm, fill=c_t2)
        draw.text((615, card_y + 102), "LDI R2 15 canceled", font=f_mono_b, fill=c_yellow)
        draw.text((615, card_y + 125), "R2 remains 0 (unmodified)", font=f_mono_sm, fill=c_green)

        # Card EX: Branch Taken
        draw.rounded_rectangle([(815, card_y + 30), (995, card_y + 155)], radius=10, fill=(249, 115, 22, 25), outline=c_orange, width=2)
        draw.text((828, card_y + 42), "[PC=1] BEQZ R1 2", font=f_mono_card, fill=c_cyan)
        draw.text((828, card_y + 68), "Condition: R1 == 0 (TRUE)", font=f_mono_sm, fill=c_green)
        draw.text((828, card_y + 92), "Output: Branch TAKEN", font=f_mono_b, fill=c_red)
        draw.text((828, card_y + 115), "Target PC = 1 + 1 + 2 = 4", font=f_mono_b, fill=c_yellow)
        draw.text((828, card_y + 135), "Flushed 2 younger stages", font=f_mono_sm, fill=c_t3)
    else:
        # Card IF: Fetched
        draw.rounded_rectangle([(385, card_y + 30), (565, card_y + 155)], radius=10, fill=(34, 197, 94, 25), outline=c_green, width=2)
        draw.text((400, card_y + 42), "[PC=4] LDI R4 10", font=f_mono_card, fill=c_cyan)
        draw.text((400, card_y + 68), "Raw: 0x310A (I-type)", font=f_mono_sm, fill=c_t2)
        draw.text((400, card_y + 95), "Status: Valid Fetch", font=f_mono_b, fill=c_green)
        draw.text((400, card_y + 122), "PC incremented: 4 → 5", font=f_mono_sm, fill=c_t3)

        # Card ID: Decoded
        draw.rounded_rectangle([(600, card_y + 30), (780, card_y + 155)], radius=10, fill=(234, 179, 8, 25), outline=c_yellow, width=2)
        draw.text((615, card_y + 42), "[PC=3] ADD R3 R2", font=f_mono_card, fill=c_cyan)
        draw.text((615, card_y + 68), "Decoded: ADD (R-type)", font=f_mono_sm, fill=c_t2)
        draw.text((615, card_y + 95), "Inputs: R3=10, R2=3", font=f_mono_b, fill=c_yellow)
        draw.text((615, card_y + 122), "Forwarded R3 from EX!", font=f_mono_b, fill=c_cyan)

        # Card EX: Executing
        draw.rounded_rectangle([(815, card_y + 30), (995, card_y + 155)], radius=10, fill=(249, 115, 22, 25), outline=c_orange, width=2)
        draw.text((828, card_y + 42), "[PC=2] LDI R3 10", font=f_mono_card, fill=c_cyan)
        draw.text((828, card_y + 68), "Inputs: IMM = 10", font=f_mono_sm, fill=c_t2)
        draw.text((828, card_y + 95), "Output: R3 = 10", font=f_mono_b, fill=c_green)
        draw.text((828, card_y + 122), "Written to register file", font=f_mono_sm, fill=c_t3)

    # Console Section below pipeline cards
    draw.rounded_rectangle([(385, 290), (1015, 758)], radius=8, fill=c_bg0, outline=c_brd)
    draw.rectangle([(385, 290), (1015, 322)], fill=(0, 0, 0, 110))
    draw.text((400, 298), "SIMULATOR CYCLE LOG TRACE", font=f_badge, fill=c_t3)

    log_entries = [
        ("══ Cycle 7 ══", c_cyan),
        ("  [EX Stage] [PC=1] BEQZ R1 2", c_t1),
        ("       Inputs: R1_val=0, IMM=2", c_t2),
        ("       Output: Branch TAKEN → Target PC = 1 + 1 + 2 = 4", c_orange),
        ("  ⚠ CONTROL HAZARD: Branch taken flushes younger pipeline stages", c_red),
        ("  [ID Stage] Instruction: [PC=2] LDI R2 15 → FLUSHED", c_red),
        ("  [IF Stage] Instruction: [PC=3] LDI R3 15 → FLUSHED", c_red),
        ("  [EX] Program Counter redirected to 4", c_teal),
        ("══ Cycle 8 ══", c_cyan),
        ("  [EX Stage] [bubble]", c_t3),
        ("  [ID Stage] [bubble]", c_t3),
        ("  [IF Stage] Fetched target: [PC=4] LDI R4 25 (raw=0x3119)", c_green),
        ("       PC incremented: 4 → 5", c_t2)
    ] if is_flush else [
        ("══ Cycle 5 ══", c_cyan),
        ("  [EX Stage] [PC=2] LDI R3 10  Output: R3 = 10", c_t1),
        ("  [ID Stage] [PC=3] ADD R3 R2  Decoded: R3=10 (same-cycle forwarded), R2=3", c_yellow),
        ("  [IF Stage] [PC=4] LDI R4 10  Fetched raw=0x310A", c_green),
        ("══ Cycle 6 ══", c_cyan),
        ("  [EX Stage] [PC=3] ADD R3 R2", c_t1),
        ("       Inputs: R3=10, R2=3  Output: R3 = 13", c_green),
        ("       [EX] R3 changed to 13", c_cyan),
        ("       [EX] SREG: C=0, V=0, N=0, S=0, Z=0", c_t2),
        ("  [ID Stage] [PC=4] LDI R4 10  Decoded: R4, IMM=10", c_yellow),
        ("  [IF Stage] [PC=5] SUB R4 R2  Fetched raw=0x1102", c_green),
        ("       PC incremented: 5 → 6", c_t3)
    ]

    for j, (txt, col) in enumerate(log_entries):
        draw.text((400, 334 + j * 24), txt, font=f_mono_sm, fill=col)

    # --- METRICS PANEL ---
    m_boxes = [
        ("CYCLES", "7" if is_flush else "6", c_teal),
        ("RETIRED", "2" if is_flush else "4", c_teal),
        ("CPI", "3.50" if is_flush else "1.50", c_teal),
        ("STALLS", "0", c_green),
        ("FLUSHES", "1" if is_flush else "0", c_red if is_flush else c_t3)
    ]

    for idx, (m_lbl, m_val, m_col) in enumerate(m_boxes):
        bx = 1056 + (idx % 2) * 102
        by = 120 + (idx // 2) * 75
        if idx == 4:
            bx = 1056
            by = 120 + 2 * 75
        draw.rounded_rectangle([(bx, by), (bx + 94, by + 65)], radius=6, fill=c_bg0, outline=c_brd)
        draw.text((bx + 10, by + 10), m_lbl, font=f_badge, fill=c_t3)
        draw.text((bx + 10, by + 30), m_val, font=f_big, fill=m_col)

    # Timing Diagram
    draw.text((1056, 360), "INSTRUCTION TIMING", font=f_sec_h, fill=c_t2)
    draw.rounded_rectangle([(1056, 385), (1256, 758)], radius=6, fill=c_bg0, outline=c_brd)

    cycles_show = 8
    for cy in range(1, cycles_show + 1):
        row_y = 398 + (cy - 1) * 36
        draw.text((1068, row_y + 4), f"C{cy}", font=f_mono_sm, fill=c_t3)
        stages_data = [
            ("F", c_green, 1100),
            ("D", c_yellow, 1145),
            ("E", c_orange, 1190)
        ]
        for ch, col, scx in stages_data:
            draw.rounded_rectangle([(scx, row_y), (scx + 36, row_y + 26)], radius=4, fill=(col[0], col[1], col[2], 28), outline=col)
            draw.text((scx + 13, row_y + 5), ch, font=f_mono_b, fill=col)

    # --- STATE PANEL ---
    # PC and SREG
    draw.rounded_rectangle([(1296, 120), (1426, 195)], radius=6, fill=c_bg0, outline=c_brd)
    draw.text((1310, 130), "PROGRAM COUNTER", font=f_badge, fill=c_t3)
    pc_val_str = "4" if is_flush else "6"
    draw.text((1340, 150), pc_val_str, font=f_big, fill=c_teal)

    draw.rounded_rectangle([(1438, 120), (1574, 195)], radius=6, fill=c_bg0, outline=c_brd)
    draw.text((1450, 130), "SREG FLAGS", font=f_badge, fill=c_t3)
    flags = [("C", False), ("V", False), ("N", False), ("S", False), ("Z", is_flush)]
    for fi, (fl, is_on) in enumerate(flags):
        fx = 1448 + fi * 24
        fy = 155
        f_bg = (34, 197, 94, 45) if is_on else c_bg3
        f_col = c_green if is_on else c_t3
        draw.rounded_rectangle([(fx, fy), (fx + 20, fy + 24)], radius=4, fill=f_bg, outline=f_col)
        draw.text((fx + 5, fy + 4), fl, font=f_mono_b, fill=f_col)

    # Registers R0..R63 grid
    draw.text((1296, 215), "GENERAL-PURPOSE REGISTERS (R0–R63)", font=f_sec_h, fill=c_t2)
    draw.rounded_rectangle([(1296, 240), (1574, 758)], radius=6, fill=c_bg0, outline=c_brd)

    cols = 4
    rows = 16
    for r in range(rows):
        for c in range(cols):
            idx = r * cols + c
            rx = 1306 + c * 65
            ry = 250 + r * 30
            is_ch = (idx in [1, 4]) if is_flush else (idx in [1, 2, 3])
            val = "0"
            if is_flush:
                if idx == 1: val = "0"
                elif idx == 4: val = "25"
            else:
                if idx == 1: val = "10"
                elif idx == 2: val = "3"
                elif idx == 3: val = "13"

            bg = (6, 182, 212, 35) if is_ch else c_bg1
            outl = c_cyan if is_ch else c_brd
            draw.rounded_rectangle([(rx, ry), (rx + 60, ry + 24)], radius=4, fill=bg, outline=outl)
            draw.text((rx + 4, ry + 5), f"R{idx}", font=f_mono_sm, fill=c_t3 if not is_ch else c_cyan)
            draw.text((rx + 36, ry + 5), val, font=f_mono_b, fill=c_t1)

    # 3. Bottom Memory Subsystem Panel (y=780 to 945)
    draw.rounded_rectangle([(12, 780), (1588, 945)], radius=10, fill=c_bg2, outline=c_brd)
    draw.text((26, 795), "HARVARD MEMORY SUBSYSTEM (DATA MEMORY: 2048 BYTES | INSTRUCTION MEMORY: 1024 WORDS)", font=f_sec_h, fill=c_t2)

    mem_vals = [(0, 10), (1, 20), (2, 30), (5, 30), (10, 25), (12, 25)] if not is_flush else [(5, 27), (10, 25)]
    for mi, (addr, val) in enumerate(mem_vals):
        mx = 26 + mi * 130
        my = 825
        draw.rounded_rectangle([(mx, my), (mx + 115, my + 50)], radius=6, fill=c_bg0, outline=c_teal)
        draw.text((mx + 10, my + 8), f"DataMem[{addr}]", font=f_mono_sm, fill=c_t3)
        draw.text((mx + 10, my + 26), f"= {val} (0x{val:02X})", font=f_mono_b, fill=c_teal)

    draw.text((width - 320, 915), "Harvard Architecture · 16-bit ISA · 3-Stage Pipeline", font=f_mono_sm, fill=c_t3)

    img.save(output_path, "PNG")
    print(f"Generated {output_path}")

if __name__ == "__main__":
    os.makedirs("screenshots", exist_ok=True)
    render_ui_screenshot("screenshots/pipeline_simulator_ui.png", is_flush=False)
    render_ui_screenshot("screenshots/branch_flush_hazard.png", is_flush=True)
