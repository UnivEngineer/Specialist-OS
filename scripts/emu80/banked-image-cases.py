"""Сценарии загрузки образов; запускаются внутри собственного стенда kernel."""


def check_images(m, ks, bios, select, invoke, request, regs, rows):
    word = lambda n: n.to_bytes(2, 'little')

    def checksum(data):
        total = sum(data)
        prefix = total-data[-1]
        return ((prefix+(prefix >> 8)) & 255) << 8 | (total & 255)

    def container(body, address=0x5000):
        header = b'\xcd MX\1\0'+word(address)+word(address)+word(len(body))+word(checksum(body))
        return header+word(checksum(header))+body

    def save(page, name, data):
        select(page)
        path = ('B:/'+name).encode('ascii')
        m.write(0x3200, path+bytes(128-len(path)))
        m.write(0x3100, word(0x3200)+word(0x5000)+word(len(data))+bytes(34))
        m.write(0x5000, data)
        request(page, ks['MX_DOS_FILESAVEPATH'], regs(hl=0x3100))

    def open_image(page, name, error=0, legacy=False):
        select(page)
        base, ext = name.split('.')
        m.write(0x3300, base.encode().ljust(8)+ext.encode().ljust(3)+bytes(21))
        result = invoke(page, ks['BANK_DOS_GATE'], regs(hl=0x3300, a=16),
                        ks['MX_DOS_IMAGEOPEN'], legacy=bios['bios_imageOpen'] if legacy else None)
        assert result[1] == bool(error), ('image-open', page, name, result)
        if error: assert result[0] == error, (name, result, error)
        return m.call('cpu_state')['registers']

    def load_image(page, target=0x5000, error=0, legacy=False):
        result = invoke(page, ks['BANK_DOS_GATE'], regs(de=target), ks['MX_DOS_FILELOAD'],
                        legacy=bios['bios_fileLoad'] if legacy else None)
        assert result[1] == bool(error), ('image-load', page, result)
        if error: assert result[0] == error, (result, error)
        return m.call('cpu_state')['registers']

    for page, length in ((0,1), (3,257), (0,4096), (3,4097), (0,8193)):
        body = bytearray((i*71+length+i//31)&255 for i in range(length))
        body[:min(6,length)] = (b'\x3e\x5a\x32\x00\x37\xc9' if length >= 6 else b'\xc9')[:min(6,length)]
        data = container(body)
        save(page, 'IMAGE.COM', data)
        r = open_image(page, 'IMAGE.COM', legacy=page==0)
        assert (r['hl']['dec'],r['de']['dec'],r['bc']['dec']) == (0x5000,0x5000,length)
        assert int.from_bytes(m.read(0x331C,4),'little') == len(data)
        if page == 0:
            # Открытый курсор не может быть присвоен другим клиентом.
            load_image(3, error=7)
            select(page)
        m.write(0x5000, bytes([0xA5])*(length+16))
        r = load_image(page, legacy=page==0)
        assert r['hl']['dec'] == 0x5000
        assert m.read(0x5000,length) == body
        assert m.read(0x5000+length,16) == bytes([0xA5])*16
        # Проверяем исполнение именно в странице клиента, обычный RET.
        m.write(0x3700,b'\0');m.write(0x3EFE,word(0x3E00));m.stop_at(0x3E00)
        m.call('regs_set',pc=0x5000,sp=0x3EFE)
        state = m.until(0x3E00,10)
        assert state['registers']['sp']['dec'] == 0x3F00
        assert m.read(0x3700,1) == (b'Z' if length >= 6 else b'\0')
        load_image(page,error=7)  # Подготовка расходуется ровно один раз.
        # Исторический load-at читает весь файл, включая заголовок.
        packet = regs(hl=0x3300,de=0x6000,bc=0x7788)
        # imageOpen сохраняет исходное имя в дескрипторе клиента.
        m.write(0x6000,bytes([0xA5])*(len(data)+16))
        request(page,ks['MX_DOS_FILELOAD2'],packet)
        assert m.read(0x3000,6) == packet[:6]
        assert m.read(0x6000,len(data)) == data
        assert m.read(0x6000+len(data),16) == bytes([0xA5])*16
        rows.append(dict(case='image-load-verify-execute-client-and-raw-load',caller=page,bytes=length))

    # Повреждения заголовка и тела дают BAD_IMAGE, а не передачу управления.
    data = bytearray(container(bytes(range(256))*17))
    data[14] ^= 1
    save(3,'BAD.COM',data)
    open_image(3,'BAD.COM',error=11)
    load_image(3,error=7)
    data[14] ^= 1;data[-1] ^= 1
    save(3,'BAD.COM',data)
    open_image(3,'BAD.COM')
    load_image(3,error=11)
    rows.append(dict(case='image-bad-header-and-body-checksum'))
    save(0,'IMAGE.COM',container(b'\xc9'*257))
    open_image(0,'IMAGE.COM')
    load_image(0,target=0xF7FF,error=7)
    load_image(0)  # Ошибка диапазона не расходует подготовленный образ.
    open_image(0,'IMAGE.COM')
    assert invoke(0,ks['BANK_DOS_GATE'],regs(),ks['MX_DOS_FILEGETSETDRIVE']) == (1,False)
    load_image(0,error=7)
    request(0,ks['MX_DOS_FILELOAD2'],regs(hl=0x3300,de=0xF7FF),expected=7)
    request(0,ks['MX_DOS_FILELOAD2'],regs(hl=0x3300,de=0x3301),expected=7)
    rows.append(dict(case='image-range-context-invalidation-raw-overlap'))
    print('BANK DOS IMAGE LOAD PASS',flush=True)
