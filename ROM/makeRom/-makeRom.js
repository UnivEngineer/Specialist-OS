//----------------------------------------------------------------------------
// RAMFOS
// Создание образа диска из отдельных файлов
//
// 2013-11-01 Разработано vinxru
//----------------------------------------------------------------------------

fat16   = 1;                    // Сформировать ром-диск в FAT16
fatSize = 1;                    // Размер FAT в кластерах
dirSize = 3;                    // Размер каталога в кластерах
diskSize = (65536 >> 8);        // Размер ПЗУ в кластерах
maxFiles = fat16 ? 24-1 : 48-2; // Максимум файлов в каталоге (последние 16 байт каталога - это код загрузчика)
includeFont = 0;                // Надо поместить шрифт по адресу 0x800

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
ignore["-MAKEROM.JS"] = 1;
ignore["-MAKEROM_OLD.JS"] = 1;
ignore["SPECSVGA.BIN"] = 1;
ignore["SPMX.BIN"] = 1;
ignore["makeRom.vcxproj.user"] = 1;
ignore["makeRom.vcxproj"] = 1;
ignore["makeRom.vcxproj.filters"] = 1;
if (includeFont == 1)
    ignore["FONT.FNT"] = 1;

bootFile = "DOS.SYS";

firstFiles = [];
firstFiles["NC.COM"] = 1;

// Пустая FAT
fat = [];
for (i=0; i<4; i++) fat[i] = 0xFF; // первые 4 байта - команда перехода на загрузчик
for (;i<diskSize; i++) fat[i] = 0;
for (;i<256; i++) fat[i] = 0xFF;

dest = [];
for(i=0; i<diskSize*256; i++) dest += encode[0xFF];

dir         = "";
dosCluster  = 0;
dosSize     = 0;
dosAddr     = 0;
fontCluster = 0;
filesCnt    = 0;
minCluster  = 4;

function allocCluster(cluster)
{
    for(;;)
    {
        for (var i=minCluster; i<diskSize; i++)
        {
            if(fat[i]==0)
            {
                if(cluster) fat[cluster] = i;
                fat[i] = i;
                return i;
            }
        }

        if(minCluster==4)
        {
			shell.Popup("Не хватило места!\n" + filesCnt, 0, "Error", 0)
            throw "Нет места";
        }
        minCluster = 4;
    }
}

function putFile(fileName, boot)
{
    data = loadAll(fileName);
    data_size = data.length;

    // Файлы нулевого объема не поддерживаются
    if(data.length==0) return;

    // Проверяем объем
    if(filesCnt+1==maxFiles)
    {
        shell.Popup("Максимум файлов: " + maxFiles, 0, "Ошибка", 0);
        throw "Максимум файлов "+maxFiles;
    }
    filesCnt++;

    // Получаем адрес загрузки
    startAddr = 0;

    fileName = fileName.toUpperCase();
    ext = fso.GetExtensionName(fileName);
    if (ext == "RKS")
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

    // Сохраняем файл
    cluster = startCluster = 0;
    while (data.length != 0)
    {
        cluster = allocCluster(cluster, minCluster); 
        if(startCluster==0) startCluster = cluster;
        block = data.substr(0,256); data=data.substr(256);
        dest = dest.substr(0,256*cluster) + block + dest.substr(256*cluster+block.length);
    }    

    // Дескриптор файла в каталоге
    if (fat16)
    {
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
        dir += encode[startCluster];                // первый кластер - байт 0
        dir += encode[0];                           // первый кластер - байт 1
        dir += encode[(data_size - 1) & 0xFF];      // размер - байт 0
        dir += encode[(data_size - 1) >> 8];        // размер - байт 1
        dir += encode[0];                           // размер - байт 2
        dir += encode[0];                           // размер - байт 3

        /*shell.Popup(
            fileName + "." + ext + "\n" +
            startAddr + "\n" +
            startCluster + "\n" +
            data_size + "\n\n",
            0, "File", 0);*/
    }
    else
    {
        dir += (fileName+"        ").substr(0,6);   // имя
        dir += (ext+"   ").substr(0,3);             // расширение
        dir += encode[0];                           // attrib
        dir += encode[startAddr & 0xFF];            // адрес загрузки - байт 0
        dir += encode[startAddr >> 8];              // адрес загрузки - байт 1
        dir += encode[(data_size - 1) & 0xFF];      // размер - байт 0
        dir += encode[(data_size - 1) >> 8];        // размер - байт 1
        dir += encode[0];                           // crc?
        dir += encode[startCluster];                // первый кластер
    }

    // Операционная система
    if (boot)
    {
        dosCluster = startCluster;
        dosAddr = startAddr;
        dosSize = (data_size+255)>>8;
    }
}

