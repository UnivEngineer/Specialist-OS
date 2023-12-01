//----------------------------------------------------------------------------
// MXOS
// �������� ������ ����� �� ��������� ������
//
// 2013-11-01 ����������� vinxru
// 2022-02-15 ���������� SpaceEngineer
//----------------------------------------------------------------------------

// �������� ���������
volumeSize    = 64*1024         // ������ ����� ����� � ������
chipSize      = 64*1024         // ������ ����� ���������� ��� � ������
maxFiles      = 64;             // �������� ������ � �������� ��������
volumeLabel   = "ROM DISK 64"   // ����� ����, 11 ��������

// ��������� �� ����������� ���� (���), � ��� �����,
// ������� ��������� ������ ����������� � ��� � ���������
makeBootDisk  = 0;
bootFile      = "";

// ��������� �� ����� �� ������ 0x800 � ��� ����� ������
makeRomFont   = 0;
fontFile      = "";

// �����, ������� ������ ���� � ������ �����
firstFiles = [];

// ������ ������ ���:
// 0 - 32 �� ��� �����������-MX
// 1 - 64 �� ��� �����������-MX2
// 2 - ����-���� (�������� �� ����� �������� chipSize)
romFormat = 2

// ��� ����� ������ ���
romFileName = "FALSH64k.BIN"

// ���� � ������, ���� ��������� ����� ���
destinationPath = "..\\";
emulatorPath    = "..\\..\\..\\..\\Emulator\\emu\\Specialist\\";

//----------------------------------------------------------------------------

// ������ ��������������� ��������
fatElemSize   = 2;                              // ������ �������� fat � ������
sectorSize    = 256;                            // ������ ������� � ������
secPerClus    = 1;                              // ���������� �������� � ��������
volumeSectors = volumeSize / sectorSize;        // ������ ����� ����� � ��������
fatSectors    = volumeSectors * 2 / sectorSize; // ������ FAT � ��������
dirSectors    = maxFiles * 32 / sectorSize;     // ������ ��������� �������� � ��������

fatStartSector  = 1;                                            // ������ ������ ������� FAT
dirStartSector  = fatStartSector + fatSectors;                  // ������ ������ ��������� ��������
dataStartSector = dirStartSector + dirSectors;                  // ������ ������ ������� ������
dataSectors     = volumeSectors - fatSectors - dirSectors - 1;  // ������ ������� ������ � ��������
dataClusters    = dataSectors / secPerClus;                     // ������ ������� ������ � ���������

// ����������� ������
fso = new ActiveXObject("Scripting.FileSystemObject");
shell = new ActiveXObject("WScript.Shell");
function kill(name) { if(fso.FileExists(name)) fso.DeleteFile(name); }
function fileSize(name) { return fso.GetFile(name).Size; }
function loadAll(name) { return fso.OpenTextFile(name, 1, false, 0).Read(fileSize(name)); } // File.LoadAll ������ 
function save(fileName, data) { fso.CreateTextFile(fileName).Write(data); }
src = loadAll("tbl.bin"); encode = []; decode = []; for(i=0; i<256; i++) { encode[i] = src.charAt(i); decode[src.charCodeAt(i)] = i; }

// ������ ����������� ����� �����
function specialistSum(data) {
  s = 0;
  for(i=0; i<data.length-1; i++)
    s += decode[data.charCodeAt(i)] * 257;
  s = (s & 0xFF00) + ((s + decode[data.charCodeAt(i)]) & 0xFF);
  return (s & 0xFFFF);
}

// ������� ��������� �����
kill("list.tmp");

// �����, ������� �� ���� ��������� �� ����
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

// ��������� ����� boot �������
bootSector = "BOOT.BIN";
bootSrc = loadAll("boot.bin");
boot = [];
for (i=0; i<256; i++) boot[i] = decode[bootSrc.charCodeAt(i)];

// ������ FAT
fat = [];
for (i=0; i<4;              i++) fat[i] = 0xFF; // ������ ��� �������� ���������
for (   ; i<fatSectors*256; i++) fat[i] = 0x00; // ��������� �������� ��������

// ������ ������� ������
dest = [];

// ��������� ��������
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

        shell.Popup("�� ������� �����!\n" + numFiles, 0, "Error", 0)
        throw "��� �����";
    }
}

