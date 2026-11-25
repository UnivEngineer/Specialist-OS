"""Сборка MEM.SYS из source/mem; промежуточные фрагменты — bin/banked."""
from pathlib import Path
import subprocess
import re

ROOT = Path(__file__).absolute().parents[2]
OUT = ROOT / 'bin/banked'

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    target_file=ROOT/'bin/BIOS.sym'
    previous=dict(re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',target_file.read_text(),re.M)) if target_file.exists() else {}
    (OUT/'main-io-import.inc').write_text(
        f'MAIN_BLOCK_IO_TARGET = 0{int(previous.get("mainBlockIO","0"),16):04X}h\n'
        f'MAIN_FIXED_TARGET = 0{int(previous.get("biosMemFixedMain","0"),16):04X}h\n'
        f'MAIN_PRINT_TARGET = 0{int(previous.get("biosMemPrintMain","0"),16):04X}h\n'
        f'MAIN_MEM_TOP_TARGET  = 0{int(previous.get("v_memTop","0"),16):04X}h\n')
    for name in ('WINDOW', 'TRANSPORT'):
        subprocess.run([str(ROOT/'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                        f'--lst={OUT/name}.lst', f'--sym={OUT/name}.sym',
                        f'--raw={OUT/name}.bin', (ROOT/f'source/mem/{name}.asm').as_posix()], check=True)
    sy = {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
                                          (OUT/'TRANSPORT.sym').read_text(),re.M)}
    transport = OUT/'TRANSPORT.bin'
    payload = transport.read_bytes()
    length = sy['bankDataTailEnd'] - sy['BANK_LANDING']
    assert len(payload) - length == sy['movedEnd'] - sy['bankFastPrepare']
    (OUT/'fast.bin').write_bytes(payload[length:])
    transport.write_bytes(payload[:length])
    (OUT/'transport-imports.inc').write_text('; Генерируемые адреса внутреннего транспорта.\n'+''.join(
        f'{name} = 0{sy[name]:04X}h\n' for name in ('bankCopyMoveRemote','moveForwardDispatch','moveDataForward','moveDataForwardEnd','bankMoveToRam','bankMoveToMain','bankMxRegsDos','bankFastPrepare','bankFastBiosSP','bankFastDos','bankFastReturn','bankLocalChecked','bankLocalBlockBad','bankQueueBefore','bankQueueBind','bankPublishSP','bankQueueResetImage','bankCall','bankCallPacket','bankCallback','bankModuleCallback','bankCopyFar','bankCopyEnter','bankCopySector','bankCopyPhysical','bankCopyBytes','bankCopyQuads','bankCopyPrepared','bankCopyRequest','bankCopyOK','bankCopyError','bankCopyPair','bankCopyPageCheck','bankCopyRange','bankCopyReturn','bankCopyClone','bankCopySP','bankCopyRest','bankCopyTail','bankCopyLocalParams','bankCopySourcePair','bankCopyDestPair','bankCopyReturnPair','bankCopyCount','bankPageCount','bankPage','bankOwner','bankHome','bankDepth','bankServiceSP','bankStackFloor')),encoding='utf-8')
    with (OUT/'transport-imports.inc').open('a',encoding='utf-8') as stream:
        for name in ('bankTextCopy32','bankTextChecked','bankTextCopyPartial'):
            stream.write(f'{name} = 0{sy[name]:04X}h\n')
        stream.write(f'bankNearForward = 0{sy["compact223Forward"]:04X}h\n')
        stream.write(f'bankLocalForward = 0{sy["MEM_LOCAL_FORWARD"]:04X}h\n')
        stream.write(f'bankMoveRevision = {sy["MEM_RUNTIME_REVISION"]}\n')
        stream.write(f'bankMoveFullEntry = 0{sy["moveCore"]:04X}h\n')
        stream.write(f'bankMoveDataEntry = 0{sy["moveDataForward"]:04X}h\n')
        stream.write(f'bankMoveDataBytes = {sy["moveDataForwardEnd"]-sy["moveDataForward"]}\n')
        stream.write(f'bankMoveDirectOperand = 0{sy["moveForwardDispatch"]+1:04X}h\n')
        stream.write(f'bankUnifiedCore = 0{sy["moveCore"]:04X}h\n')
        stream.write(f'BANK_COPY_DATA_BYTES = {sy["bankDataTailEnd"]-sy["MEM_DATA_START"]}\n')
        stream.write(f'BANK_TRANSPORT_BYTES = {(OUT/"TRANSPORT.bin").stat().st_size}\n')
        stream.write(f'BANK_COPY_GUARD_ADDR = 0{sy["bankKeyboardSignature"]:04X}h\n')
    for name in ('DISKIO','FLASHDRV','FASTDRV','ROMDRV','MAINBIOS','LOCALBLOCK'):
        folder='source/io' if name in ('MAINIO','MAINBIOS') else ('source/bootstrap/tests' if name in ('DISKIO','FLASHDRV','FASTDRV','ROMDRV') else 'source/mem')
        subprocess.run([str(ROOT/'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                        f'--lst={OUT/name}.lst', f'--sym={OUT/name}.sym',
                        f'--raw={OUT/name}.bin', (ROOT/f'{folder}/{name}.asm').as_posix()], check=True)
    # Одна сборка разрешает ссылки LOCAL <-> TEXT без фиксированных перемычек.
    # Установочный формат по-прежнему содержит отдельные LOCAL и CLIENT.
    cs = dict(re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', (OUT/'LOCALBLOCK.sym').read_text(), re.M))
    core = (OUT/'LOCALBLOCK.bin').read_bytes()
    split = int(cs['mx_callPacket'],16)-int(cs['MEM_IMAGE_LOCAL_START'],16)
    (OUT/'CLIENT.bin').write_bytes(core[split:])
    (OUT/'LOCALBLOCK.bin').write_bytes(core[:split])
    (OUT/'CLIENT.sym').write_text((OUT/'LOCALBLOCK.sym').read_text(),encoding='utf-8')
    (OUT/'CLIENT.lst').write_bytes((OUT/'LOCALBLOCK.lst').read_bytes())
    with (OUT/'transport-imports.inc').open('a',encoding='utf-8') as stream:
        for name in ('memDosReadChunk', 'memDosWriteChunk'):
            stream.write(f'{name} = 0{int(cs[name],16):04X}h\n')
        stream.write(f"bankMemExecCopy = 0{int(cs['memExecCopy'],16):04X}h\n")
        stream.write(f"bankMxRegsRouteCall = 0{int(cs['bankMxRegsRouteCall'],16):04X}h\n")
        stream.write(f'BANK_CLIENT_BYTES = {(OUT/"CLIENT.bin").stat().st_size}\n')
        stream.write(f'BANK_MAIN_CLIENT_BYTES = {int(cs["bankClientEnd"],16)-int(cs["mx_callPacket"],16)}\n')
        stream.write(f'BANK_LOCAL_BYTES = {(OUT/"LOCALBLOCK.bin").stat().st_size}\n')
        stream.write(f'BANK_MAIN_LOCAL_BYTES = {int(cs["MEM_IO_SYSTEM"],16)-int(cs["MEM_IMAGE_LOCAL_START"],16)}\n')
    for kind in range(3):
        name = f'LOCALDRV{kind}'
        subprocess.run([str(ROOT/'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                        f'-DLOCAL_KIND={kind}', f'--lst={OUT/name}.lst',
                        f'--sym={OUT/name}.sym', f'--raw={OUT/name}.bin',
                        (ROOT/'source/mem/LOCALDRV.asm').as_posix()], check=True)

    def symbols(path):
        return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', path.read_text(),re.M)}
    with (OUT/'flash-imports.inc').open('w',encoding='utf-8') as stream:
        ls=symbols(OUT/'LOCALDRV0.sym')
        for key in ('RomReadSelect','FlashRead'):
            stream.write(f'local0{key} = {ls[key]-ls["Driver"]}\n')
        for kind in (1,2):
            ls=symbols(OUT/f'LOCALDRV{kind}.sym')
            for key in ('SectorLimit','v_chipMap'):
                stream.write(f'local{kind}{key} = {ls[key]-ls["Driver"]}\n')
        cs=symbols(OUT/'LOCALBLOCK.sym')
        stream.write(f'LOCAL_DRIVER_KIND = 0{cs["localDriverKind"]:04X}h\n')

    subprocess.run([str(ROOT/'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                    f'--lst={ROOT/"bin/MEM.lst"}', f'--sym={ROOT/"bin/MEM.sym"}',
                    f'--raw={ROOT/"bin/MEM.sys"}', (ROOT/'source/mem/MEM.asm').as_posix()], check=True)

if __name__ == '__main__':
    main()
