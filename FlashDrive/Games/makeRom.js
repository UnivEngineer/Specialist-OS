//----------------------------------------------------------------------------
// MXOS
// �������� ������ ����� �� ��������� ������
//
// 2013-11-01 ����������� vinxru
// 2022-01-27 ��������� SpaceEngineer
//----------------------------------------------------------------------------

fat16       = 1;                // ������������ ���-���� � FAT16
fatElemSize = fat16 ? 2 : 1;    // ������ �������� fat � ������
fatSize     = fat16 ? 16: 1;    // ������ fat � ���������
dirSize     = fat16 ? 4 : 3;    // ������ �������� � ���������
shipSizeB   = 512*1024;         // ������ ��� � ������
diskSize    = shipSizeB >> 8;   // ������ ��� � ���������
maxFiles    = dirSize * 8;      // �������� ������ � ��������

// ����������� ������
fso = new ActiveXObject("Scripting.FileSystemObject");
shell = new ActiveXObject("WScript.Shell");
function kill(name) { if(fso.FileExists(name)) fso.DeleteFile(name); }
function fileSize(name) { return fso.GetFile(name).Size; }
function loadAll(name) { return fso.OpenTextFile(name, 1, false, 0).Read(fileSize(name)); } // File.LoadAll ������ 
function save(fileName, data) { fso.CreateTextFile(fileName).Write(data); }
src = loadAll("tbl.bin"); encode = []; decode = []; for(i=0; i<256; i++) { encode[i] = src.charAt(i); decode[src.charCodeAt(i)] = i; }

