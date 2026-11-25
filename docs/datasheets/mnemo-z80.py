"""Построение памяток ВМ80А в синтаксисе Z80 и Intel и проверка их машинного кода."""

import argparse
from functools import lru_cache
from pathlib import Path
import shutil
import subprocess
import tempfile

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[2]
REG = ("B", "C", "D", "E", "H", "L", "(HL)", "A")
PAIR = ("BC", "DE", "HL", "SP")
STACK = ("BC", "DE", "HL", "AF")
COND = ("NZ", "Z", "NC", "C", "PO", "PE", "P", "M")
ALU = ("ADD A,", "ADC A,", "SUB ", "SBC A,", "AND ", "XOR ", "OR ", "CP ")
DUP = {0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 0xCB, 0xD9, 0xDD, 0xED, 0xFD}


def instruction(code):
    row, col = divmod(code, 8)
    if code < 0x40:
        pair = PAIR[row // 2]
        if col == 0:
            return ("NOP", "duplicate" if code else "system")
        if col == 1:
            return (f"ADD HL,{pair}", "widealu") if row % 2 else (f"LD {pair},nn", "word")
        if col == 2:
            name = ("LD (BC),A", "LD A,(BC)", "LD (DE),A", "LD A,(DE)",
                    "LD (nn),HL", "LD HL,(nn)", "LD (nn),A", "LD A,(nn)")[row]
            return name, "word" if row in (4, 5) else "byte"
        if col == 3:
            return ("DEC " if row % 2 else "INC ") + pair, "alu"
        if col in (4, 5):
            return ("INC " if col == 4 else "DEC ") + REG[row], "alu"
        if col == 6:
            return "LD " + REG[row] + ",n", "byte"
        return ("RLCA", "RRCA", "RLA", "RRA", "DAA", "CPL", "SCF", "CCF")[row], "rotate" if row < 4 else "alu"
    if code < 0x80:
        if code == 0x76:
            return "HALT", "system"
        return f"LD {REG[row % 8]},{REG[col]}", "byte"
    if code < 0xC0:
        return ALU[row % 8] + REG[col], "compare" if row % 8 == 7 else "alu"
    k = row % 8
    if col == 0:
        return "RET " + COND[k], "branch"
    if col == 1:
        if k % 2 == 0:
            return "POP " + STACK[k // 2], "word"
        return (("RET", "branch"), ("RET", "duplicate"), ("JP (HL)", "branch"), ("LD SP,HL", "word"))[k // 2]
    if col == 2:
        return f"JP {COND[k]},nn", "branch"
    if col == 3:
        return (("JP nn", "branch"), ("JP nn", "duplicate"), ("OUT (n),A", "io"), ("IN A,(n)", "io"),
                ("EX (SP),HL", "word"), ("EX DE,HL", "word"), ("DI", "system"), ("EI", "system"))[k]
    if col == 4:
        return f"CALL {COND[k]},nn", "branch"
    if col == 5:
        if k % 2 == 0:
            return "PUSH " + STACK[k // 2], "word"
        return "CALL nn", "branch" if k == 1 else "duplicate"
    if col == 6:
        return ALU[k] + "n", "compare" if k == 7 else "alu"
    return f"RST {k * 8:02X}h", "branch"


Z80_TABLE = [instruction(i) for i in range(256)]
TASM_TABLE = ROOT / "tasm" / "TASM85.TAB"


def intel_table():
    by_code = {}
    for line in TASM_TABLE.read_text(encoding="ascii").splitlines():
        if not line or not line[0].isalpha():
            continue
        op, arg, opcode, size, *_ = line.split()
        if op in ("RIM", "SIM"):
            continue
        arg = arg.replace('""', "").replace("*", "nn" if size == "3" else "n")
        by_code[int(opcode, 16)] = f"{op} {arg}".rstrip()
    assert set(by_code) == set(range(256)) - DUP
    # Подпись дубля берём у документированного кода с тем же действием.
    canonical = {asm: i for i, (asm, group) in enumerate(Z80_TABLE) if group != "duplicate"}
    return [(by_code[canonical[asm]], group) for asm, group in Z80_TABLE]


def check_assembly():
    byte_operands = {*range(0x06, 0x40, 8), *range(0xC6, 0x100, 8), 0xD3, 0xDB}
    word_operands = {0x01, 0x11, 0x21, 0x31, 0x22, 0x2A, 0x32, 0x3A,
                     0xC3, 0xCD, *range(0xC2, 0x100, 8), *range(0xC4, 0x100, 8)}
    assert {i for i, (_, group) in enumerate(TABLE) if group == "duplicate"} == DUP
    assert len({asm for asm, group in TABLE if group != "duplicate"}) == 244
    source, expected = ["    .ORG 0" if intel else "    ORG 0"], bytearray()
    for code, (asm, group) in enumerate(TABLE):
        if group == "duplicate":
            continue
        source.append("    " + asm.replace("nn", "1234h").replace("n", "56h"))
        expected.append(code)
        if code in byte_operands:
            expected.append(0x56)
        if code in word_operands:
            expected.extend((0x34, 0x12))
    with tempfile.TemporaryDirectory() as temp:
        work = Path(temp)
        if intel:
            source.append("    .END")
            shutil.copyfile(TASM_TABLE, work / TASM_TABLE.name)
            cmd = [ROOT / "tasm" / "TASM.EXE", "-85", "-b", "mnemo.asm", "mnemo.bin"]
        else:
            cmd = [ROOT / "tasm" / "sjasmplus.exe", "--i8080", "--raw=mnemo.bin", "mnemo.asm"]
        (work / "mnemo.asm").write_text("\n".join(source) + "\n", encoding="ascii")
        subprocess.run(cmd, check=True, cwd=work)
        actual = (work / "mnemo.bin").read_bytes()
        if actual != expected:
            raise ValueError(f"Расхождение выходного кода ({len(actual)} / {len(expected)} байт)")
    print(f"244 opcodes, {len(expected)} bytes, 12 duplicates")


SCALE = 3
W, H = 2020, 1000
dark_theme = False
intel = False
png = False
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dark", action="store_true", help="построить ночную версию памятки")
    parser.add_argument("--syntax", choices=("z80", "i8080"), default="z80", help="система мнемоник")
    parser.add_argument("--png", action="store_true", help="сохранить Z80 в PNG (для i8080 PNG используется всегда)")
    args = parser.parse_args()
    dark_theme = args.dark
    intel = args.syntax == "i8080"
    png = args.png or intel
TABLE = intel_table() if intel else Z80_TABLE
sheet_name = "Mnemo_i8080" if intel else "Mnemo_Z80"
OUT = ROOT / "datasheets" / (sheet_name + ("_dark" if dark_theme else "") + (".png" if png else ".jpg"))


def syntax(z80, i8080):
    return i8080 if intel else z80


def tone(day, night):
    return night if dark_theme else day


paper       = tone("#f8f9f6", "#232527")
surface     = tone("#ffffff", "#2a2d30")
ink         = tone("#19292b", "#eff0eb")
muted       = tone("#445759", "#c9cecc")
rule        = tone("#aab6b4", "#60696b")
axis_fill   = tone("#19292b", "#1b1d1f")
axis_ink    = tone("#fcfcf7", "#e5eae6")
grid_major  = tone("#40514f", "#929b9d")
grid_minor  = tone("#81918a", "#647073")
neutral_bar = tone("#e3e8e3", "#3b4244")
flag_fixed  = tone("#edf1ed", "#373b3e")
flag_active = tone("#dce6de", "#3b5148")
# Для каждой темы пара задаёт заливку ячейки и акцент заголовка раздела.
colors = {
    "byte":      tone(("#b7d9bd", "#377c50"), ("#2b4939", "#86b995")),
    "word":      tone(("#f5df85", "#926d0f"), ("#564821", "#c9ac60")),
    "io":        tone(("#9fd3e6", "#147293"), ("#214952", "#79b6c1")),
    "alu":       tone(("#fbfcf9", "#536560"), ("#2b2f32", "#9faeaa")),
    "compare":   tone(("#9ac9e8", "#256d9e"), ("#2c4662", "#84b4d8")),
    "widealu":   tone(("#d3c2e1", "#79588e"), ("#483c57", "#b3a0c9")),
    "rotate":    tone(("#edb4d0", "#a34876"), ("#58394d", "#d49cba")),
    "system":    tone(("#cce3ea", "#46798b"), ("#354852", "#9db9c8")),
    "branch":    tone(("#edb4a0", "#a54d33"), ("#593b32", "#d9a08b")),
    "duplicate": tone(("#d6dada", "#677673"), ("#46494b", "#acb0ae")),
}
img = Image.new("RGB", (W * SCALE, H * SCALE), paper)
draw = ImageDraw.Draw(img)
fonts = {"code": "consolab.ttf", "regular": "consola.ttf",
         "body": "segoeui.ttf", "semibold": "seguisb.ttf", "bold": "segoeuib.ttf"}


@lru_cache(None)
def font(face, size):
    return ImageFont.truetype(str(Path("C:/Windows/Fonts") / fonts[face]), round(size * SCALE))


def rect(x, y, w, h, fill):
    box = (round(x*SCALE), round(y*SCALE), round((x+w)*SCALE)-1, round((y+h)*SCALE)-1)
    draw.rectangle(box, fill=fill)


def label(x, y, s, size=14, face="body", color=ink, align="l", limit=None, fit=False):
    f = font(face, size)
    if limit is not None:
        while fit and draw.textlength(s, font=f) > limit*SCALE and size > 12:
            size -= 0.25
            f = font(face, size)
        if draw.textlength(s, font=f) > limit*SCALE:
            raise ValueError(f"Текст выходит за поле {limit}: {s} ({draw.textlength(s, font=f)/SCALE:.1f})")
    draw.text((round(x*SCALE), round(y*SCALE)), s, font=f, fill=color, anchor=align+"m")


LEFT, SECOND, CW = 26, 496, 460


def panel(x, y, h, title, group, timing=True):
    rect(x, y, CW, h, rule)
    rect(x+0.8, y+0.8, CW-1.6, h-1.6, surface)
    fill, accent = colors[group]
    rect(x+0.8, y+0.8, CW-1.6, 28, fill if group != "alu" else neutral_bar)
    rect(x, y, 4, 29, accent)
    label(x+12, y+14, title, 16.7, "bold", limit=CW-90)
    if timing:
        label(x+CW-12, y+15, "ТАКТЫ", 11, "bold", align="r")


def rows(x, y, items, step, pos=195, descsize=14.0, codesize=15.6):
    for i, (asm, action, ticks) in enumerate(items):
        cy = y + i*step
        label(x+12, cy, asm, codesize, "code", limit=pos-20)
        label(x+pos, cy, action, descsize, limit=CW-pos-60)
        label(x+CW-13, cy, ticks, 15.5, "code", align="r", limit=47)


def notes(x, y, items, step=21, pos=86):
    for i, (symbol, meaning) in enumerate(items):
        label(x+12, y+i*step, symbol, 15, "code", limit=pos-19)
        label(x+pos, y+i*step, meaning, 14, limit=CW-pos-11)


def left():
    panel(LEFT, 100, 143, "Пересылки · 8 бит", "byte")
    rows(LEFT, 139, [
        (syntax("LD r,r′",               "MOV r,r′"),          "r ← r′",              "5"),
        (syntax("LD r,(HL) / LD (HL),r", "MOV r,M / MOV M,r"), "r ← [HL] / [HL] ← r", "7"),
        (syntax("LD r,n / LD (HL),n",    "MVI r,n / MVI M,n"), "r ← n / [HL] ← n",    "7/10"),
        (syntax("LD A,(BC) / LD (BC),A", "LDAX B / STAX B"),   "A ← [BC] / [BC] ← A", "7"),
        (syntax("LD A,(DE) / LD (DE),A", "LDAX D / STAX D"),   "A ← [DE] / [DE] ← A", "7"),
        (syntax("LD A,(nn) / LD (nn),A", "LDA nn / STA nn"),   "A ← [nn] / [nn] ← A", "13"),
    ], 18.5, pos=216)
    panel(LEFT, 250, 66, "Ввод и вывод", "io")
    rows(LEFT, 288, [(syntax("IN A,(n)", "IN n"), "A ← порт n", "10"),
                     (syntax("OUT (n),A", "OUT n"), "порт n ← A", "10")], 18.5)
    panel(SECOND, 100, 216, "Пересылки · 16 бит и стек", "word")
    rows(SECOND, 142, [
        (syntax("LD rr,nn",   "LXI rr,nn"), "rr ← nn",                 "10"),
        (syntax("LD (nn),HL", "SHLD nn"),   "[nn] ← L, [nn+1] ← H",    "16"),
        (syntax("LD HL,(nn)", "LHLD nn"),   "L ← [nn], H ← [nn+1]",    "16"),
        (syntax("LD SP,HL",   "SPHL"),      "SP ← HL",                 "5"),
        ("PUSH qq",                        "SP ← SP−2, [SP] ← qq",    "11"),
        ("POP qq",                         "qq ← [SP], SP ← SP+2",    "10"),
        (syntax("EX DE,HL",   "XCHG"),      "(DE, HL) ← (HL, DE)",     "4"),
        (syntax("EX (SP),HL", "XTHL"),      "([SP], HL) ← (HL, [SP])", "18"),
    ], 20.45, pos=148)
    label(SECOND+12, 305, "В слове [SP] младший байт хранится по адресу SP.", 13.6, limit=CW-24)
    panel(LEFT, 325, 252, "Арифметика и логика · 8 бит", "alu", False)
    for xc, val in [(318, "r"), (370, syntax("(HL)", "M")), (429, "n")]:
        label(LEFT+xc, 340, val, 14, "code", align="m")
    compare_op = syntax("CP n", "CMP r / CPI n")
    operations = [
        (syntax("ADD A,n", "ADD r / ADI n"), "A ← A + n"),
        (syntax("ADC A,n", "ADC r / ACI n"), "A ← A + n + CY"),
        (syntax("SUB n",   "SUB r / SUI n"), "A ← A − n"),
        (syntax("SBC A,n", "SBB r / SBI n"), "A ← A − n − CY"),
        (syntax("AND n",   "ANA r / ANI n"), "A ← A AND n"),
        (syntax("XOR n",   "XRA r / XRI n"), "A ← A XOR n"),
        (syntax("OR n",    "ORA r / ORI n"), "A ← A OR n"),
        (compare_op, syntax("Флаги по A − n, A сохранён", "F по A − n, A сохранён")),
    ]
    action_x = syntax(119, 139)
    for i, (op, action) in enumerate(operations):
        cy = 367 + 22*i
        if op == compare_op:
            rect(LEFT+0.8, cy-10.5, CW-1.6, 21, colors["compare"][0])
            rect(LEFT+0.8, cy-10.5, 3.2, 21, colors["compare"][1])
        label(LEFT+12, cy, op, 15.6, "code", limit=action_x-14)
        label(LEFT+action_x, cy, action, 14.0, limit=303-action_x)
        for xc, ticks in [(318, "4"), (370, "7"), (429, "7")]:
            label(LEFT+xc, cy, ticks, 15.5, "code", align="m")
    label(LEFT+12, 545, syntax("Столбцы тактов соответствуют операндам r, (HL) и константе n.",
                              "Вместо r допустим M. Формулы даны для константы n."), 14, limit=CW-24)
    label(LEFT+12, 565, "Обновляются S, Z, AC, P и CY.", 14, limit=CW-24)
    panel(SECOND, 325, 177, "Инкремент, декремент и флаги", "alu")
    rows(SECOND, 365, [
        (syntax("INC r / INC (HL)", "INR r / INR M"),   "+1, меняет S, Z, AC, P",      "5/10"),
        (syntax("DEC r / DEC (HL)", "DCR r / DCR M"),   "−1, меняет S, Z, AC, P",      "5/10"),
        (syntax("INC rr / DEC rr",  "INX rr / DCX rr"), "+1 / −1, флаги сохранены",    "5"),
        (syntax("SCF", "STC"),                         "CY ← 1",                      "4"),
        (syntax("CCF", "CMC"),                         "CY ← 1 − CY",                 "4"),
        (syntax("CPL", "CMA"),                         "Инверсия A, флаги сохранены", "4"),
        ("DAA",                                        "BCD-коррекция A, все флаги",  "4"),
    ], 20.9, pos=168)
    panel(SECOND, 509, 68, "Сложение · 16 бит", "widealu")
    rows(SECOND, 555, [(syntax("ADD HL,rr", "DAD rr"), "HL ← HL + rr, меняется только CY", "10")], 20, pos=145)
    panel(LEFT, 586, 115, "Вращение аккумулятора · только CY", "rotate")
    rows(LEFT, 626, [
        (syntax("RLCA", "RLC"), "Влево по кругу, бит 7 → CY",          "4"),
        (syntax("RRCA", "RRC"), "Вправо по кругу, бит 0 → CY",         "4"),
        (syntax("RLA",  "RAL"), "Влево через CY, бит 7 → CY → бит 0",  "4"),
        (syntax("RRA",  "RAR"), "Вправо через CY, бит 0 → CY → бит 7", "4"),
    ], 20.8, pos=85)
    panel(LEFT, 709, 113, "Управление процессором", "system")
    rows(LEFT, 748, [
        ("NOP",  "Холостой ход",                              "4"),
        (syntax("HALT", "HLT"), "Останов, указаны такты входа", "7"),
        ("DI",   "Запретить прерывания",                      "4"),
        ("EI",   "Разрешить прерывания после следующей команды", "4"),
    ], 20.5, pos=71, descsize=13.3)
    panel(SECOND, 586, 236, "Переходы, вызовы и возвраты", "branch")
    rows(SECOND, 627, [
        (syntax("JP nn / JP cc,nn", "JMP nn / Jcc nn"), "Переход / переход при cc", "10"),
        (syntax("JP (HL)", "PCHL"),    "PC ← HL",                    "5"),
        ("CALL nn",                   "Вызов по адресу nn",         "17"),
        (syntax("CALL cc,nn", "Ccc nn"), "Вызов при условии cc",    "11/17"),
        ("RET",                       "Возврат по адресу из стека", "10"),
        (syntax("RET cc", "Rcc"),     "Возврат при условии cc",     "5/11"),
        (syntax("RST v", "RST k"), syntax("Вызов по адресу вектора v", "Вызов по адресу 8 × k"), "11"),
    ], 21.9, pos=172)
    label(SECOND+12, 788, syntax("Для CALL cc и RET cc такты даны в порядке «нет / да».",
                                "Суффикс входит в мнемонику (JNZ, CZ, RPE). Такты — «нет / да»."), 13.6, limit=CW-24)
    label(SECOND+12, 808, syntax("v = 00h, 08h, 10h, 18h, 20h, 28h, 30h, 38h.",
                                "k = 0…7.  RST 0 → 00h, RST 1 → 08h, …, RST 7 → 38h."), 14, "regular", limit=CW-24)
    panel(LEFT, 831, 149, "Операнды", "alu", False)
    notes(LEFT, 871, [
        ("r, r′", "B, C, D, E, H, L или A"),
        ("rr", syntax("BC, DE, HL или SP", "B, D, H, SP — пары BC, DE, HL и указатель SP")),
        ("qq", syntax("BC, DE, HL или AF (A и флаги F)", "B, D, H, PSW — пары BC, DE, HL и слово PSW (A и F)")),
        ("n / nn", "8 / 16 бит. В коде nn первым идёт младший байт."),
        (syntax("(HL)", "M"), "Байт по адресу HL. В пояснениях [ ] означают память."),
    ], step=23, pos=75)
    panel(SECOND, 831, 149, "Регистр флагов F и условия cc", "alu", False)
    for i, flag in enumerate(("S", "Z", "0", "AC", "0", "P", "1", "CY")):
        x = SECOND + 12 + 40*i
        rect(x, 865, 36, 23, flag_fixed if flag in ("0", "1") else flag_active)
        label(x+18, 876.5, flag, 14, "code", align="m")
    label(SECOND+CW-12, 878, "биты 7 → 0", 13, align="r")
    label(SECOND+12, 904, "NZ / Z", 14.8, "code")
    label(SECOND+94, 904, "Z = 0 / 1")
    label(SECOND+233, 904, "NC / C", 14.8, "code")
    label(SECOND+326, 904, "CY = 0 / 1")
    label(SECOND+12, 925, "PO / PE", 14.8, "code")
    label(SECOND+94, 925, "P = 0 / 1")
    label(SECOND+233, 925, "P / M", 14.8, "code")
    label(SECOND+326, 925, "S = 0 / 1")
    label(SECOND+12, 948, "S — знак, Z — нуль, AC — перенос из бита 3, CY — перенос.", 13.7, limit=CW-24)
    label(SECOND+12, 967, syntax("P = 1 при чётном числе единиц. POP AF загружает F из стека.",
                                "P = 1 при чётном числе единиц. POP PSW загружает F из стека."), 13.7, limit=CW-24)


def opcode_map():
    x0, x1, y0, y1 = 980, 1994, 100, 926
    side, cap = 31, 31
    w = (x1-x0-2*side)/16
    h = (y1-y0-2*cap)/16
    rect(x0, y0, x1-x0, y1-y0, axis_fill)
    for i in range(16):
        xc, yc = x0+side+(i+0.5)*w, y0+cap+(i+0.5)*h
        for y in (y0+cap/2, y1-cap/2):
            label(xc, y, f"{i:X}", 20, "code", axis_ink, "m")
        for x in (x0+side/2, x1-side/2):
            label(x, yc, f"{i:X}", 20, "code", axis_ink, "m")
    for code, (asm, group) in enumerate(TABLE):
        row, col = divmod(code, 16)
        x, y = x0+side+col*w, y0+cap+row*h
        rect(x, y, w, h, colors[group][0])
        instr, _, arg = asm.partition(" ")
        cx, cy = x+w/2, y+h/2
        label(cx, cy-8 if arg else cy, instr, 17.2, "code", align="m", limit=w-5)
        if arg:
            label(cx, cy+10, arg, 16.0, "regular", align="m", limit=w-5, fit=True)
        if group == "duplicate":
            label(x+w-4, y+8, "*", 11.5, "code", muted, "r")
    for j in range(17):
        thickness = 1.8 if j % 4 == 0 else 0.67
        color = grid_major if j % 4 == 0 else grid_minor
        rect(x0+side+j*w-thickness/2, y0+cap, thickness, 16*h, color)
        rect(x0+side, y0+cap+j*h-thickness/2, 16*w, thickness, color)
    rect(x0, 942, 25, 25, colors["duplicate"][0])
    label(x0+12.5, 956, "*", 16, "code", align="m")
    label(x0+36, 953, "Серые ячейки — недокументированные дубли ВМ80А.", 15.4, "semibold")
    label(x0+36, 973, "Мнемоника в серой ячейке описывает действие байта. Ассемблер выбирает основной код.", 14, color=muted)
    rect(1676, 942, 1, 37, rule)
    label(1692, 953, syntax("LD HL,1234h", "LXI H,1234h"), 15.4, "code")
    label(1692, 973, "Машинный код", 14, color=muted)
    label(1808, 973, "21 34 12", 15, "code")


def render():
    label(LEFT, 43, syntax("Мнемоники Z80", "Мнемоники i8080"), 42, "bold")
    divider = syntax(382, 414)
    rect(divider, 27, 1.1, 34, rule)
    label(divider+20, 44, "КР580ВМ80А", 29, "semibold")
    label(LEFT, 79, syntax("Синтаксис SjASMPlus --i8080", "Синтаксис Intel 8080") +
          "     ·     Время в тактах ВМ80А     ·     При 2 МГц один такт равен 0,5 мкс", 15, color=muted)
    label(980, 45, "Карта кодов операций", 32, "bold")
    label(980, 79, "Старшая шестнадцатеричная цифра выбирает строку, младшая — столбец.", 15, color=muted)
    label(1994, 48, syntax("7Eh  →  LD A,(HL)", "7Eh  →  MOV A,M"), 20, "code", align="r")
    left()
    opcode_map()
    if png:
        img.save(OUT, "PNG", dpi=(600, 600))
    else:
        img.save(OUT, "JPEG", quality=96, subsampling=0, optimize=True, dpi=(600, 600))
    with Image.open(OUT) as saved:
        saved.load()
        assert saved.format == ("PNG" if png else "JPEG") and saved.size == (W*SCALE, H*SCALE)
        print(OUT, saved.size, saved.info["dpi"])


if __name__ == "__main__":
    check_assembly()
    render()
