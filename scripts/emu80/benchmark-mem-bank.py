"""Сравнить MEM на физическом маппере MX2 в собственном скрытом Emu80 32×."""
from pathlib import Path
import argparse
import configparser
import hashlib
import json
import os
import re
import runpy
import shutil
import struct
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
WORD = struct.Struct('<H').pack
SYSTEM = 'C-Flash-512k-MMX-System-Emu80.bin'
STUB, PACKET, SP = 0x8100, 0x8200, 0x8650


def symbols(path):
    result = {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',path.read_text(encoding='utf-8'),re.M)}
    if 'mx_call' in result:  # Имя F800h в архивных --bundle.
        result.setdefault('mx_callPacket', result['mx_call'])
    return result


def shell(script):
    result=subprocess.run(['powershell.exe','-NoProfile','-Command',script],capture_output=True,text=True,check=True,
                          creationflags=subprocess.CREATE_NO_WINDOW)
    return result.stdout.strip()


def ps(path):
    return "'"+str(path).replace("'","''")+"'"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def hidden_qt(runtime):
    config=configparser.ConfigParser()
    config.read(runtime/'qt.conf',encoding='utf-8-sig')
    plugins=(runtime/config.get('Paths','Plugins',fallback='.')).resolve()
    platforms=plugins/'platforms'
    if not (platforms/'qoffscreen.dll').is_file():
        raise RuntimeError(f'Отсутствует Qt offscreen в {platforms}. Emu80 не запущен.')
    os.environ['QT_QPA_PLATFORM']='offscreen'
    os.environ['QT_QPA_PLATFORM_PLUGIN_PATH']=str(platforms)
    os.environ['QT_PLUGIN_PATH']=str(plugins)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bundle',type=Path,default=ROOT,help='Каталог с bin и images')
    parser.add_argument('--runtime',type=Path,required=True)
    parser.add_argument('--port',type=int,choices=range(19781,19797),default=19781)
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--suite',choices=('copy','bios','legacy','user','remote-move','local-copy','text','text-windows','text-main','all'),default='copy',help='all: copy + bios')
    parser.add_argument('--text-lengths',type=int,nargs='+',default=(0,1,20,31,32,33,64,128),
                        help='Длины строк для --suite text, от 0 до 511 байт')
    args=parser.parse_args()
    if any(not 0 <= length <= 511 for length in args.text_lengths):
        parser.error('Длина текста должна быть от 0 до 511 байт')
    out=args.out.resolve()
    out.mkdir(parents=True,exist_ok=True)
    bundle,runtime=args.bundle.resolve(),args.runtime.resolve()
    image=bundle/'images'/SYSTEM
    exe=runtime/'Emu80qt-mcp.exe'
    bench=runpy.run_path(str(ROOT/'source-apps/diskbench/check-emulator.py'))['Bench']
    m=bench(f'http://127.0.0.1:{args.port}/mcp')
    b=symbols(bundle/'bin/BIOS.sym')
    t=symbols(bundle/'bin/banked/TRANSPORT.sym')
    k=symbols(bundle/'bin/banked/LOCALBLOCK.sym')
    mem=(bundle/'bin/MEM.sys').read_bytes()
    segments=[struct.unpack_from('<HHH',mem,6+6*i) for i in range(mem[5])]
    window=(bundle/'bin/banked/WINDOW.bin').read_bytes()
    markers={page:bytes([0xa5,page,0xff^page,0x5a]) for page in (0,1,2,3,4,6)}
    # Общий вход MOVE rev66+ обслуживает любую страницу без COPY.
    local_move='bankCopyMoveRemote' in t

    def owner():
        return shell(f'$l=Get-NetTCPConnection -State Listen -LocalPort {args.port} -ErrorAction SilentlyContinue; '
                     'if($l){$l[0].OwningProcess}; exit 0')

    def page(n):
        at=0xfffe if n&0x80 else 0xfffd if n else 0xfffc
        m.write(at,bytes([(n&15) if n&0x80 else n-1 if n else 0]))

    def run(caller, target, hl, *, af=0x0057, bc=0x3456, de=0x5678, inline=None):
        page(caller)
        code=b'\xcd'+WORD(target)+(WORD(inline) if inline is not None else b'')
        m.write(STUB,code)
        stop=STUB+len(code)
        m.stop_at(stop)
        m.call('regs_set',pc=STUB,sp=SP,af=af,bc=bc,de=de,hl=hl)
        start=m.call('cpu_state')['cpu_ticks']
        state=m.until(stop,10)
        regs={name:state['registers'][name]['dec'] for name in ('af','bc','de','hl','sp','pc')}
        assert regs.pop('pc')==stop and regs['sp']==SP,regs
        assert m.read(t['bankPage'],1)==bytes([caller]),(caller,regs)
        assert m.read(0x86a0,4)==markers[caller]
        return state['cpu_ticks']-start,regs

    def copy(caller, source, dest, count, offset, mode='far', error=0):
        start_src=0x2000+offset if not error else 0xff00 if error==0xe2 else 0x2000
        start_dst=0x5000+offset
        if source==dest==1 and not error:
            # Перед AUTOEXEC эти участки свободны. Оба целиком выше кода DOS и ниже его стека.
            start_src,start_dst=0xa000+offset,0xc800+offset
        if count and not error:
            page(source)
            if source&0x80:
                payload=m.read(start_src,count)
            else:
                payload=bytes((i*73+(i>>8)+count+offset)&255 for i in range(count))
                m.write(start_src,payload)
        else:
            payload=bytes(count)
        if not dest&0x80 and dest<=16:
            page(dest)
            m.write(start_dst-1,b'\xa5'+bytes([0xcc])*count+b'\x5a')
        keep_window=local_move and (error or not count or mode=='far' and source==dest)
        for seed in ('initial','repeat'):
            page(caller)
            if seed=='initial':
                m.write(0xffc0,window)
            initial_window=m.read(0xffc0,32) if keep_window else None
            before=m.read(t['bankOwner'],7)
            m.write(PACKET,struct.pack('<BHBHH',source,start_src,dest,start_dst,count))
            entry=t['bankCopySector'] if mode=='rom' else k['mx_memcpyFar']
            elapsed,regs=run(caller,entry,PACKET)
            assert bool(regs['af']&1)==bool(error) and regs['af']>>8==error,(caller,source,dest,count,regs)
            assert (regs['hl'],regs['bc'],regs['de'])==(PACKET,0x3456,0x5678),regs
            assert m.read(t['bankOwner'],7)==before
            if initial_window is not None:
                assert m.read(0xffc0,32)==initial_window
            if not dest&0x80 and dest<=16:
                page(dest)
                actual=m.read(start_dst-1,count+2)
                expected=b'\xa5'+(bytes([0xcc])*count if error else payload)+b'\x5a'
                assert actual==expected,(caller,source,dest,count,offset,seed,
                    next(i for i,(a,c) in enumerate(zip(actual,expected,strict=True)) if a!=c))
            if not error and count:
                page(source)
                assert m.read(start_src,count)==payload
            yield dict(operation=mode,caller=caller,source=source,dest=dest,count=count,offset=offset,
                       windowSeed=seed,error=error,ticks=elapsed)

    def move(caller, target, offset, count, delta, *, far=False):
        addr=(0xa000 if target==1 else 0x4000)+offset
        first=min(addr,addr+delta)-1
        size=count+abs(delta)+2
        original=bytes((i*17+(i>>3)+13)&255 for i in range(size))
        expected=bytearray(original)
        expected[addr+delta-first:addr+delta-first+count]=original[addr-first:addr-first+count]
        page(target)
        resident='MEM_EXEC_READY' in k
        runnable=not resident or m.read(k['MEM_EXEC_READY'],1)==bytes([k['MEM_EXEC_READY_VALUE']])
        m.write(first,original)
        page(caller)
        m.write(0xffc0,window)
        m.write(PACKET,struct.pack('<BHBHH',target,addr,target,addr+delta,count))
        context=m.read(t['bankOwner'],7)
        near_ready=m.read(k['MEM_NEAR_COPY'],2)!=b'\0\0'
        unsupported=not local_move and resident and (not runnable or target==caller and not near_ready)
        error=k['MEM_ERROR_UNSUPPORTED'] if count and unsupported else 0
        if error:
            expected=original
        entry=k['mx_memcpyFar' if far else 'mx_memory']
        ticks,regs=run(caller,entry,PACKET,af=0x0057)
        assert regs['af']>>8==error and bool(regs['af']&1)==bool(error),(caller,target,count,regs)
        assert (regs['hl'],regs['bc'],regs['de'])==(PACKET,0x3456,0x5678),regs
        assert m.read(t['bankOwner'],7)==context
        if local_move:
            assert m.read(0xffc0,32)==window
        page(target)
        assert m.read(first,size)==expected,(caller,target,offset,count,delta)
        return dict(operation='far-overlap' if far else 'move',caller=caller,source=target,dest=target,offset=offset,count=count,
                    delta=delta,error=error,ticks=ticks)

    def bios_cases():
        cursor=b['bios_vars.cursorY']
        mem_top=b['v_memTop']
        hook, counter, nested=0x8300,0x8400,0x8402
        routines=[('mx_printChar','bios_printChar',0xf81b,True),
                  ('mx_getCursorPos','bios_getCursorPos',0xf81e,False),
                  ('mx_setCursorPos','bios_setCursorPos',0xf821,False),
                  ('mx_printCharRaw','bios_printCharRaw',0xf824,True),
                  ('mx_getMemTop','bios_getMemTop',0xf830,False),
                  ('mx_setMemTop','bios_setMemTop',0xf833,False)]
        routines.extend(row for row in (('mx_printHexByte','bios_printHexByte',0xf827,True),
                                       ('mx_printHexWord','bios_printHexWord',0xf82a,True)) if row[0] in k)
        for name,old,address,prints in routines:
            assert k[name]==address,(name,k.get(name))
            page(0)
            saved_vram=m.read(0x9000,0x3000) if prints else None
            saved_cursor=m.read(cursor,2)
            saved_top=m.read(mem_top,2)
            args=dict(hl=0x877f if name=='mx_setMemTop' else 0x120a,af=0x5893,bc=0x3451,de=0x5678)

            def restart():
                page(0)
                m.write(cursor,WORD(0x100a))
                m.write(mem_top,WORD(0x8bff))
                if prints:
                    m.write(0x9000,saved_vram)

            def observe():
                page(0)
                return m.read(cursor,2),m.read(mem_top,2),m.read(0x9000,0x3000) if prints else None

            restart()
            direct_ticks,direct_regs=run(0,b[old],**args)
            direct_state=observe()
            for caller in (0,1,6):
                page(caller)
                initial_context=m.read(t['bankOwner'],7)
                body=b'\x21'+WORD(counter)+b'\x34\xcd'+WORD(k['mx_getCursorPos'])+b'\x22'+WORD(nested)+b'\xc9'
                m.write(hook,body)
                for after_copy in (False,True):
                    for queued in (False,True):
                        restart()
                        page(caller)
                        m.write(0xffc0,window)
                        if after_copy:
                            m.write(PACKET,struct.pack('<BHBHH',2,0x2000,4,0x5000,32))
                            run(caller,k['mx_memcpyFar'],PACKET)
                        original_window=m.read(0xffc0,32)
                        m.write(counter,bytes(4))
                        run(caller,k['mx_queueBind'],hook if queued else 0)
                        ticks,regs=run(caller,address,**args)
                        assert regs==direct_regs,(name,caller,regs,direct_regs)
                        assert m.read(t['bankOwner'],7)==initial_context
                        assert m.read(0xffc0,32)==original_window
                        assert m.read(counter,1)==bytes([queued]),(name,caller,queued)
                        if queued:
                            assert m.read(nested,2)==WORD(0x100a)
                        run(caller,k['mx_queueBind'],0)
                        assert observe()==direct_state,(name,caller,after_copy,queued)
                        yield dict(operation=name,caller=caller,afterCopy=after_copy,queued=queued,ticks=ticks,
                                   direct=direct_ticks,result=regs)
            page(0)
            if prints:
                m.write(0x9000,saved_vram)
            m.write(cursor,saved_cursor)
            m.write(mem_top,saved_top)
            print(name,'PASS',flush=True)

        # Обработчик очереди печатает до того, как внешний CALL использует исходный C.
        page(0)
        vram=m.read(0x9000,0x3000)
        initial_cursor=m.read(cursor,2)
        args=dict(af=0x5893,bc=0x3451,de=0x5678,hl=0x1234)
        m.write(cursor,WORD(0x100a))
        run(0,b['bios_printChar'],**dict(args,bc=0x344e))
        _,direct_regs=run(0,b['bios_printChar'],**args)
        expected_cursor=m.read(cursor,2)
        expected_vram=m.read(0x9000,0x3000)
        for caller in (0,1,6):
            page(0)
            m.write(0x9000,vram)
            m.write(cursor,WORD(0x100a))
            page(caller)
            m.write(0xffc0,window)
            m.write(counter,b'\0')
            body=b'\x21'+WORD(counter)+b'\x34\x0e\x4e\xcd'+WORD(k['mx_printChar'])+b'\xc9'
            m.write(hook,body)
            run(caller,k['mx_queueBind'],hook)
            before=m.read(t['bankOwner'],7)
            ticks,result=run(caller,k['mx_printChar'],**args)
            assert result==direct_regs and m.read(counter,1)==b'\x01'
            assert m.read(t['bankOwner'],7)==before and m.read(0xffc0,32)==window
            run(caller,k['mx_queueBind'],0)
            page(0)
            assert m.read(cursor,2)==expected_cursor and m.read(0x9000,0x3000)==expected_vram
            yield dict(operation='nested-print',caller=caller,ticks=ticks,result=result)
        page(0)
        m.write(0x9000,vram)
        m.write(cursor,initial_cursor)
        print('nested-print PASS',flush=True)

    def text_cases():
        kernel=symbols(bundle/'bin/banked/KERNEL.sym')
        source,cursor=0x8400,b['bios_vars.cursorY']
        clean_video=bytes(0x3000)
        modes=[('z','bios_printStringZ','Z'),
               ('raw-z','bios_printStringRawZ','RAW_Z'),
               ('raw-lim','bios_printStringRawLim','RAW_LIM')]
        if 'MXOS_BIOS_PRINT_STRING_LIM' in k:
            modes.append(('lim','bios_printStringLim','LIM'))
        for length in args.text_lengths:
            controls=[('lim-controls','bios_printStringLim','LIM')] if length==33 and 'MXOS_BIOS_PRINT_STRING_LIM' in k else []
            for mode,entry,id_suffix in modes+controls:
                payload=(b'Abc123! ' * ((length+7)//8))[:length]
                if mode=='lim-controls':
                    payload=b'A\0B\rC\n'+b'X'*25+b'\x1b)'
                routes=[(0,'bios',b[entry],None)]
                for caller in (1,6):
                    syscall=k['MXOS_BIOS_PRINT_STRING_'+id_suffix]
                    routes.append((caller,'regs',k['mx_callRegs'],syscall))
                    routes.append((caller,'packet',k['mx_callPacket'],syscall))
                if mode=='z':
                    routes.append((1,'dos-native',kernel['bankDosPrintString'],None))
                expected=None
                page(0)
                inverse,esc=b['bios_vars.inverse'],b['bios_upper.escMode']
                initial_inverse,initial_esc=m.read(inverse,2),m.read(esc,1)
                if mode=='lim-controls':
                    scalar=b''.join(b'\x0e'+bytes([ch])+b'\xcd'+WORD(b['bios_printChar']) for ch in payload)
                    m.write(0x8300,scalar+b'\xc9')
                    m.write(0x9000,clean_video)
                    m.write(cursor,WORD(0x100a))
                    run(0,0x8300,source)
                    expected=(m.read(cursor,2),m.read(0x9000,0x3000),m.read(inverse,2),m.read(esc,1))
                for caller,route,address,inline in routes:
                    page(0)
                    m.write(inverse,initial_inverse)
                    m.write(esc,initial_esc)
                    m.write(0x9000,clean_video)
                    m.write(cursor,WORD(0x100a))
                    page(caller)
                    m.write(source,payload+b'\0poison')
                    before=m.read(t['bankOwner'],7)
                    if route=='packet':
                        m.write(PACKET,struct.pack('<4H',source,0x1234,length,0x5a92))
                        elapsed,regs=run(caller,address,source,af=0x5a92,bc=inline,de=PACKET)
                        assert m.read(PACKET,8)==struct.pack('<4H',source+length,0x1234,length,0x5a92)
                    else:
                        elapsed,regs=run(caller,address,source,af=0x5a92,bc=length,
                                         de=0x1234,inline=inline)
                    assert (regs['af'],regs['hl'],regs['bc'],regs['de'])==(0x5a92,source+length,length,0x1234),(caller,mode,length,regs)
                    assert m.read(t['bankOwner'],7)==before,(caller,route,mode,length)
                    assert m.read(source,length+7)==payload+b'\0poison'
                    page(0)
                    state=(m.read(cursor,2),m.read(0x9000,0x3000),m.read(inverse,2),m.read(esc,1))
                    if expected is None:
                        expected=state
                    else:
                        assert expected==state,(caller,route,mode,length)
                    yield dict(operation=mode,caller=caller,route=route,length=length,
                               ticks=elapsed,result=regs)
        print('text PASS',flush=True)

    def text_main_cases():
        """Сверить MAIN-адрес, регистры и экран при вызове из MAIN, DOS и EXEC."""
        page(0)
        cursor, inverse, esc = b['bios_vars.cursorY'], b['bios_vars.inverse'], b['bios_upper.escMode']
        inverse0, esc0 = m.read(inverse, 2), m.read(esc, 1)

        def reset_screen():
            page(0)
            m.write(0x9000, bytes(0x3000))
            m.write(cursor, WORD(0x100a))
            m.write(inverse, inverse0)
            m.write(esc, esc0)

        def screen():
            page(0)
            return (m.read(0x9000, 0x3000), m.read(cursor, 2), m.read(inverse, 2), m.read(esc, 1))

        dest = 0x8700  # В MAIN ниже memTop стенда, в DOS выше конца ядра.
        for mode, vector in (('Z', 'bios_printStringZ'), ('RAW_Z', 'bios_printStringRawZ'),
                             ('RAW_LIM', 'bios_printStringRawLim'), ('LIM', 'bios_printStringLim')):
            op = k['MXOS_BIOS_PRINT_STRING_' + mode + '_MAIN']
            for length in (0, 33, 1024):
                data = bytearray((b'Abc123! ' * 128)[:length])
                if length:
                    data[30:33] = b'\x1b)A'
                    if mode.endswith('LIM'):
                        data[5] = 0
                data += b'\0Poison'
                reset_screen()
                m.write(dest, data)
                direct, _ = run(0, b[vector], dest, bc=length)
                expected = screen()
                yield dict(mode=mode, length=length, caller=0, route='direct', ticks=direct)
                for caller in (0, 1, 6):
                    for abi in ('regs', 'packet'):
                        reset_screen()
                        page(caller)
                        # То же 16-битное значение в банке клиента указывает на другие байты.
                        if caller:
                            m.write(dest, b'BAD\0')
                        registers = (dest, 0x3456, length, 0x5a92)
                        result = (dest + length, *registers[1:])
                        if abi == 'packet':
                            m.write(PACKET, struct.pack('<4H', *registers))
                            ticks, _ = run(caller, k['mx_callPacket'], 0, bc=op, de=PACKET)
                            assert m.read(PACKET, 8) == struct.pack('<4H', *result), (caller, mode, abi)
                        else:
                            ticks, regs = run(caller, k['mx_callRegs'], dest, af=registers[3],
                                              bc=length, de=registers[1], inline=op)
                            assert tuple(regs[r] for r in ('hl', 'de', 'bc', 'af')) == result, (caller, mode, regs)
                        assert screen() == expected, (caller, mode, length, abi)
                        assert m.read(dest, len(data)) == data
                        yield dict(mode=mode, length=length, caller=caller, route=abi, ticks=ticks)

        # Тот же корпус и то же тело gridBankFar, что у выпускаемого FontBen.
        grid = symbols(bundle/'bin/FONTGRID.sym')
        app = symbols(bundle/'bin/FONTCOM.sym')
        length = grid['GRID_TEXT_BYTES']
        start = app['gridText']
        corpus = (bundle/'bin/FONTCOM.i80').read_bytes()[start:start+length] + b'\0'
        page(6)
        module = (bundle/'bin/FONTGRID.i80').read_bytes()
        m.write(grid['entry'], module[:grid['IMAGE_BODY_SIZE']])
        source = grid['gridCorpus']
        m.write(source, corpus)
        for kind, (mode, vector) in enumerate((('Z', 'bios_printStringZ'), ('RAW_Z', 'bios_printStringRawZ'),
                                              ('RAW_LIM', 'bios_printStringRawLim')), start=2):
            copied = length + (mode != 'RAW_LIM')
            reset_screen()
            m.write(cursor, bytes(2))
            m.write(dest, corpus)
            direct, _ = run(0, b[vector], dest, bc=length)
            expected = screen()
            yield dict(mode=mode, length=length, caller=0, route='fontben-direct', ticks=direct)
            reset_screen()
            m.write(cursor, bytes(2))
            op = k['MXOS_BIOS_PRINT_STRING_' + mode]
            near, _ = run(6, k['mx_callRegs'], source, bc=length, inline=op)
            assert screen() == expected, (mode, 'near')
            yield dict(mode=mode, length=length, caller=6, route='fontben-near', ticks=near)
            reset_screen()
            m.write(cursor, bytes(2))
            m.write(dest - 1, bytes([0xa5]) * (copied + 2))
            page(6)
            m.write(grid['gridKind'], bytes([kind]))
            m.write(grid['gridFarCopy'], struct.pack('<BHBHH', 6, source, 0, dest, copied))
            far, regs = run(6, grid['gridBankFar'], 0)
            assert not regs['af'] & 1 and regs['hl'] == dest + length, regs
            assert screen() == expected, (mode, 'far1024')
            assert m.read(dest - 1, copied + 2) == b'\xa5' + corpus[:copied] + b'\xa5'
            yield dict(mode=mode, length=length, caller=6, route='fontben-far1024', ticks=far)

    def window_cost_cases():
        """Сравнить непрерывную печать и те же порции на одном движке в MAIN."""
        page(0)
        source,cursor=0x8400,b['bios_vars.cursorY']
        font=b['FONT_ENGINE_TEXT_LIM']
        # Обе контрольные точки вызывают тот же счётный вход, что и банковый Z.
        assert m.read(b['FONT_ENGINE_TEXT_EXT'],2)==WORD(font)
        # В отдельном проходе считаем только вызовы одиночной отрисовки из Z/LIM.
        rotate=symbols(bundle/'bin/fonts/Rotate.sym')
        scalar_call=b'\xcd'+WORD(rotate['printCharBody'])
        counter,probe=0x8300,0x8310
        sites=[]
        for vector,entry,fallback in ((b['FONT_ENGINE_Z'],'printStringZ','printStringChar'),
                                      (font,'printStringLim','stringLimControl')):
            assert m.read(vector,3)==b'\xc3'+WORD(rotate[entry]),'Нужен штатный Rotate'
            start=rotate[fallback]
            body=m.read(start,12)
            assert body.count(scalar_call)==1
            sites.append(start+body.index(scalar_call)+1)
        # PUSH HL / LHLD / INX H / SHLD / POP HL / JMP сохраняют аргументы и флаги.
        m.write(probe,b'\xe5\x2a'+WORD(counter)+b'\x23\x22'+WORD(counter)+b'\xe1\xc3'+scalar_call[1:])
        for start_x in (16,0):
            for length in (32,64,128,256):
                payload=b'Abc123! '*(length//8)+b'\0'
                expected=None
                for route,entry,block in (
                        ('main-z',b['bios_printStringZ'],length),
                        ('main-lim',b['bios_printStringLim'],length),
                        ('font-whole',font,length),
                        ('font-32',font,32),
                        ('bank-z',k['mx_callRegs'],length),
                        ('bank-lim',k['mx_callRegs'],length)):
                    page(0)
                    m.write(0x9000,bytes(0x3000))
                    m.write(cursor,bytes((10,start_x)))
                    caller=6 if route.startswith('bank-') else 0
                    page(caller)
                    m.write(source,payload)
                    op={'bank-z':'Z','bank-lim':'LIM'}.get(route)
                    inline=k['MXOS_BIOS_PRINT_STRING_'+op] if op else None
                    portions=[]
                    for offset in range(0,length,block):
                        elapsed,regs=run(caller,entry,source+offset,bc=block,inline=inline)
                        assert regs['hl']==source+offset+block
                        page(0)
                        y,x=m.read(cursor,2)
                        portions.append(dict(ticks=elapsed,x=x,y=y))
                    state=(m.read(cursor,2),m.read(0x9000,0x3000))
                    if expected is None:
                        expected=state
                    assert state==expected,(route,start_x,length)
                    row=dict(route=route,start_x=start_x,length=length,portions=portions,
                             ticks=sum(part['ticks'] for part in portions))
                    if route in ('main-z','bank-z'):
                        m.write(0x9000,bytes(0x3000))
                        m.write(cursor,bytes((10,start_x)))
                        m.write(counter,WORD(0))
                        try:
                            for site in sites:
                                m.write(site,WORD(probe))
                            counted,_=run(caller,entry,source,bc=length,inline=inline)
                            page(0)
                            scalar=int.from_bytes(m.read(counter,2),'little')
                            assert counted-elapsed==68*scalar
                            assert (m.read(cursor,2),m.read(0x9000,0x3000))==expected
                            row['scalar_glyphs']=scalar
                        finally:
                            page(0)
                            for site in sites:
                                m.write(site,scalar_call[1:])
                    yield row

    def legacy_cases():
        # Номера BIOS из MEM rev64, снятые с публичного ABI в rev65.
        historical_ids=(('PRINT_CHAR',0x0102),('GET_CURSOR_POS',0x0109),
                        ('SET_CURSOR_POS',0x010a),('PRINT_CHAR_RAW',0x0114),
                        ('PRINT_HEX_BYTE',0x0106),('PRINT_HEX_WORD',0x010e),
                        ('GET_MEM_TOP',0x0110),('SET_MEM_TOP',0x010f))
        cursor,mem_top=b['bios_vars.cursorY'],b['v_memTop']
        page(0)
        video=m.read(0x9000,0x3000)
        original_cursor=m.read(cursor,2)
        original_top=m.read(mem_top,2)
        for name,op in historical_ids:
            symbol='MXOS_BIOS_'+name
            assert k.get(symbol,op)==op, (symbol,k[symbol],op)
            args=dict(af=0x5893,bc=0x3451,de=0x5678,hl=0x877f if name=='SET_MEM_TOP' else 0x120a)
            supported=symbol in k
            for caller in (0,6):
                for mode in ('inline',) if supported else ('packet','inline'):
                    page(0)
                    m.write(cursor,WORD(0x100a))
                    m.write(mem_top,WORD(0x8bff))
                    m.write(0x9000,video)
                    page(caller)
                    m.write(0xffc0,window)
                    context=m.read(t['bankOwner'],7)
                    packet=struct.pack('<4H',*(args[r] for r in ('hl','de','bc','af')))
                    m.write(PACKET,packet)
                    if mode=='inline':
                        ticks,result=run(caller,k['mx_callRegs'],inline=op,**args)
                    else:
                        ticks,result=run(caller,k['mx_callPacket'],args['hl'],af=args['af'],de=PACKET,bc=op)
                    assert m.read(t['bankOwner'],7)==context
                    assert m.read(PACKET,len(packet))==packet
                    if not supported:
                        assert result['af']&1 and result['af']>>8==k['DOS_ERR_UNSUPPORTED'],(name,caller,mode,result)
                        page(0)
                        assert m.read(cursor,2)==WORD(0x100a) and m.read(mem_top,2)==WORD(0x8bff)
                        assert m.read(0x9000,0x3000)==video
                    yield dict(operation=name,number=op,caller=caller,mode=mode,supported=supported,ticks=ticks,result=result)
        page(0)
        m.write(0x9000,video)
        m.write(cursor,original_cursor)
        m.write(mem_top,original_top)
        print('legacy BIOS IDs PASS',flush=True)

    def user_case():
        bank_nc=symbols(bundle/'bin/NC_CORE.sym')
        main_nc=symbols(bundle/'bin/NC.sym')
        cli=symbols(bundle/'bin/CLI.sym')
        m.stop_at(bank_nc['ncBankInputPoll'])
        for _ in range(32):
            m.until(bank_nc['ncBankInputPoll'],30)
            active=m.read(t['bankPage'],1)[0]
            if active>1:
                break
            m.call('debug_step',mode='into')
        else:
            raise RuntimeError('Банковый NC не дошёл до опроса клавиатуры')
        m.call('bp_clear')
        m.call('emu_run_for',ms=500)
        m.stop_at(cli['cliPrompt'])
        for attempt in range(4):
            m.tap('f10')
            try:
                m.until(cli['cliPrompt'],8)
                assert m.read(t['bankPage'],1)==b'\x00'
                break
            except RuntimeError:
                if attempt==3:
                    raise
        m.call('bp_clear')
        m.call('emu_run_for',ms=500)
        def enter(text, addr):
            m.call('bp_clear')
            m.call('emu_run_for',ms=500)
            m.tap('esc')
            m.call('emu_run_for',ms=250)
            names={':':'colon','/':'slash','.':'period',' ':'space'}
            for char in text.lower():
                if char=='_':
                    m.call('kbd_send',keys=['shift'],action='press')
                    try:
                        m.tap('0')
                    finally:
                        m.call('kbd_send',keys=['shift'],action='release')
                else:
                    m.tap(names.get(char,char))
            assert m.read(addr,len(text)).upper()==text.encode(),text

        command='C:/DOS/NC_MN.COM'
        enter(command,cli['CMD_LINE'])
        m.stop_at(main_nc['mainLoop'])
        m.tap('enter')
        m.until(main_nc['mainLoop'],30)
        assert m.read(t['bankPage'],1)==b'\x00'
        assert m.read(0,4)==(bundle/'bin/NC.i80').read_bytes()[:4]
        print('NC EXEC -> CLI -> NC MAIN PASS',flush=True)
        editors=[]
        for filename,symfile,ready,banked in (('EDIT_MN','EDIT.sym','keyInput',False),
                                              ('EDIT','EDITCORE.sym','ebWaitReady',True)):
            path=bundle/'bin'/symfile
            if not path.exists():
                continue
            editor=symbols(path)
            line='C:/DOS/'+filename+'.COM C:/AUTOEXEC.BAT'
            enter(line,main_nc['vars.cmdLine'])
            m.stop_at(editor[ready])
            m.tap('enter')
            for _ in range(24):
                m.until(editor[ready],15)
                bank=m.read(t['bankPage'],1)[0]
                if (bank>1 if banked else bank==0):
                    break
                m.call('debug_step',mode='into')
            else:
                raise RuntimeError(f'{filename} не вышел на клавиатурный цикл')
            start,lo,hi,end=(int.from_bytes(m.read(editor[key],2),'little') for key in
                             ('v_textBeginLo','v_gapLo','v_gapHi','v_textEnd'))
            assert lo-start>=4 and m.read(start,4)==b'PATH', (filename,bank,start,lo)
            assert end>start and hi>=lo
            m.call('bp_clear')
            m.call('emu_run_for',ms=500)
            m.stop_at(main_nc['mainLoop'])
            m.tap('f10')
            m.until(main_nc['mainLoop'],30)
            assert m.read(t['bankPage'],1)==b'\x00'
            assert m.read(0,4)==(bundle/'bin/NC.i80').read_bytes()[:4]
            editors.append(dict(profile=filename,page=bank,bytes=end-start,returned_to_main=True))
            print(filename,'PASS',flush=True)
        return dict(operation='keyboard',exec_page=active,command=command,main_page=0,editors=editors)

    assert not owner(),f'Порт {args.port} занят'
    assert exe.exists()
    hidden_qt(runtime)
    # Применяем снимок только в частном runtime, после подтверждения свободного слота.
    shutil.copyfile(image,runtime/'specialist-os/system-flash.bin')
    launch=ROOT/'scripts/emu80/run.ps1'
    stdout=shell(f'$p=& {ps(launch)} -EmulatorDirectory {ps(runtime)} -Hidden -Flash -Mcp -McpPort {args.port} -PassThru; $p.Id')
    pid=int(stdout.splitlines()[-1])
    try:
        for _ in range(30):
            if owner()==str(pid):
                break
            time.sleep(.1)
        else:
            raise RuntimeError('PID скрытого экземпляра не подтвердил MCP listener')
        path=shell(f'Get-Process -Id {pid} | Select-Object -ExpandProperty Path')
        assert Path(path).resolve()==exe.resolve()
        assert shell(f'(Get-Process -Id {pid}).MainWindowHandle.ToInt64()')=='0'
        system=m.call('sys_info')
        assert system['cpu']['speed_factor']==32 and not system['cpu']['full_throttle']
        launch_conf=(runtime/'specialist-os/launch.conf').read_text()
        assert 'pitSoundSource.muted = yes' in launch_conf and 'tapeMonitor.muted = yes' in launch_conf
        assert digest(runtime/'specialist-os/system-flash.bin')==digest(image)
        m.call('debug_break')
        m.stop_at(b['biosBootExec'])
        m.call('emu_reset')
        m.until(b['biosBootExec'],30)
        assert m.read(b['biosBootPages'],1)==b'\x10'
        assert m.read(t['bankPage'],1)==b'\0'
        assert m.read(k['MEM_RUNTIME_REVISION_ADDR'],2)==WORD(t['MEM_RUNTIME_REVISION'])
        rows=[]
        if args.suite=='user':
            rows.append(user_case())
        else:
            # DATA остаётся с хвостом, установленным bootstrap; EXEC получает штатные сегменты.
            page(6)
            for address,count,offset in segments:
                if address!=0xffc0:
                    m.write(address,mem[offset:offset+count])
            m.write(t['bankPage'],b'\x06')
            m.write(t['bankOwner'],b'\x0d\x06')
            m.write(t['bankDepth'],b'\x00')
            m.write(t['bankPageCount'],b'\x10')
            m.write(t['bankServiceSP'],WORD(t['DOS_BANK_STACK_TOP']))
            m.write(t['bankStackFloor'],WORD(t['DOS_BANK_STACK_BOTTOM']+64))
            m.write(k['MEM_BANK_KIND'],bytes([k['MEM_BANK_EXEC']]))
            if 'memExecCopy' in k:
                m.write(k['MEM_NEAR_COPY'],WORD(k['memExecCopy']))
            if 'MEM_EXEC_READY' in k:
                m.write(k['MEM_EXEC_READY'],bytes([k['MEM_EXEC_READY_VALUE']]))
            for physical,marker in markers.items():
                page(physical)
                m.write(0x86a0,marker)
            for physical,marker in markers.items():
                page(physical)
                assert m.read(0x86a0,4)==marker
        with (out/'samples.jsonl').open('w',encoding='utf-8') as sink:
            def save(row):
                rows.append(row)
                sink.write(json.dumps(row)+'\n')
                sink.flush()
            if args.suite in ('copy','all'):
                lengths=(0,1,2,3,4,15,16,17,30,31,32,33,63,64,65,127,128,129,255,256,257,511,512,513,8191,8192)
                for caller,source,dest in ((0,0,3),(0,2,0),(0,2,4),(1,1,3),(6,6,3),(6,3,6),(6,2,4)):
                    for count in lengths:
                        for offset in ((0,1,255) if count in (31,32,33,255,256,257,8192) else (1,)):
                            for row in copy(caller,source,dest,count,offset):
                                save(row)
                    print('FAR',caller,source,dest,'PASS',flush=True)
                for caller,source,dest in ((0,0x80,3),(1,0x85,3),(6,0x80,6)):
                    for offset in (0,1,255):
                        for row in copy(caller,source,dest,256,offset,'rom'):
                            save(row)
                for caller,src,dst,count,error in ((0,17,3,1,0xe0),(0,2,3,1,0xe2),(6,17,17,0,0)):
                    for row in copy(caller,src,dst,count,0,'far',error):
                        save(row)
                for caller,target in ((0,0),(0,3),(1,3),(6,6),(6,3)):
                    for count in (0,1,31,32,33,255,256,257,511,512,513,8192):
                        for delta in (-1,1):
                            save(move(caller,target,1,count,delta))
                            if local_move and target==3:
                                save(move(caller,target,1,count,delta,far=True))
            if args.suite in ('bios','all'):
                for row in bios_cases():
                    save(row)
            if args.suite=='remote-move':
                routes=((0,6),(1,6),(6,0),(6,1))
                if local_move:
                    routes+=((0,3),(1,3),(6,3))
                for caller,target in routes:
                    for count in (0,1,31,32,33,255,256,257,511,512,513,8192):
                        for delta in (-1,1):
                            save(move(caller,target,1,count,delta))
            if args.suite=='local-copy':
                for caller,target in ((0,0),(0,3),(0,6),(1,6),(6,0),(6,1),(6,6)):
                    for count in (1,31,32,33,255,256,257,8192):
                        for row in copy(caller,target,target,count,1):
                            save(row)
            if args.suite=='legacy':
                for row in legacy_cases():
                    save(row)
            if args.suite=='text':
                for row in text_cases():
                    save(row)
            if args.suite=='text-windows':
                for row in window_cost_cases():
                    save(row)
            if args.suite=='text-main':
                for row in text_main_cases():
                    save(row)
        names=('MEM.sys','BIOS.sys','DOS.sys','IO.sys','banked/CLIENT.bin','banked/LOCALBLOCK.bin',
               'banked/TRANSPORT.bin','banked/WINDOW.bin','banked/MAINBIOS.bin')
        report=dict(revision=t['MEM_RUNTIME_REVISION'],image_sha256=digest(image),emu_sha256=digest(exe),
                    cpu=system['cpu'],hidden=True,components={name:(bundle/'bin'/name).stat().st_size for name in names},
                    rows=rows)
        (out/'result.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
        print('PASS',len(rows),flush=True)
    finally:
        shell(f'$p=Get-Process -Id {pid} -ErrorAction SilentlyContinue; if($p -and $p.Path -eq {ps(exe)}) '
              '{ Stop-Process -InputObject $p; $p.WaitForExit() }; exit 0')


if __name__=='__main__':
    main()
