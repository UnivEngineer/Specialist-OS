"""Фиксированный ABI BIOS, плотный образ DOS и непересекающиеся буферы."""
from pathlib import Path
import argparse,json,runpy,struct
ROOT=Path(__file__).resolve().parents[2]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report-directory',type=Path,default=ROOT/'docs/source-layout-039/layout')
    args=parser.parse_args();out=args.report_directory.resolve();out.mkdir(parents=True,exist_ok=True)
    h=runpy.run_path(str(ROOT/'scripts/build/pack-dos.py'))
    bios=h['symbols'](ROOT/'bin/BIOS.sym');dos=h['symbols'](ROOT/'bin/banked/KERNEL.sym')
    layout=json.loads((ROOT/'bin/System.layout.json').read_text())
    image=(ROOT/'bin/DOS.sys').read_bytes();binary=(ROOT/'bin/BIOS.sys').read_bytes()
    assert layout['dos']['start']==dos['DOS_BANK_CODE_ADDR']==0x100
    assert layout['dos']['end']==dos['kernelEnd']
    assert len(image)==layout['dos']['bytes']==dos['kernelEnd']-dos['dosCoreStart']
    assert image==(ROOT/'bin/banked/KERNEL.bin').read_bytes()
    assert dos['dosCoreEnd']==dos['dosExtensionStart']
    work = dos['dosWorkStart']
    end = dos['dosWorkEnd']
    assert (work, end) == (layout['dos']['variablesStart'], layout['dos']['variablesEnd'])
    assert dos['kernelEnd'] <= work == dos['DOS_VARIABLES_ADDR']
    assert work < dos['dosDefaultsEnd'] < dos['dosZeroEnd'] == dos['memoryRamFresh'] == end-1
    assert end <= dos['DOS_BANK_DATA_ADDR'] == dos['cliIoBuffer'] == dos['cliAssociationBuffer']
    assert dos['dosDefaultsEnd']-work == dos['kernelEnd']-dos['dosColdDefaults']
    assert all(work <= dos[name] < end for name in ('v_diskInfo','v_cliPath','v_tmpFileDescr','v_cacheCount','v_dosModuleReady'))
    assert image[:3]==b'\xC3'+dos['dosBoot'].to_bytes(2,'little')
    assert len(binary)==layout['bios']['bytes']
    io=(ROOT/'bin/IO.sys').read_bytes()
    assert len(io)==layout['io']['bytes']+layout['io']['bankBytes']
    assert io[layout['io']['bytes']:]==(ROOT/'bin/banked/KEYBANK.bin').read_bytes()
    assert io[:layout['io']['bytes']]==(ROOT/'bin/IO.main.bin').read_bytes()
    assert layout['io']['bankAddress']==dos['IO_BANK_KEYS']
    assert layout['io']['bankAddress']+layout['io']['bankBytes']==dos['MEM_IMAGE_LOCAL_START']
    assert layout['io']['end']==bios['MEM_IMAGE_LOCAL_START']
    assert io[layout['io']['bytes']-15:layout['io']['bytes']]==io[-15:]
    assert bios['io_pitRead']==dos['io_pitRead']==layout['io']['end']-15
    assert bios['io_pitGetPort']==dos['io_pitGetPort']==layout['io']['end']-5
    assert layout['io']['start'] <= bios['beep'] < bios['ioKeyReadCtrl'] < bios['ioKeyScan2']
    assert bios['ioKeyScan2'] < bios['beepTimer'] < bios['ioKeyClockInit'] < bios['io_pitRead']
    assert layout['bios']['start'] <= bios['ioStd.ioKeyScan2'] < layout['io']['start']
    clock_gate = dos['BIOS_BANK_KEY_CLOCK_INIT']
    relative = clock_gate-layout['io']['start']
    assert io[relative:relative+3] == b'\xC3'+bios['ioKeyClockInit'].to_bytes(2,'little')
    assert 'ioKeyClockInit' not in dos and 'IO_TIMER_MODE' not in dos
    timer_service = image[dos['timerService']-layout['dos']['start']:dos['timerRelease']-layout['dos']['start']]
    assert b'\xaf\x21'+clock_gate.to_bytes(2,'little')+b'\xcd'+dos['bankCall'].to_bytes(2,'little') in timer_service
    for mode in (82,91):
        legacy = bios[f'mon2_setKeybMode{mode}']-layout['bios']['start']
        io_mode = bios[f'ioKeyMode{mode}']
        assert bios['ioKeyReadCtrl'] < io_mode < bios['ioKeyScan2']
        assert binary[legacy:legacy+6] == bytes([0x3e,int(str(mode),16),0x32])+bios['IO_KEYB_MODE'].to_bytes(2,'little')+b'\xc9'
    for abi, name in [('BYTE', 'printHexByte'), ('WORD', 'printHexWord')]:
        relative=bios[f'BIOS_BANK_PRINT_HEX_{abi}']-layout['io']['start']
        assert io[relative:relative+3] == b'\xc3'+bios[name].to_bytes(2,'little')
    mem=(ROOT/'bin/MEM.sys').read_bytes()
    address,size,offset=struct.unpack_from('<HHH',mem,6)
    at=offset+bios['MEM_MAIN_IO_BRIDGE']-address
    assert mem[at:at+3]==b'\xc3'+bios['mainBlockIO'].to_bytes(2,'little')
    assert bios['LOCAL_DRIVER_END']-bios['LOCAL_DRIVER_ADDR']>=412
    assert bios['SYSTEM_FLASH_END']<=bios['LOCAL_DRIVER_ADDR']
    assert bios['biosLowEnd']<=bios['FONT_ADDR']
    assert bios['io_vars']+bios['IO_VARIABLES']<=bios['STD_IO_ADDR']
    assert bios['FONT_ADDR']+len((ROOT/'bin/fonts/Compact.pf').read_bytes())<bios['FONT_ARENA_END']
    assert bios['bios_upper']+bios['BIOS_UPPER_VARIABLES'] <= bios['io_vars']
    assert bios['MON2_VARIABLES'] == 33
    for field,address in [('keyboardHook',0x8FDF),('tapeError',0x8FE1),('monitorFont',0x8FE7),('currentChar',0x8FE9),('cursorVisible',0x8FEA),('inputBusy',0x8FEB),('keyLocks',0x8FEC),('oldSP',0x8FF6),('inverse',0x8FFA),('cursorY',0x8FFC),('readDelay',0x8FFF)]:
        assert bios['mon2_vars.'+field] == address,field
    intervals=sorted((v['address'],v['address']+v['size']) for v in layout['blocks']+layout['windows'] if v['size'])
    assert all(a[1]<=b[0] for a,b in zip(intervals,intervals[1:]))
    for item in layout['windows']:
        if 'target' in item:
            offset=item['address']-layout['bios']['start']
            assert binary[offset:offset+3]==b'\xC3'+bios[item['target']].to_bytes(2,'little')
    spans=[(0,0x100,'vectors'),(dos['dosCoreStart'],dos['kernelEnd'],'image'),(work,end,'DOS variables'),(dos['MEM_IMAGE_LOCAL_START'],dos['MEM_BANK_CLIENT_START'],'local-block')]
    for address,size,name in [('DOS_BANK_DATA_ADDR',dos['DOS_BANK_DATA_SIZE'],'exchange'),('DOS_BANK_OBJECT_ADDR',128,'object'),('MEM_ALLOC_TABLE',192,'allocations'),('MODULE_TABLE',128,'modules'),('DOS_IMAGE_CONTEXT_ADDR',83,'image-context'),('DOS_IMAGE_HEADER_ADDR',dos['IMAGE_RELOC_HEADER'],'image-header'),('DOS_FONT_STATE_ADDR',169,'font-state'),('DOS_MODULE_GUARD_ADDR',9,'module-release-guard'),('DOS_PARTIAL_RECORD',64,'partial-record'),('DOS_CLI_PATH1',128,'path1'),('DOS_CLI_PATH2',128,'path2'),('DOS_REQUEST_SCRATCH',42,'request-scratch'),
                              ('DOS_BANK_PATH_ADDR',128,'path'),('DOS_DRIVER_REGISTRY',640,'registry'),
                              ('DOS_DRIVER_IMAGE',576,'driver-image'),('DOS_BANK_CACHE_ADDR',8192,'cache'),
                              ('DOS_BANK_CACHE_TABLE',512,'cache-table'),
                              ('DOS_BANK_PARTIAL_ADDR',512,'partial'),
                              ('DOS_VERIFY_BUFFER',256,'verify'),('DOS_IO_BATCH_ADDR',152,'LBA-batch'),
                              ('DOS_CACHE_BOOT_ADDR',512,'BPB'),
                              ('DOS_BANK_CMD_ADDR',130,'command'),('DOS_BANK_COMPAT_ADDR',33,'variables'),
                              ('DOS_RESIDENT_WORK',dos['dosResidentWorkEnd']-dos['DOS_RESIDENT_WORK'],'resident and memory variables'),
                              ('DOS_ARENA_START',dos['DOS_BANK_MODULE_END']-dos['DOS_ARENA_START'],'DLL and installer arena'),('BAT_BUFFER',dos['BAT_BUFFER_SIZE'],'BAT'),
                              ('IO_BANK_KEYS',layout['io']['bankBytes'],'local keyboard'),('DOS_BANK_STACK_BOTTOM',dos['DOS_BANK_STACK_TOP']-dos['DOS_BANK_STACK_BOTTOM'],'stack'),('MEM_BANK_CLIENT_START',0x800,'tail')]:
        spans.append((dos[address],dos[address]+size,name))
    assert dos['DOS_BANK_DRIVER_END'] == dos['DOS_BANK_MODULE_END'] == dos['BAT_BUFFER']
    assert dos['BAT_INPUT'] == dos['BAT_HANDLE'] + dos['FILE_HANDLE']
    assert dos['BAT_INPUT'] + dos['BAT_READ_SIZE'] == dos['DOS_CLI_PATH2']
    assert dos['DOS_BANK_DATA_ADDR'] <= dos['BAT_COMMAND'] <= dos['DOS_BANK_DATA_ADDR'] + dos['DOS_BANK_DATA_SIZE'] - dos['MAX_PATH']
    spans.sort();assert all(a[1]<=b[0] for a,b in zip(spans,spans[1:])),spans
    _,groups=runpy.run_path(str(ROOT/'scripts/build/build-bios-dos.py'))['component_groups']()
    for region,units in groups.items():
        for unit in units:
            for filename in unit['files']:
                path=Path(filename).resolve();assert path.is_file(),path
                assert path.is_relative_to(ROOT/f'source/{region}') or path.is_relative_to(ROOT/'source/common') or (region=='dos' and path.is_relative_to(ROOT/'source/cli')) or (region=='bios' and path.is_relative_to(ROOT/'source/io')),path
    result=dict(biosBytes=len(binary),dosBytes=len(image),dosStart=dos['dosCoreStart'],dosEnd=dos['kernelEnd'],
                freeBeforeVariables=work-dos['kernelEnd'],freeBeforeBuffers=dos['DOS_BANK_DATA_ADDR']-end,regions=spans,fixedSlots=len(layout['windows']))
    (out/'result.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print('BIOS/DOS LAYOUT PASS',len(binary),len(image),result['fixedSlots'],'fixed slots')

if __name__=='__main__':main()
