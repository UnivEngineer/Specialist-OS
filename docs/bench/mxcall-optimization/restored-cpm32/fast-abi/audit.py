from pathlib import Path
import re
paths=('bios','dos','ui-gfx','io','flash','floppy','ramdisk')
ids=(0x0102,0x0109,0x010A,0x0114,0x0106,0x010E,0x0110,0x010F)
fixed=(0xF81B,0xF81E,0xF821,0xF824,0xF827,0xF82A,0xF830,0xF833)
numbers=re.compile(r'(?<![\w.])(?:([0-9a-f]+)h|0x([0-9a-f]+)|\$([0-9a-f]+)|(\d+))(?![\w.])',re.I)
old=re.compile(r'\bMXOS_BIOS_(?:PRINT_CHAR|GET_CURSOR_POS|SET_CURSOR_POS|PRINT_CHAR_RAW|PRINT_HEX_BYTE|PRINT_HEX_WORD|GET_MEM_TOP|SET_MEM_TOP)\b')
abi=re.compile(r'^\s*(?:(?:[\w.]+):\s*)?(?:mxos_call|(?:call|jp)\s+(?:mx_call|mx_callRegs)|(?:ld|dw))\b',re.I)
for name in paths:
    files=[p for p in Path('source',name).rglob('*') if p.suffix.lower() in ('.asm','.inc')]
    for p in files:
        for n,line in enumerate(p.read_text(encoding='utf-8-sig').splitlines(),1):
            code=line.split(';',1)[0]
            if old.search(code):
                print('old name', p,n,code)
            for m in numbers.finditer(code):
                _,value=next((i,g) for i,g in enumerate(m.groups()) if g)
                number=int(value,10 if m.group(4) else 16)
                if number in fixed or number in ids and abi.match(code):
                    print('literal',p,n,code)
    print(name, len(files))
