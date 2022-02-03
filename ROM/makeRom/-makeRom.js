//----------------------------------------------------------------------------
// RAMFOS
// �������� ������ ����� �� ��������� ������
//
// 2013-11-01 ����������� vinxru
//----------------------------------------------------------------------------

fat16   = 1;                    // ������������ ���-���� � FAT16
fatSize = 1;                    // ������ FAT � ���������
dirSize = 3;                    // ������ �������� � ���������
diskSize = (65536 >> 8);        // ������ ��� � ���������
maxFiles = fat16 ? 24-1 : 48-2; // �������� ������ � �������� (��������� 16 ���� �������� - ��� ��� ����������)
includeFont = 0;                // ���� ��������� ����� �� ������ 0x800

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

// ������ FAT
fat = [];
for (i=0; i<4; i++) fat[i] = 0xFF; // ������ 4 ����� - ������� �������� �� ���������
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
			shell.Popup("�� ������� �����!\n" + filesCnt, 0, "Error", 0)
            throw "��� �����";
        }
        minCluster = 4;
    }
}

function putFile(fileName, boot)
{
    data = loadAll(fileName);
    data_size = data.length;

    // ����� �������� ������ �� ��������������
    if(data.length==0) return;

    // ��������� �����
    if(filesCnt+1==maxFiles)
    {
        shell.Popup("�������� ������: " + maxFiles, 0, "������", 0);
        throw "�������� ������ "+maxFiles;
    }
    filesCnt++;

    // �������� ����� ��������
    startAddr = 0;

    fileName = fileName.toUpperCase();
    ext = fso.GetExtensionName(fileName);
    if (ext == "RKS")
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

    // ��������� ����
    cluster = startCluster = 0;
    while (data.length != 0)
    {
        cluster = allocCluster(cluster, minCluster); 
        if(startCluster==0) startCluster = cluster;
        block = data.substr(0,256); data=data.substr(256);
        dest = dest.substr(0,256*cluster) + block + dest.substr(256*cluster+block.length);
    }    

    // ���������� ����� � ��������
    if (fat16)
    {
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
        dir += encode[startCluster];                // ������ ������� - ���� 0
        dir += encode[0];                           // ������ ������� - ���� 1
        dir += encode[(data_size - 1) & 0xFF];      // ������ - ���� 0
        dir += encode[(data_size - 1) >> 8];        // ������ - ���� 1
        dir += encode[0];                           // ������ - ���� 2
        dir += encode[0];                           // ������ - ���� 3

        /*shell.Popup(
            fileName + "." + ext + "\n" +
            startAddr + "\n" +
            startCluster + "\n" +
            data_size + "\n\n",
            0, "File", 0);*/
    }
    else
    {
        dir += (fileName+"        ").substr(0,6);   // ���
        dir += (ext+"   ").substr(0,3);             // ����������
        dir += encode[0];                           // attrib
        dir += encode[startAddr & 0xFF];            // ����� �������� - ���� 0
        dir += encode[startAddr >> 8];              // ����� �������� - ���� 1
        dir += encode[(data_size - 1) & 0xFF];      // ������ - ���� 0
        dir += encode[(data_size - 1) >> 8];        // ������ - ���� 1
        dir += encode[0];                           // crc?
        dir += encode[startCluster];                // ������ �������
    }

    // ������������ �������
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
    // ������������ �������
    if(boolFileFounded=="" && shortFileName == "DOS.SYS")
        bootFileFounded = fileName;
    else if(firstFiles[shortFileName])
        filesA.push(fileName);
    else if(!ignore[fileName])
        filesB.push(fileName);
}

if (includeFont)
{
  minCluster = 8; // ���� ������ ���������� � ����� ������
  putFile("font.fnt");
}

if(bootFileFounded)
  putFile(bootFileFounded, true);
for(i=0; i<filesA.length; i++)
  putFile(filesA[i], false);
for(i=0; i<filesB.length; i++)
  putFile(filesB[i], false);

// ������������ ��������

while(dir.length < 3*256) dir += encode[0xFF];

// ��� ��������

if(dosCluster) {  
  // ����������� ���������
  if(filesCnt > 46) throw "������� ����� ������, ������ ��������� ���������";
  fat[0] = 0xC3, fat[1] = 0xE1, fat[2] = 0x03;
  dir = dir.substr(0, 16*46);
  dir += encode[0xFF]+encode[0x21]+encode[0x00]+encode[dosCluster]+encode[0x11]+encode[dosAddr&0xFF]+encode[dosAddr>>8]+encode[0xC3];
  dir += encode[0xF1]+encode[0x03]+encode[0xFF]+encode[0xFF]+encode[0xFF]+encode[0xFF]+encode[0xFF]+encode[0xFF];
  dir += encode[0xFF]+encode[0x7E]+encode[0x12]+encode[0x13]+encode[0x23]+encode[0x7A]+encode[0xFE]+encode[(dosAddr>>8) + dosSize];
  dir += encode[0xC2]+encode[0xF1]+encode[0x03]+encode[0xC3]+encode[dosAddr&0xFF]+encode[dosAddr>>8]+encode[0xFF]+encode[0xFF];
} else {
  // ������ ����������� ���������
  fat[0] = 0xC3, fat[1] = 0x00, fat[2] = 0x00;
}

// FAT+�������
start = "";
for(i=0; i<256; i++) start += encode[fat[i]];
start += dir;

// ���������� MX
std = start + dest.substr(4*256);

// ���������� MX2
mx2 = encode[0x31] + encode[0xFF] + encode[0xF7] + encode[0xC7]; // lxi sp, 0F7FFh / rst 0
mx2 += dest.substr(32768, 32768-4);
while(mx2.length < 32768) mx2 += encode[0xFF];
mx2 += start + dest.substr(start.length, 32768-start.length);
while(mx2.length < 65536) mx2 += encode[0xFF];

// ��������� ���������
save("..\\MXOS_MY.bin", mx2);
//save("specsvga.bin", mx2);
//save("spmx.bin", std);
//save("..\\specsvga.bin", mx2);
//save("..\\spmx.rom", std);

// � ����� � ��������
save("D:\\Projects\\Specialist\\Emulator\\emu\\Specialist\\MXOS_MY.bin", mx2);
//save("D:\\Projects\\Specialist\\Emulator\\emu80\\specmx\\commander\\MXOS_MY.rom", mx2);
