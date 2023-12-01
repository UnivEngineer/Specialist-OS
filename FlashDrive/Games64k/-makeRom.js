//----------------------------------------------------------------------------
// MXOS
// Создание образа диска из отдельных файлов
//
// 2013-11-01 Разработано vinxru
// 2022-02-15 Доработано SpaceEngineer
//----------------------------------------------------------------------------

// Основные настройки
volumeSize    = 64*1024         // Размер всего диска в байтах
chipSize      = 64*1024         // Размер одной микросхемы ПЗУ в байтах
maxFiles      = 64;             // Максимум файлов в корневом каталоге
volumeLabel   = "ROM DISK 64"   // Метка тома, 11 символов

// Создавать ли загрузочный диск (ПЗУ), и имя файла,
// который загрузчик должен скопировать в ОЗУ и запустить
makeBootDisk  = 0;
bootFile      = "";

// Поместить ли шрифт по адресу 0x800 и имя файла шрифта
makeRomFont   = 0;
fontFile      = "";

// Файлы, которые должны быть в начале диска
firstFiles = [];

// формат образа ПЗУ:
// 0 - 32 кб для Специалиста-MX
// 1 - 64 кб для Специалиста-MX2
// 2 - флеш-диск (нарезать на файлы размером chipSize)
romFormat = 2

// Имя файла образа ПЗУ
romFileName = "FALSH64k.BIN"

// Путь к папкам, куда сохранять образ ПЗУ
destinationPath = "..\\";
emulatorPath    = "..\\..\\..\\..\\Emulator\\emu\\Specialist\\";

//----------------------------------------------------------------------------

// Расчет вспомогательных значений
fatElemSize   = 2;                              // Размер элемента fat в байтах
sectorSize    = 256;                            // Размер сектора в байтах
secPerClus    = 1;                              // Количество секторов в кластере
volumeSectors = volumeSize / sectorSize;        // Размер всего диска в секторах
fatSectors    = volumeSectors * 2 / sectorSize; // Размер FAT в секторах
dirSectors    = maxFiles * 32 / sectorSize;     // Размер корневого каталога в секторах

fatStartSector  = 1;                                            // Первый сектор таблицы FAT
dirStartSector  = fatStartSector + fatSectors;                  // Первый сектор корневого каталога
dataStartSector = dirStartSector + dirSectors;                  // Первый сектор области данных
dataSectors     = volumeSectors - fatSectors - dirSectors - 1;  // Размер области данных в секторах
dataClusters    = dataSectors / secPerClus;                     // Размер области данных в кластерах

// Стандартная ерунда
fso = new ActiveXObject("Scripting.FileSystemObject");
shell = new ActiveXObject("WScript.Shell");
function kill(name) { if(fso.FileExists(name)) fso.DeleteFile(name); }
function fileSize(name) { return fso.GetFile(name).Size; }
function loadAll(name) { return fso.OpenTextFile(name, 1, false, 0).Read(fileSize(name)); } // File.LoadAll глючит 
function save(fileName, data) { fso.CreateTextFile(fileName).Write(data); }
src = loadAll("tbl.bin"); encode = []; decode = []; for(i=0; i<256; i++) { encode[i] = src.charAt(i); decode[src.charCodeAt(i)] = i; }

// Расчет контрольной суммы файла
function specialistSum(data) {
  s = 0;
  for(i=0; i<data.length-1; i++)
    s += decode[data.charCodeAt(i)] * 257;
  s = (s & 0xFF00) + ((s + decode[data.charCodeAt(i)]) & 0xFF);
  return (s & 0xFFFF);
}

// Удаляем временные файлы
kill("list.tmp");

// Файлы, которые не надо добавлять на диск
ignore = [];
ignore["LIST.TMP"] = 1;
ignore["TBL.BIN"] = 1;
ignore["BOOT.BIN"] = 1;
ignore["-MAKEROM.JS"] = 1;
ignore["SPECSVGA.BIN"] = 1;
ignore["SPMX.BIN"] = 1;
if (makeRomFont == 1)
    ignore[fontFile] = 1;

dir         = "";
dosCluster  = 0;
dosSize     = 0;
dosAddr     = 0;
fontCluster = 0;
numFiles    = 0;

// Загружаем образ boot сектора
bootSector = "BOOT.BIN";
bootSrc = loadAll("boot.bin");
boot = [];
for (i=0; i<256; i++) boot[i] = decode[bootSrc.charCodeAt(i)];

// Пустая FAT
fat = [];
for (i=0; i<4;              i++) fat[i] = 0xFF; // первые два кластера резервные
for (   ; i<fatSectors*256; i++) fat[i] = 0x00; // остальные кластеры свободны