// ������ ����������� ����� �����
function specialistSum(data)
{
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
ignore["MAKEROM.JS"] = 1;

// ������ FAT
fat = [];

// ������ �������
dir = [];

// ������ ����
dest = [];

// ������ ����� ���
rom = [];

minCluster  = fatSize + dirSize;
numClusters = minCluster;
maxClusters = diskSize;    // ������ �������� ��� � ��������

numPages = 0;
maxPages = 1;   // ������ ���������� ��� � ���������

numChips = 0;

function allocCluster(cluster, minCluster)
{
    for (;;)
    {
        for (var i=minCluster; i<diskSize; i++)
        {
            if (fat16)
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
            else
            {
                if (fat[i] == 0)
                {
                    if (cluster) fat[cluster] = i;
                    fat[i] = i;
                    return i;
                }
            }
        }

        if (minCluster == fatSize + dirSize)
        {
			shell.Popup("�� ������� �����!\n" + filesCnt, 0, "Error", 0)
            throw "��� �����";
        }
        minCluster = fatSize + dirSize;
    }
}

// �������� ������ ������ ���������� ���
function newChip()
{
	//shell.Popup("New chip: " + numChips, 0, "New chip", 0)

	// ������ ����� ���
	rom = [];
	
	numPages = 0;
}

// ���������� �������� ������ ���������� ��� � ���������� �����
function finishChip()
{
	//shell.Popup("Finish chip: " + numChips, 0, "Finish chip", 0)

	// ��������� ����
	save("rom" + numChips + ".bin", rom);

	numChips++;
}

// �������� ����� �������� ���
function newPage()
{
	//shell.Popup("New page: " + numPages, 0, "New page", 0)

	// ������ FAT
	//fat = [];
	//for (i=0; i<4; i++) fat[i] = 0x76; // ������ 4 ����� - ��� �������� �� ���������, ������� HLT �����
	//for (;i<maxClusters; i++) fat[i] = 0;
	//for (;i<256; i++) fat[i] = 0xFF;

	// ������ FAT
    fat = [];
    // ������ 4 ����� fat ������ �������� �������� �� ���������.
    // ��� ��� �� �����, ����� 76h ����� (HLT)
	for (i=0; i<4; i++) fat[i] = 0x76;
    // ��������� �������� �������� (��� 0x0000)
    for (;i<fatElemSize*diskSize; i++) fat[i] = 0x00;

	// ������ �������
	dir = [];

	// ������ ����
	dest = [];
	for(i=0; i<maxClusters*256; i++) dest += encode[0xFF];

	numFiles = 0;
	numClusters = 4;
}

// ���������� ������� �������� ���
function finishPage()
{
    //shell.Popup("Finish page: " + numPages, 0, "Finish page", 0)

    // ������ ����� ��������
    while (dir.length < dirSize*256)
        dir += encode[0xFF];

    // FAT + �������
    start = "";
    for (i=0; i<fatSize*256; i++) start += encode[fat[i]];
    start += dir;

	// �������� �������� �����
	page = start + dest.substr(start.length, shipSizeB-start.length);

	// ������ ����� �������� �����
	while (page.length < shipSizeB) page += encode[0xFF];

	// ��������� � ����� ���
	rom += page;

	numPages++;
	
	if (numPages == maxPages)
	{
		finishChip();
		newChip();
	}
}

// ���������� ����� � ��������� �����
function putFile(fileName)
{
	data = loadAll(fileName);
	data_size = data.length;
	data_clusters = (data_size >> 8);

	// ����� �������� ������ �� ��������������
	if (data_size == 0) return 1;

	// ��������� ����� �����
	if (numClusters + data_clusters >= maxClusters - minCluster) return 2;

	// ��������� ������ ��������
	if (numFiles >= maxFiles) return 2;

	numClusters += data_clusters;
	numFiles++;

	// �������� ����� ��������
	startAddr = 0;
	fileName = fileName.toUpperCase();
	ext = fso.GetExtensionName(fileName);

	//if ((numChips == 2) && (numPages > 0))
	//  shell.Popup("File: " + fileName
    //                + "\nsize = " + data_clusters
    //                + " clusters\nnumFiles = " + numFiles
    //                + "\ntotalClusters = " + numClusters
    //                + "\nnumPages = " + numPages,
    //                0, "Adding file", 0)


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
	cluster = firstFileCluster = 0;
	while (data.length != 0)
	{
		cluster = allocCluster(cluster, minCluster); 
		if (firstFileCluster == 0) firstFileCluster = cluster;
		block = data.substr(0,256);
		data = data.substr(256);
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
        dir += encode[firstFileCluster & 0xFF];     // ������ ������� - ���� 0
        dir += encode[firstFileCluster >> 8];       // ������ ������� - ���� 1
        dir += encode[(data_size - 1) & 0xFF];      // ������ - ���� 0
        dir += encode[(data_size - 1) >> 8];        // ������ - ���� 1
        dir += encode[0];                           // ������ - ���� 2
        dir += encode[0];                           // ������ - ���� 3
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
        dir += encode[0];                           // crc
        dir += encode[firstFileCluster];            // ������ �������
    }

	return 0;
}

//----------------------- ������ -----------------------------


// ��������� ������ ������
shell.Run("cmd /c dir /b /on *.* >list.tmp", 2, true);
list = fso.OpenTextFile("list.tmp", 1, false, 0);

// ��������� �������� ����� ������ � ������
filesA=[];
while (!list.AtEndOfStream)
{
	fileName = list.readLine().toUpperCase();
	ext = fso.GetExtensionName(fileName);
	if (ignore[fileName]) continue;
	if (ext == "BIN") continue;
	fn = fso.GetBaseName(fso.GetBaseName(fileName));
	shortFileName = fn + "." + ext;
	filesA.push(fileName);
}

// ������ ������� ���������� ���
newChip();

// ������ ������� ��������
newPage();

// ��������� ������ ���� �� ����
for (f=0; f<filesA.length; f++)
{
	//shell.Popup("Trying file: " + filesA[f] + "\nNumber " + f, 0, "Adding file", 0)
	if (putFile(filesA[f]) == 2)
	{
		// ���� �� ������, ��������� ������� � ������ ����� ��������
		finishPage();
		newPage();
		f--;
	}
}

// ��������� ������� ��������
finishPage();

// ��������� ������� ���������� ���
finishChip();
