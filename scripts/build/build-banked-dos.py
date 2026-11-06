"""Сборка файлового ядра для физической страницы ДОЗУ 0."""
from pathlib import Path
import json
import runpy

ROOT = Path(__file__).resolve().parents[2]


def main():
    runpy.run_path(str(ROOT/'scripts/build/build-banked-transport.py'))['main']()
    h = runpy.run_path(str(ROOT/'scripts/build/pack-dos.py'))
    _, groups = runpy.run_path(str(ROOT/'scripts/build/build-bios-dos.py'))['component_groups']()
    out = ROOT/'bin/banked'
    fs = h['symbols'](out/'FLASHDRV.sym')
    (out/'flash-imports.inc').write_text(''.join(
        f'{name} = 0{fs[name]:04X}h\n' for name in ('Driver','flashDosInit','FuncSize')), encoding='utf-8')
    with (out/'flash-imports.inc').open('a',encoding='utf-8') as stream:
        for kind,stem in [(1,'FLASHDRV'),(2,'FASTDRV')]:
            ss=h['symbols'](out/f'{stem}.sym')
            ls=h['symbols'](out/f'LOCALDRV{kind}.sym')
            for key in ('SectorLimit','v_chipMap'):
                stream.write(f'local{kind}{key} = {ls[key]-0xE800}\n')
                stream.write(f'source{kind}{key} = 0{ss[key]:04X}h\n')
        cs=h['symbols'](out/'CLIENT.sym')
        stream.write(f'LOCAL_DRIVER_KIND = 0{cs["localDriverKind"]:04X}h\n')
    rs = h['symbols'](out/'ROMDRV.sym')
    with (out/'flash-imports.inc').open('a',encoding='utf-8') as stream:
        stream.write(f'romDriver = 0{rs["Driver"]:04X}h\nromDiskInfoPointer = 0{rs["v_diskInfo"]:04X}h\n')
    fs = h['symbols'](out/'FASTDRV.sym')
    with (out/'flash-imports.inc').open('a',encoding='utf-8') as stream:
        for alias,key in [('fastDriverNative','Driver'),('fastDosInit','flashDosInit'),('fastFuncSize','FuncSize'),('fastChipStates','v_chipStates')]:
            stream.write(f'{alias} = 0{fs[key]:04X}h\n')
    lines = []
    for unit in groups['dos']:
        for filename in unit['files']:
            if filename == 'reboot3.inc':
                lines.append(h['include'](ROOT/'source/banked/kernel-start.inc'))
            else:
                lines.append(h['include'](ROOT/'source/dos'/filename))
    for name in ('cmp_hl_de','math','strcmp','move8080','memcpy_bc_hl','memmove_bc_hl',
                 'memset_de_20_b','argNext','calcCS','biosState','installDriver'):
        lines.append(h['include'](ROOT/f'source/dos/{name}.inc'))
    (out/'kernel-layout.inc').write_text('\n'.join(lines)+'\n', encoding='utf-8')
    registry = json.loads((ROOT/'source/bios/syscalls.json').read_text(encoding='utf-8'))
    services = next(group['services'] for group in registry if group['group'] == 2)
    assert [service['id'] for service in services] == list(range(len(services)))
    (out/'kernel-services.inc').write_text(''.join(
        f'    DW {service["target"].removeprefix("dos_")}\n' for service in services), encoding='utf-8')
    h['assemble'](ROOT/'source/banked/KERNEL.asm', out/'KERNEL.bin', [])
    sy = h['symbols'](out/'KERNEL.sym')
    report = {key:sy[key] for key in ('dosCoreStart','dosCoreEnd','kernelEnd','BANK_KERNEL_CACHE',
                                     'bankKernelCommand','kernelDispatch','v_drives','v_diskInfo')}
    report['bytes'] = (out/'KERNEL.bin').stat().st_size
    (out/'KERNEL.layout.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(f'Bank DOS: {report["bytes"]} bytes, end {sy["kernelEnd"]:04X}h')


if __name__ == '__main__':
    main()