// ���������� �����
function putFile(fileName, isBoot)
{
    data = loadAll(fileName);
    data_size = data.length;

    // ����� �������� ������ �� ��������������
    //if (data_size==0) return;

    // ��������� �����
    if (numFiles+1==maxFiles)
    {
        shell.Popup("�������� ������: " + maxFiles, 0, "������", 0);
        throw "�������� ������ "+maxFiles;
    }
    numFiles++;

    // �������� ����� ��������
    startAddr = 0;

    //fileName = fileName.toUpperCase();
    ext = fso.GetExtensionName(fileName);
    if (ext.toUpperCase() == "RKS")
    {
        // �������� ����� �������� �� ��������� �����
        startAddr = decode[data.charCodeAt(0)] + decode[data.charCodeAt(1)] * 256;
        endAddr   = decode[data.charCodeAt(2)] + decode[data.charCodeAt(3)] * 256;
        len = endAddr - startAddr + 1;
        data = data.substr(4, len);   
    }
    else
    {
        // �������� ����� �������� �� ����� �����
        fileName = fso.GetBaseName(fileName);
        startAddr = fso.GetExtensionName(fileName) * 1;
    }

    // �������� �� ������
    fileName = fso.GetBaseName(fileName);

    //shell.Popup(
    //    fileName + "." + ext + "\n" +
    //    startAddr + "\n" +
    //    data_size + "\n\n",
    //    0, "File", 0);

    // ��������� ����
    cluster = firstCluster = 0;     // ���� �������� ������� ��������� �� ������� 0
    while (data.length != 0)
    {
        cluster = allocCluster(cluster); 
        if (firstCluster == 0) firstCluster = cluster;
        block = data.substr(0, 256);
        data = data.substr(256);
        while (block.length < 256) block += encode[0xFF];
        dest = dest + block;
    }

    // ���������� ����� � ��������
    dir += (fileName+"        ").substr(0,8);   // ���
    dir += (ext+"   ").substr(0,3);             // ����������
    dir += encode[0];                           // attrib
    dir += encode[0];                           // (������ FAT32) ���. � WinNT
    dir += encode[0];                           // (������ FAT32) ����� �������� - ������������
    dir += encode[0] + encode[0];               // (������ FAT32) ����� ��������
    dir += encode[0] + encode[0];               // (������ FAT32) ���� ��������
    dir += encode[startAddr & 0xFF];            // (������ FAT32) ���� ���������; ���������� ��� ������ �������� - ���� 0
    dir += encode[startAddr >> 8];              // (������ FAT32) ���� ���������; ���������� ��� ������ �������� - ���� 1
    dir += encode[0];                           // ������ ������� - ���� 2
    dir += encode[0];                           // ������ ������� - ���� 3
    dir += encode[0] + encode[0];               // ����� ������
    dir += encode[0] + encode[0];               // ���� ������
    dir += encode[firstCluster & 0xFF];         // ������ ������� - ���� 0
    dir += encode[firstCluster >> 8];           // ������ ������� - ���� 1
    dir += encode[(data_size - 1) & 0xFF];      // ������ - ���� 0
    dir += encode[(data_size - 1) >> 8];        // ������ - ���� 1
    dir += encode[0];                           // ������ - ���� 2
    dir += encode[0];                           // ������ - ���� 3

    /*shell.Popup(
        fileName + "." + ext + "\n" +
        startAddr + "\n" +
        firstCluster + "\n" +
        data_size + "\n\n",
        0, "File", 0);*/

    // ������������ �������
    if (isBoot)
    {
        dosCluster = firstCluster;
        dosAddr = startAddr;
        dosSize = (data_size+255)>>8;
    }
}

// ��������� ������ ������
shell.Run("cmd /c dir /b /on *.* >list.tmp", 2, true);
list = fso.OpenTextFile("list.tmp", 1, false, 0);
boolFileFound = "", filesA = [], filesB = [];
while (!list.AtEndOfStream)
{
    fileName = list.readLine();
    fileNameU = fileName.toUpperCase()
    shortFileName = fso.GetBaseName(fso.GetBaseName(fileNameU))+"."+fso.GetExtensionName(fileNameU);

    // ������������ �������
    if (makeBootDisk && boolFileFound == "" && shortFileName == bootFile)
        bootFileFound = fileName;
    else if (firstFiles[shortFileName])
        filesA.push(fileName);
    else if (!ignore[fileNameU])
        filesB.push(fileName);
}