// Пустая область данных
dest = [];

// Выделение кластера
function allocCluster(cluster)
{
    for (;;)
    {
        for (var i=2; i<dataClusters; i++)
        {
            i2 = i * 2;
            if ((fat[i2] == 0) && (fat[i2+1] == 0))
            {
                if (cluster)
                {
                   fat[cluster*2]   = (i & 0xFF);
                   fat[cluster*2+1] = (i >> 8) & 0xFF;
                }
                
                fat[i2]   = (i & 0xFF);
                fat[i2+1] = (i >> 8) & 0xFF;
                return i;
            }
        }

        shell.Popup("Не хватило места!\n" + numFiles, 0, "Error", 0)
        throw "Нет места";
    }
}

// Добавление файла
function putFile(fileName, isBoot)
{
    data = loadAll(fileName);
    data_size = data.length;

    // Файлы нулевого объема не поддерживаются
    //if (data_size==0) return;

    // Проверяем объем
    if (numFiles+1==maxFiles)
    {
        shell.Popup("Максимум файлов: " + maxFiles, 0, "Ошибка", 0);
        throw "Максимум файлов "+maxFiles;
    }
    numFiles++;

    // Получаем адрес загрузки
    startAddr = 0;

    //fileName = fileName.toUpperCase();
    ext = fso.GetExtensionName(fileName);
    if (ext.toUpperCase() == "RKS")
    {
        // Получаем адрес загрузки из заголовка файла
        startAddr = decode[data.charCodeAt(0)] + decode[data.charCodeAt(1)] * 256;
        endAddr   = decode[data.charCodeAt(2)] + decode[data.charCodeAt(3)] * 256;
        len = endAddr - startAddr + 1;
        data = data.substr(4, len);   
    }
    else
    {
        // Получаем адрес загрузки из имени файла
        fileName = fso.GetBaseName(fileName);
        startAddr = fso.GetExtensionName(fileName) * 1;
    }

    // Отрезаем всё лишнее
    fileName = fso.GetBaseName(fileName);

    //shell.Popup(
    //    fileName + "." + ext + "\n" +
    //    startAddr + "\n" +
    //    data_size + "\n\n",
    //    0, "File", 0);

    // Сохраняем файл
    cluster = firstCluster = 0;     // файл нулевого размера указывает на кластер 0
    while (data.length != 0)
    {
        cluster = allocCluster(cluster); 
        if (firstCluster == 0) firstCluster = cluster;
        block = data.substr(0, 256);
        data = data.substr(256);
        while (block.length < 256) block += encode[0xFF];
        dest = dest + block;
    }

    // Дескриптор файла в каталоге
    dir += (fileName+"        ").substr(0,8);   // имя
    dir += (ext+"   ").substr(0,3);             // расширение
    dir += encode[0];                           // attrib
    dir += encode[0];                           // (только FAT32) исп. в WinNT
    dir += encode[0];                           // (только FAT32) время создания - миллисекунды
    dir += encode[0] + encode[0];               // (только FAT32) время создания
    dir += encode[0] + encode[0];               // (только FAT32) дата создания
    dir += encode[startAddr & 0xFF];            // (только FAT32) дата обращения; используем для адреса загрузки - байт 0
    dir += encode[startAddr >> 8];              // (только FAT32) дата обращения; используем для адреса загрузки - байт 1
    dir += encode[0];                           // первый кластер - байт 2
    dir += encode[0];                           // первый кластер - байт 3
    dir += encode[0] + encode[0];               // время записи
    dir += encode[0] + encode[0];               // дата записи
    dir += encode[firstCluster & 0xFF];         // первый кластер - байт 0
    dir += encode[firstCluster >> 8];           // первый кластер - байт 1
    dir += encode[(data_size - 1) & 0xFF];      // размер - байт 0
    dir += encode[(data_size - 1) >> 8];        // размер - байт 1
    dir += encode[0];                           // размер - байт 2
    dir += encode[0];                           // размер - байт 3

    /*shell.Popup(
        fileName + "." + ext + "\n" +
        startAddr + "\n" +
        firstCluster + "\n" +
        data_size + "\n\n",
        0, "File", 0);*/

    // Операционная система
    if (isBoot)
    {
        dosCluster = firstCluster;
        dosAddr = startAddr;
        dosSize = (data_size+255)>>8;
    }
}