shell.Run("cmd /c dir /b /on *.* >list.tmp", 2, true);
list = fso.OpenTextFile("list.tmp", 1, false, 0);
boolFileFounded="", filesA=[], filesB=[];
while(!list.AtEndOfStream)
{
    fileName = list.readLine().toUpperCase();
    shortFileName = fso.GetBaseName(fso.GetBaseName(fileName))+"."+fso.GetExtensionName(fileName);
    // Операционная система
    if(boolFileFounded=="" && shortFileName == "DOS.SYS")
        bootFileFounded = fileName;
    else if(firstFiles[shortFileName])
        filesA.push(fileName);
    else if(!ignore[fileName])
        filesB.push(fileName);
}

if (includeFont)
{
  minCluster = 8; // Файл должен начинаться с этого адреса
  putFile("font.fnt");
}

if(bootFileFounded)
  putFile(bootFileFounded, true);
for(i=0; i<filesA.length; i++)
  putFile(filesA[i], false);
for(i=0; i<filesB.length; i++)
  putFile(filesB[i], false);

// Выравнивание каталога

while(dir.length < 3*256) dir += encode[0xFF];

// Код загрузки

if(dosCluster) {  
  // Стандартный загрузчик
  if(filesCnt > 46) throw "Слишком много файлов, некуда поместить загрузчик";
  fat[0] = 0xC3, fat[1] = 0xE1, fat[2] = 0x03;
  dir = dir.substr(0, 16*46);
  dir += encode[0xFF]+encode[0x21]+encode[0x00]+encode[dosCluster]+encode[0x11]+encode[dosAddr&0xFF]+encode[dosAddr>>8]+encode[0xC3];
  dir += encode[0xF1]+encode[0x03]+encode[0xFF]+encode[0xFF]+encode[0xFF]+encode[0xFF]+encode[0xFF]+encode[0xFF];
  dir += encode[0xFF]+encode[0x7E]+encode[0x12]+encode[0x13]+encode[0x23]+encode[0x7A]+encode[0xFE]+encode[(dosAddr>>8) + dosSize];
  dir += encode[0xC2]+encode[0xF1]+encode[0x03]+encode[0xC3]+encode[dosAddr&0xFF]+encode[dosAddr>>8]+encode[0xFF]+encode[0xFF];
} else {
  // Просто подвешиваем компьютер
  fat[0] = 0xC3, fat[1] = 0x00, fat[2] = 0x00;
}

// FAT+Каталог
start = "";
for(i=0; i<256; i++) start += encode[fat[i]];
start += dir;

// Специалист MX
std = start + dest.substr(4*256);

// Специалист MX2
mx2 = encode[0x31] + encode[0xFF] + encode[0xF7] + encode[0xC7]; // lxi sp, 0F7FFh / rst 0
mx2 += dest.substr(32768, 32768-4);
while(mx2.length < 32768) mx2 += encode[0xFF];
mx2 += start + dest.substr(start.length, 32768-start.length);
while(mx2.length < 65536) mx2 += encode[0xFF];

// Сохраняем результат
save("..\\MXOS_MY.bin", mx2);
//save("specsvga.bin", mx2);
//save("spmx.bin", std);
//save("..\\specsvga.bin", mx2);
//save("..\\spmx.rom", std);

// И сразу в эмулятор
save("D:\\Projects\\Specialist\\Emulator\\emu\\Specialist\\MXOS_MY.bin", mx2);
//save("D:\\Projects\\Specialist\\Emulator\\emu80\\specmx\\commander\\MXOS_MY.rom", mx2);