//if (makeRomFont)
//{
//  minCluster = 8; // ���� ������ ���������� � ����� ��������
//  putFile(fontFile);
//}

if (makeBootDisk && bootFileFound)
  putFile(bootFileFound, true);
for (i=0; i<filesA.length; i++)
  putFile(filesA[i], false);
for (i=0; i<filesB.length; i++)
  putFile(filesB[i], false);

// ������ ����� ��������
while (dir.length < dirSectors*256)
    dir += encode[0xFF];

// ������������ ��������� ���� boot �������
boot[0x0B] = sectorSize & 0xFF;     // BPB_BytsPerSec (��. ����)
boot[0x0C] = sectorSize >> 8;       // BPB_BytsPerSec (��. ����)
boot[0x0D] = secPerClus & 0xFF;     // BPB_SecPerClus
boot[0x11] = maxFiles & 0xFF;       // BPB_RootEntCnt (��. ����)
boot[0x12] = maxFiles >> 8;         // BPB_RootEntCnt (��. ����)
boot[0x13] = volumeSectors & 0xFF;  // BPB_TotSec16 (��. ����)
boot[0x14] = volumeSectors >> 8;    // BPB_TotSec16 (��. ����)
boot[0x16] = fatSectors & 0xFF;     // BPB_FATSz16 (��. ����)
boot[0x17] = fatSectors >> 8;       // BPB_FATSz16 (��. ����)
for (i=0; i<11; i++)                // BS_VolLab
    boot[0x2B + i] = decode[volumeLabel.charCodeAt(i)];

// ������������ ��� ����������
if (makeBootDisk)
{
    dosROMAddr = ((dosCluster - 2) * secPerClus + dataStartSector) * sectorSize;
    boot[0x3F] = dosROMAddr & 0xFF;         // ��������� ����� DOS.SYS � ��� (������ ����������)
    boot[0x40] = dosROMAddr >> 8;
    boot[0x42] = dosAddr & 0xFF;            // ��������� ����� DOS.SYS � ������ (���� ����������)
    boot[0x43] = dosAddr >> 8;
    boot[0x4A] = (dosAddr >> 8) + dosSize;  // �������� ����� DOS.SYS � ������ (������� ���� + 1)
}

// �������� ����� ���
start = "";
// Boot ������
for (i=0; i<256; i++) start += encode[boot[i]];
// FAT
for (i=0; i<fatSectors*256; i++) start += encode[fat[i]];
// �������
start += dir;

// ��������� ����� ���
if (romFormat == 0)
{
    // ���������� MX
    rom = start + dest;
    while (rom.length < 65536) rom += encode[0xFF];

    // ��������� ���������
    save(destinationPath + romFileName, rom);
    save(emulatorPath    + romFileName, rom);
}
else if (romFormat == 1)
{
    // ���������� MX2
    // � ������ ��� - ��� �������� �� boot ������ (lxi sp, 0F7FFh / rst 0),
    // ��� 4 ������ ����� ����������� � �������� ���-�����
    rom = encode[0x31] + encode[0xFF] + encode[0xF7] + encode[0xC7];

    // ������ �������� ��� - ��� ������ �������� ������
    rom += dest.substr(32768-start.length, 32768-start.length-4);
    while (rom.length < 32768) rom += encode[0xFF];

    // ������ �������� ��� - ��� ������ �������� ������
    rom += start + dest.substr(0, 32768-start.length);
    while (rom.length < 65536) rom += encode[0xFF];

    // ��������� ���������
    save(destinationPath + romFileName, rom);
    save(emulatorPath    + romFileName, rom);
}
else if (romFormat == 2)
{
    // ���� ����
    rom = start + dest;
    
    // ��������� ��� �����
    romFn  = fso.GetBaseName(romFileName);
    romExt = fso.GetExtensionName(romFileName);

    // �������� �� ����� �������� chipSize
    partNum = 0;
    while (rom.length > 0)
    {
        part = rom.substr(0, chipSize);
        rom = rom.substr(chipSize);
        while (part.length < chipSize) part += encode[0xFF];

        // ��������� ���������
        partFileName = destinationPath + romFn + partNum + "." + romExt;
        save(partFileName, part);
        
        partNum++;
    }
}