// Получение списка файлов
shell.Run("cmd /c dir /b /on *.* >list.tmp", 2, true);
list = fso.OpenTextFile("list.tmp", 1, false, 0);
boolFileFound = "", filesA = [], filesB = [];
while (!list.AtEndOfStream)
{
    fileName = list.readLine();
    fileNameU = fileName.toUpperCase()
    shortFileName = fso.GetBaseName(fso.GetBaseName(fileNameU))+"."+fso.GetExtensionName(fileNameU);

    // Операционная система
    if (makeBootDisk && boolFileFound == "" && shortFileName == bootFile)
        bootFileFound = fileName;
    else if (firstFiles[shortFileName])
        filesA.push(fileName);
    else if (!ignore[fileNameU])
        filesB.push(fileName);
}

//if (makeRomFont)
//{
//  minCluster = 8; // Файл должен начинаться с этого кластера
//  putFile(fontFile);
//}

if (makeBootDisk && bootFileFound)
  putFile(bootFileFound, true);
for (i=0; i<filesA.length; i++)
  putFile(filesA[i], false);
for (i=0; i<filesB.length; i++)
  putFile(filesB[i], false);

// Пустой хвост каталога
while (dir.length < dirSectors*256)
    dir += encode[0xFF];

// Модифицируем некоторые поля boot сектора
boot[0x0B] = sectorSize & 0xFF;     // BPB_BytsPerSec (мл. байт)
boot[0x0C] = sectorSize >> 8;       // BPB_BytsPerSec (ст. байт)
boot[0x0D] = secPerClus & 0xFF;     // BPB_SecPerClus
boot[0x11] = maxFiles & 0xFF;       // BPB_RootEntCnt (мл. байт)
boot[0x12] = maxFiles >> 8;         // BPB_RootEntCnt (ст. байт)
boot[0x13] = volumeSectors & 0xFF;  // BPB_TotSec16 (мл. байт)
boot[0x14] = volumeSectors >> 8;    // BPB_TotSec16 (ст. байт)
boot[0x16] = fatSectors & 0xFF;     // BPB_FATSz16 (мл. байт)
boot[0x17] = fatSectors >> 8;       // BPB_FATSz16 (ст. байт)
for (i=0; i<11; i++)                // BS_VolLab
    boot[0x2B + i] = decode[volumeLabel.charCodeAt(i)];

// Модифицируем код загрузчика
if (makeBootDisk)
{
    dosROMAddr = ((dosCluster - 2) * secPerClus + dataStartSector) * sectorSize;
    boot[0x3F] = dosROMAddr & 0xFF;         // начальный адрес DOS.SYS в ПЗУ (откуда копировать)
    boot[0x40] = dosROMAddr >> 8;
    boot[0x42] = dosAddr & 0xFF;            // начальный адрес DOS.SYS в памяти (куда копировать)
    boot[0x43] = dosAddr >> 8;
    boot[0x4A] = (dosAddr >> 8) + dosSize;  // конечный адрес DOS.SYS в памяти (старший байт + 1)
}

// Собираем образ ПЗУ
start = "";
// Boot сектор
for (i=0; i<256; i++) start += encode[boot[i]];
// FAT
for (i=0; i<fatSectors*256; i++) start += encode[fat[i]];
// Каталог
start += dir;

// Сохраняем образ ПЗУ
if (romFormat == 0)
{
    // Специалист MX
    rom = start + dest;
    while (rom.length < 65536) rom += encode[0xFF];

    // Сохраняем результат
    save(destinationPath + romFileName, rom);
    save(emulatorPath    + romFileName, rom);
}
else if (romFormat == 1)
{
    // Специалист MX2
    // В начале ПЗУ - код перехода на boot сектор (lxi sp, 0F7FFh / rst 0),
    // эти 4 лишних байта учитываются в драйвере ром-диска
    rom = encode[0x31] + encode[0xFF] + encode[0xF7] + encode[0xC7];

    // Первая половина ПЗУ - это вторая половина образа
    rom += dest.substr(32768-start.length, 32768-start.length-4);
    while (rom.length < 32768) rom += encode[0xFF];

    // Вторая половина ПЗУ - это первая половина образа
    rom += start + dest.substr(0, 32768-start.length);
    while (rom.length < 65536) rom += encode[0xFF];

    // Сохраняем результат
    save(destinationPath + romFileName, rom);
    save(emulatorPath    + romFileName, rom);
}
else if (romFormat == 2)
{
    // Флеш диск
    rom = start + dest;
    
    // Разбиваем имя файла
    romFn  = fso.GetBaseName(romFileName);
    romExt = fso.GetExtensionName(romFileName);

    // Нарезаем на куски размером chipSize
    partNum = 0;
    while (rom.length > 0)
    {
        part = rom.substr(0, chipSize);
        rom = rom.substr(chipSize);
        while (part.length < chipSize) part += encode[0xFF];

        // Сохраняем результат
        partFileName = destinationPath + romFn + partNum + "." + romExt;
        save(partFileName, part);
        
        partNum++;
    }
}
