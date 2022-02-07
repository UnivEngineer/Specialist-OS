//----------------------------------------------------------------------------
// MXOS
// Создание образа диска из отдельных файлов
//
// 2013-11-01 Разработано vinxru
// 2022-01-27 Дополнено SpaceEngineer
//----------------------------------------------------------------------------

fat16       = 1;                // Сформировать ром-диск в FAT16
fatElemSize = fat16 ? 2 : 1;    // Размер элемента fat в байтах
fatSize     = fat16 ? 16: 1;    // Размер fat в кластерах
dirSize     = fat16 ? 4 : 3;    // Размер каталога в кластерах
shipSizeB   = 512*1024;         // Размер ПЗУ в байтах
diskSize    = shipSizeB >> 8;   // Размер ПЗУ в кластерах
maxFiles    = dirSize * 8;      // Максимум файлов в каталоге

// Стандартная ерунда
fso = new ActiveXObject("Scripting.FileSystemObject");
shell = new ActiveXObject("WScript.Shell");
function kill(name) { if(fso.FileExists(name)) fso.DeleteFile(name); }
function fileSize(name) { return fso.GetFile(name).Size; }
function loadAll(name) { return fso.OpenTextFile(name, 1, false, 0).Read(fileSize(name)); } // File.LoadAll глючит 
function save(fileName, data) { fso.CreateTextFile(fileName).Write(data); }
src = loadAll("tbl.bin"); encode = []; decode = []; for(i=0; i<256; i++) { encode[i] = src.charAt(i); decode[src.charCodeAt(i)] = i; }

// Расчет контрольной суммы файла
function specialistSum(data)
{
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
ignore["MAKEROM.JS"] = 1;

// Пустой FAT
fat = [];

// Пустой каталог
dir = [];

// Пустой диск
dest = [];

// Пустой образ ПЗУ
rom = [];

minCluster  = fatSize + dirSize;
numClusters = minCluster;
maxClusters = diskSize;    // Размер страницы ПЗУ в секторах

numPages = 0;
maxPages = 1;   // Размер микросхемы ПЗУ в страницах

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
			shell.Popup("Не хватило места!\n" + filesCnt, 0, "Error", 0)
            throw "Нет места";
        }
        minCluster = fatSize + dirSize;
    }
}

// Создание нового образа микросхемы ПЗУ
function newChip()
{
	//shell.Popup("New chip: " + numChips, 0, "New chip", 0)

	// Пустой образ ПЗУ
	rom = [];
	
	numPages = 0;
}

// Завершение текущего образа микросхемы ПЗУ и сохранение файла
function finishChip()
{
	//shell.Popup("Finish chip: " + numChips, 0, "Finish chip", 0)

	// Сохраняем файл
	save("rom" + numChips + ".bin", rom);

	numChips++;
}

// Создание новой страницы ПЗУ
function newPage()
{
	//shell.Popup("New page: " + numPages, 0, "New page", 0)

	// Пустой FAT
	//fat = [];
	//for (i=0; i<4; i++) fat[i] = 0x76; // Первые 4 байта - код перехода на загрузчик, команда HLT здесь
	//for (;i<maxClusters; i++) fat[i] = 0;
	//for (;i<256; i++) fat[i] = 0xFF;

	// Пустой FAT
    fat = [];
    // Первые 4 байта fat заняты командой перехода на загрузчик.
    // Она нам не нужна, пишем 76h здесь (HLT)
	for (i=0; i<4; i++) fat[i] = 0x76;
    // Остальные кластеры свободны (код 0x0000)
    for (;i<fatElemSize*diskSize; i++) fat[i] = 0x00;

	// Пустой каталог
	dir = [];

	// Пустой диск
	dest = [];
	for(i=0; i<maxClusters*256; i++) dest += encode[0xFF];

	numFiles = 0;
	numClusters = 4;
}

// Завершение текущей страницы ПЗУ
function finishPage()
{
    //shell.Popup("Finish page: " + numPages, 0, "Finish page", 0)

    // Пустой хвост каталога
    while (dir.length < dirSize*256)
        dir += encode[0xFF];

    // FAT + Каталог
    start = "";
    for (i=0; i<fatSize*256; i++) start += encode[fat[i]];
    start += dir;

	// Собираем страницу диска
	page = start + dest.substr(start.length, shipSizeB-start.length);

	// Пустой хвост страницы диска
	while (page.length < shipSizeB) page += encode[0xFF];

	// Добавляем в образ ПЗУ
	rom += page;

	numPages++;
	
	if (numPages == maxPages)
	{
		finishChip();
		newChip();
	}
}

// Добавление файла в структуру диска
function putFile(fileName)
{
	data = loadAll(fileName);
	data_size = data.length;
	data_clusters = (data_size >> 8);

	// Файлы нулевого объема не поддерживаются
	if (data_size == 0) return 1;

	// Проверяем объем диска
	if (numClusters + data_clusters >= maxClusters - minCluster) return 2;

	// Проверяем размер каталога
	if (numFiles >= maxFiles) return 2;

	numClusters += data_clusters;
	numFiles++;

	// Получаем адрес загрузки
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
	cluster = firstFileCluster = 0;
	while (data.length != 0)
	{
		cluster = allocCluster(cluster, minCluster); 
		if (firstFileCluster == 0) firstFileCluster = cluster;
		block = data.substr(0,256);
		data = data.substr(256);
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
        dir += encode[firstFileCluster & 0xFF];     // первый кластер - байт 0
        dir += encode[firstFileCluster >> 8];       // первый кластер - байт 1
        dir += encode[(data_size - 1) & 0xFF];      // размер - байт 0
        dir += encode[(data_size - 1) >> 8];        // размер - байт 1
        dir += encode[0];                           // размер - байт 2
        dir += encode[0];                           // размер - байт 3
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
        dir += encode[0];                           // crc
        dir += encode[firstFileCluster];            // первый кластер
    }

	return 0;
}

//----------------------- Начало -----------------------------


// Получение списка файлов
shell.Run("cmd /c dir /b /on *.* >list.tmp", 2, true);
list = fso.OpenTextFile("list.tmp", 1, false, 0);

// Добавляем короткие имена файлов в список
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

// Создаём нулевую микросхему ПЗУ
newChip();

// Создаём нулевую страницу
newPage();

// Добавляем каждый файл на диск
for (f=0; f<filesA.length; f++)
{
	//shell.Popup("Trying file: " + filesA[f] + "\nNumber " + f, 0, "Adding file", 0)
	if (putFile(filesA[f]) == 2)
	{
		// Если не влезли, завершаем текущую и создаём новую страницу
		finishPage();
		newPage();
		f--;
	}
}

// Завершаем текущую страницу
finishPage();

// Завершаем текущую микросхему ПЗУ
finishChip();
