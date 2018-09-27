
#include "stdafx.h"
#include FRAMEWORK_H

#include SONY_EXEC_H
#include SONY_DISC_H

#include BIO2_LZO_H

#include CDX_H

// Boot
CdxFile::CdxFile(VOID) {
}
CdxFile::~CdxFile(VOID) {
}
// 
VOID CdxFile::Generic(CONST CHAR * FileList, CONST CHAR * OutName, ...) {

	// OutName
	va_list _ArgList = { NULL };
	__crt_va_start(_ArgList, OutName);
	size_t _BufferCount = (_vscprintf(OutName, _ArgList) + 2);
	CHAR * _FileName = new CHAR[(_BufferCount * 2)];
	RtlSecureZeroMemory(_FileName, (_BufferCount * 2));
	vsprintf_s(_FileName, _BufferCount, OutName, _ArgList);
	__crt_va_end(_ArgList);

	// Open
	Configuration * INI = Config->Open((CHAR*)FileList);
	if (!INI) { return; }
	_iobuf * _File = Open(CREATE_FILE, _FileName);
	if (!_File) { return; }

	// Work
	ULONG Index = 0x00;
	ULONG pData = 0x800;
	Disc->Sector = 1;
	ULONG nNoUse = (Config->SectionLine(INI, (CHAR *)"[CDX]") + 1);
	ULONG LineStart = nNoUse - 1;
	for (ULONG Line = 0; Line < (INI->nLine - nNoUse); Line++, LineStart++, Index += 0x08) {
		if (Exists(Config->Line(INI, (CHAR *)"[CDX]", Line))) {
			ULONG FileSize = Size(Config->Line(INI, (CHAR *)"[CDX]", Line));
			UCHAR * FileBuffer = Buffer(Config->Line(INI, (CHAR *)"[CDX]", Line));
			Write(_File, pData, FileBuffer, FileSize);
			delete[] FileBuffer;
			Write(_File, Index + 4, &Disc->Sector, 0x04);
			Write(_File, Index, &FileSize, 0x04);
			pData += FileSize;
			pData = (pData + 2048 - 1) / 2048 * 2048;
			Disc->Sector += Disc->GetSectorSize(FileSize);
		} else {
			ULONG FileSize = 0;
			Write(_File, Index + 4, &Disc->Sector, 0x04);
			Write(_File, Index, &FileSize, 0x04);
		}
	}

	// Terminate
	fclose(_File);
	Align(2048, _FileName);
	if (INI) { Config->Close(INI); }
}
VOID CdxFile::Bss(CONST CHAR * Dir, CHAR * BgmTable) {

	// Open
	Configuration * INI = Config->Open(BgmTable);
	if (!INI) { Message((CHAR *)"%s\r\n\r\nFile cannot be located, BGM tables will be nullified.", BgmTable); }

	// Archive
	for (ULONG Stage = 1; Stage < 0x08; Stage++)
	{
		// open
		_iobuf * _File = Open(CREATE_FILE, "%s\\BSS%02X.CDX", Dir, Stage);
		if (!_File) { return; }

		ULONG Index = 0x00;
		ULONG pData = 0x800;
		Disc->Sector = 0x01;

		ULONG BgmPtr = 0x180;
		UCHAR BgmMain = NULL;
		UCHAR BgmSub = NULL;

		CHAR * StgStr = String->Get("[STAGE%d]", Stage);

		for (ULONG Room = 0; Room < 0x30; Room++, Index += 0x08, BgmPtr += 0x02)
		{
			if (Exists("%s\\room%d%02X.bss", Dir, Stage, Room)) {
				ULONG FileSize = Size((CHAR *)"%s\\room%d%02X.bss", Dir, Stage, Room);
				UCHAR * FileBuffer = Buffer((CHAR *)"%s\\room%d%02X.bss", Dir, Stage, Room);
				Write(_File, pData, FileBuffer, FileSize);
				delete[] FileBuffer;
				Write(_File, Index + 4, &Disc->Sector, 0x04);
				Write(_File, Index, &FileSize, 0x04);
				pData += FileSize;
				Disc->Sector += Disc->GetSectorSize(FileSize);
			} else {
				ULONG FileSize = 0;
				Write(_File, Index + 4, &Disc->Sector, 0x04);
				Write(_File, Index, &FileSize, 0x04);
			}

			// BGM Table
			if (INI) {
				BgmMain = GetU8(String->ToIntU(Config->Value(INI, StgStr, Room, 0)));
				BgmSub = GetU8(String->ToIntU(Config->Value(INI, StgStr, Room, 1)));
			} else {
				BgmMain = 0xFF;
				BgmSub = 0xFF;
			}
			Write(_File, BgmPtr, &BgmMain, 1);
			Write(_File, BgmPtr + 1, &BgmSub, 1);
		}

		// Complete
		delete[] StgStr;
		fclose(_File);
		Align(2048, "%s\\BSS%02X.CDX", Dir, Stage);
	}

	// Terminate
	if (INI) { Config->Close(INI); }

}
VOID CdxFile::BSSEx(CONST CHAR * Dir, CHAR * BgmTable) {

	// Archive
	for (ULONG n = 1; n < 0x08; n++)
	{
		for (ULONG x = 0; x < 0x30; x++)
		{
			// Open
			_iobuf * _File = Open(CREATE_FILE, "%s\\BssEx\\ROOM%X%02X.BSS", Dir, n, x);
			if (!_File) { return; }

			ULONG _Offset = 0x00;

			// Work
			for (ULONG y = 0; y < 16; y++)
			{
				BOOL UseMask = FALSE;
				if (Exists("%s\\ROOM%X%02X%02d.BS", Dir, n, x, y)) {
					if (Exists("%s\\ROOM%X%02X%02dSpr.TIM", Dir, n, x, y)) {
						CHAR * _Filename = String->Get("%s\\ROOM%X%02X%02dSpr.TIM", Dir, n, x, y);
						CHAR * Output = String->Get("%s\\BssEx\\ROOM%X%02X%02d.LZO", Dir, n, x, y);
						if (Lzo->Compress(_Filename, Output)) { UseMask = TRUE; }
					}

					// 
					ULONG FileSize = Size("%s\\ROOM%X%02X%02d.BS", Dir, n, x, y);
					UCHAR * FileBuffer = Buffer("%s\\ROOM%X%02X%02d.BS", Dir, n, x, y);
					Write(_File, _Offset, FileBuffer, FileSize);
					delete[] FileBuffer;
					_Offset += FileSize;
					Write(_File, _Offset, &FileSize, 0x04);
					if (UseMask) {
						_Offset = (_Offset + 0x8000 - 1) / 0x8000 * 0x8000;
						FileSize = Size("%s\\BssEx\\ROOM%X%02X%02d.LZO", Dir, n, x, y);
						if (FileSize) {
							FileBuffer = Buffer("%s\\BssEx\\ROOM%X%02X%02d.LZO", Dir, n, x, y);
							Write(_File, _Offset, FileBuffer, FileSize);
							delete[] FileBuffer;
							_Offset = (_Offset + 0x10000 - 1) / 0x10000 * 0x10000;
						}
					} else {
						_Offset = (_Offset + 0x10000 - 1) / 0x10000 * 0x10000;
					}
				}
			}

			// Complete
			fclose(_File);
			Align(0x10000, "%s\\BssEx\\ROOM%X%02X.BSS", Dir, n, x);
		}
	}

	// Complete
	Bss(String->Get("%s\\BssEx", Dir), BgmTable);

}
VOID CdxFile::Bgm(CONST CHAR * Dir) {

	// MAIN
	_iobuf * _File = Open(CREATE_FILE, "%s\\SNDMAIN.CDX", Dir);
	if (!_File) { return; }

	ULONG Index = 0x00;
	ULONG pData = 0x800;
	Disc->Sector = 1;

	// archive
	for (ULONG n = 0; n < 0x40; n++, Index += 0x08)
	{
		if (Exists("%s\\main%02X.bgm", Dir, n)) {
			ULONG FileSize = Size("%s\\main%02X.bgm", Dir, n);
			UCHAR * FileBuffer = Buffer("%s\\main%02X.bgm", Dir, n);
			Write(_File, pData, FileBuffer, FileSize);
			delete[] FileBuffer;
			Write(_File, Index + 4, &Disc->Sector, 0x04);
			Write(_File, Index, &FileSize, 0x04);
			pData += FileSize;
			pData = (pData + 2048 - 1) / 2048 * 2048;
			Disc->Sector += Disc->GetSectorSize(FileSize);
			remove(String->Get("%s\\main%02X.bgm", Dir, n));
		}
		else {
			ULONG FileSize = 0;
			Write(_File, Index + 4, &Disc->Sector, 0x04);
			Write(_File, Index, &FileSize, 0x04);
		}
	}

	fclose(_File);
	Align(2048, "%s\\SNDMAIN.CDX", Dir);

	// SUB
	_File = Open(CREATE_FILE, "%s\\SNDSUB.CDX", Dir);
	if (!_File) { return; }

	Index = 0x00;
	pData = 0x800;
	Disc->Sector = 1;

	// archive
	for (ULONG n = 0; n < 0x40; n++, Index += 0x08)
	{
		if (Exists("%s\\sub_%02X.bgm", Dir, n)) {
			ULONG FileSize = Size("%s\\sub_%02X.bgm", Dir, n);
			UCHAR * FileBuffer = Buffer("%s\\sub_%02X.bgm", Dir, n);
			Write(_File, pData, FileBuffer, FileSize);
			delete[] FileBuffer;
			Write(_File, Index + 4, &Disc->Sector, 0x04);
			Write(_File, Index, &FileSize, 0x04);
			pData += FileSize;
			pData = (pData + 2048 - 1) / 2048 * 2048;
			Disc->Sector += Disc->GetSectorSize(FileSize);
			remove(String->Get("%s\\sub_%02X.bgm", Dir, n));
		}
		else {
			ULONG FileSize = 0;
			Write(_File, Index + 4, &Disc->Sector, 0x04);
			Write(_File, Index, &FileSize, 0x04);
		}
	}

	fclose(_File);
	Align(2048, "%s\\SNDSUB.CDX", Dir);

}
VOID CdxFile::Do2(CONST CHAR * Dir) {

	// open
	_iobuf * _File = Open(CREATE_FILE, "%s\\DO2.CDX", Dir);
	if (!_File) { return; }

	ULONG Index = 0x00;
	ULONG pData = 0x800;
	Disc->Sector = 1;

	// CAPCOM
	fpos Tbl;

	// archive
	for (ULONG n = 0; n < 0x40; n++, Index += 0x08)
	{
		if (Exists("%s\\door%02X.do2", Dir, n)) {
			Tbl.size = Size("%s\\door%02X.do2", Dir, n);
			Tbl.sector = GetU16(Disc->Sector);
			Tbl.sum = 0;
			//			Tbl.sum = Bio2->CheckSum(Common->CreateString("%s\\door%02X.do2", Dir, n));
			Disc->Sector += Disc->GetSectorSize(Tbl.size);
			Write(_File, Index, &Tbl, sizeof(tagfpos));

			// HEADER :: RW
			UCHAR * Header = Buffer("%s\\door%02X.do2", Dir, n);
			Write(_File, pData, Header, Tbl.size);
			delete[] Header;
			pData += Tbl.size;
			pData = (pData + 2048 - 1) / 2048 * 2048;
		}
		else {
			Tbl.size = 0;
			Tbl.sum = 0;
			Write(_File, Index, &Tbl.size, 0x04);
			Write(_File, Index + 4, &Disc->Sector, 0x04);
			Write(_File, Index + 7, &Tbl.sum, 0x01);
		}
	}

	fclose(_File);
	Align(4096, "%s\\DO2.CDX", Dir);

}
VOID CdxFile::Pld(CONST CHAR * Dir) {

	// open
	_iobuf * _File = Open(CREATE_FILE, "%s\\PLD.CDX", Dir);
	if (!_File) { return; }

	ULONG Index = 0x00;
	ULONG pData = 0x800;
	Disc->Sector = 1;

	// Playable
	for (ULONG n = 0; n < 0x10; n++, Index += 0x08)
	{
		if (Exists("%s\\pl%02X.pld", Dir, n)) {
			ULONG FileSize = Size("%s\\pl%02X.pld", Dir, n);
			UCHAR * FileBuffer = Buffer("%s\\pl%02X.pld", Dir, n);
			Write(_File, pData, FileBuffer, FileSize);
			delete[] FileBuffer;
			Write(_File, Index + 4, &Disc->Sector, 0x04);
			Write(_File, Index, &FileSize, 0x04);
			pData += FileSize;
			pData = (pData + 2048 - 1) / 2048 * 2048;
			Disc->Sector += Disc->GetSectorSize(FileSize);
		}
		else {
			ULONG FileSize = 0;
			Write(_File, Index + 4, &Disc->Sector, 0x04);
			Write(_File, Index, &FileSize, 0x04);
		}
	}

	// NPC
	for (ULONG n = 0; n < 0x10; n++, Index += 0x08)
	{
		if (Exists("%s\\pl%02Xch.pld", Dir, n)) {
			ULONG FileSize = Size("%s\\pl%02Xch.pld", Dir, n);
			UCHAR * FileBuffer = Buffer("%s\\pl%02Xch.pld", Dir, n);
			Write(_File, pData, FileBuffer, FileSize);
			delete[] FileBuffer;
			Write(_File, Index + 4, &Disc->Sector, 0x04);
			Write(_File, Index, &FileSize, 0x04);
			pData += FileSize;
			pData = (pData + 2048 - 1) / 2048 * 2048;
			Disc->Sector += Disc->GetSectorSize(FileSize);
		}
		else {
			ULONG FileSize = 0;
			Write(_File, Index + 4, &Disc->Sector, 0x04);
			Write(_File, Index, &FileSize, 0x04);
		}
	}


	// Complete
	fclose(_File);
	Align(4096, "%s\\PLD.CDX", Dir);

}
VOID CdxFile::PldCh(CONST CHAR * Dir) {

	// open
	_iobuf * _File = Open(CREATE_FILE, "%s\\PLDCH.CDX", Dir);
	if (!_File) { return; }

	ULONG Index = 0x00;
	ULONG pData = 0x800;
	Disc->Sector = 1;

	// archive
	for (ULONG n = 0; n < 0x10; n++, Index += 0x08)
	{
		if (Exists("%s\\pl%02Xch.pld", Dir, n)) {
			ULONG FileSize = Size("%s\\pl%02Xch.pld", Dir, n);
			UCHAR * FileBuffer = Buffer("%s\\pl%02Xch.pld", Dir, n);
			Write(_File, pData, FileBuffer, FileSize);
			delete[] FileBuffer;
			Write(_File, Index + 4, &Disc->Sector, 0x04);
			Write(_File, Index, &FileSize, 0x04);
			pData += FileSize;
			pData = (pData + 2048 - 1) / 2048 * 2048;
			Disc->Sector += Disc->GetSectorSize(FileSize);
		}
		else {
			ULONG FileSize = 0;
			Write(_File, Index + 4, &Disc->Sector, 0x04);
			Write(_File, Index, &FileSize, 0x04);
		}
	}

	fclose(_File);
	Align(4096, "%s\\PLDCH.CDX", Dir);

}
VOID CdxFile::Plw(CONST CHAR * Dir, ULONG ID) {

	// open
	_iobuf * _File = Open(CREATE_FILE, "%s\\PLW%02X.CDX", Dir, ID);
	if (!_File) { return; }

	ULONG Index = 0x00;
	ULONG pData = 0x800;
	Disc->Sector = 1;

	// archive
	for (ULONG n = 0; n < 0x14; n++, Index += 0x08)
	{
		if (Exists("%s\\pl%02Xw%02X.plw", Dir, ID, n)) {
			ULONG FileSize = Size("%s\\pl%02Xw%02X.plw", Dir, ID, n);
			UCHAR * FileBuffer = Buffer("%s\\pl%02Xw%02X.plw", Dir, ID, n);
			Write(_File, pData, FileBuffer, FileSize);
			delete[] FileBuffer;
			Write(_File, Index + 4, &Disc->Sector, 0x04);
			Write(_File, Index, &FileSize, 0x04);
			pData += FileSize;
			pData = (pData + 2048 - 1) / 2048 * 2048;
			Disc->Sector += Disc->GetSectorSize(FileSize);
		}
		else {
			ULONG FileSize = 0;
			Write(_File, Index + 4, &Disc->Sector, 0x04);
			Write(_File, Index, &FileSize, 0x04);
		}
	}

	Write(_File, Index, &ID, 0x04);

	// Close
	fclose(_File);
	Align(4096, "%s\\PLW%02X.CDX", Dir, ID);

}
VOID CdxFile::Rdt(CONST CHAR * Dir, ULONG Player) {

	// archive
	for (ULONG Stage = 1; Stage < 0x08; Stage++)
	{
		// Open
		_iobuf * _File = Open(CREATE_FILE, "%s\\STAGE%02X%02X.CDX", Dir, Player, Stage);
		if (!_File) { return; }

		ULONG Index = 0x00;
		ULONG pData = 0x800;
		Disc->Sector = 1;

		for (ULONG Room = 0; Room < 0x30; Room++, Index += 0x08)
		{
			if (Exists("%s\\ROOM%X%02X%X.RDT", Dir, Stage, Room, Player)) {
				ULONG FileSize = Size("%s\\ROOM%X%02X%X.RDT", Dir, Stage, Room, Player);
				UCHAR * FileBuffer = Buffer("%s\\ROOM%X%02X%X.RDT", Dir, Stage, Room, Player);
				Write(_File, pData, FileBuffer, FileSize);
				delete[] FileBuffer;
				Write(_File, Index + 4, &Disc->Sector, 0x04);
				Write(_File, Index, &FileSize, 0x04);
				pData += FileSize;
				pData = (pData + 2048 - 1) / 2048 * 2048;
				Disc->Sector += Disc->GetSectorSize(FileSize);
			}
			else {
				ULONG FileSize = 0;
				Write(_File, Index + 4, &Disc->Sector, 0x04);
				Write(_File, Index, &FileSize, 0x04);
			}
		}

		// Close
		fclose(_File);
		Align(4096, "%s\\STAGE%02X%02X.CDX", Dir, Player, Stage);
	}

}
VOID CdxFile::SndArms(CONST CHAR * Dir) {

	// Work
	ULONG Index = 0x00;
	ULONG pData = 0x800;
	Disc->Sector = 1;

	// Open
	_iobuf * _File = Open(CREATE_FILE, "%s\\SNDARMS.CDX", Dir);
	if (!_File) { return; }

	// Header
	for (ULONG n = 0; n < 0x14; n++, Index += 0x08)
	{
		if (Exists("%s\\ARMS%02X.EDH", Dir, n)) {
			ULONG FileSize = Size("%s\\ARMS%02X.EDH", Dir, n);
			UCHAR * FileBuffer = Buffer("%s\\ARMS%02X.EDH", Dir, n);
			Write(_File, pData, FileBuffer, FileSize);
			delete[] FileBuffer;
			Write(_File, Index, &FileSize, 0x04);
			Write(_File, Index + 4, &Disc->Sector, 0x04);
			pData += FileSize;
			pData = (pData + 2048 - 1) / 2048 * 2048;
			Disc->Sector += Disc->GetSectorSize(FileSize);
		}
		else {
			ULONG FileSize = 0;
			Write(_File, Index, &FileSize, 0x04);
			Write(_File, Index + 4, &Disc->Sector, 0x04);
		}
	}

	// Bank
	for (ULONG n = 0; n < 0x14; n++, Index += 0x08)
	{
		if (Exists("%s\\ARMS%02X.VB", Dir, n)) {
			ULONG FileSize = Size("%s\\ARMS%02X.VB", Dir, n);
			UCHAR * FileBuffer = Buffer("%s\\ARMS%02X.VB", Dir, n);
			Write(_File, pData, FileBuffer, FileSize);
			delete[] FileBuffer;
			Write(_File, Index, &FileSize, 0x04);
			Write(_File, Index + 4, &Disc->Sector, 0x04);
			pData += FileSize;
			pData = (pData + 2048 - 1) / 2048 * 2048;
			Disc->Sector += Disc->GetSectorSize(FileSize);
		}
		else {
			ULONG FileSize = 0;
			Write(_File, Index, &FileSize, 0x04);
			Write(_File, Index + 4, &Disc->Sector, 0x04);
		}
	}

	// Close
	fclose(_File);
	Align(2048, "%s\\SNDARMS.CDX", Dir);
}
VOID CdxFile::SndCore(CONST CHAR * Dir) {

	// Work
	ULONG Index = 0x00;
	ULONG pData = 0x800;
	Disc->Sector = 1;

	// Open
	_iobuf * _File = Open(CREATE_FILE, "%s\\SNDCORE.CDX", Dir);
	if (!_File) { return; }

	// Header
	for (ULONG n = 0; n < 0x16; n++, Index += 0x08)
	{
		if (Exists("%s\\CORE%02X.EDH", Dir, n)) {
			ULONG FileSize = Size("%s\\CORE%02X.EDH", Dir, n);
			UCHAR * FileBuffer = Buffer("%s\\CORE%02X.EDH", Dir, n);
			Write(_File, pData, FileBuffer, FileSize);
			delete[] FileBuffer;
			Write(_File, Index, &FileSize, 0x04);
			Write(_File, Index + 4, &Disc->Sector, 0x04);
			pData += FileSize;
			pData = (pData + 2048 - 1) / 2048 * 2048;
			Disc->Sector += Disc->GetSectorSize(FileSize);
		}
		else {
			ULONG FileSize = 0;
			Write(_File, Index, &FileSize, 0x04);
			Write(_File, Index + 4, &Disc->Sector, 0x04);
		}
	}

	// Bank
	for (ULONG n = 0; n < 0x16; n++, Index += 0x08)
	{
		if (Exists("%s\\CORE%02X.VB", Dir, n)) {
			ULONG FileSize = Size("%s\\CORE%02X.VB", Dir, n);
			UCHAR * FileBuffer = Buffer("%s\\CORE%02X.VB", Dir, n);
			Write(_File, pData, FileBuffer, FileSize);
			delete[] FileBuffer;
			Write(_File, Index, &FileSize, 0x04);
			Write(_File, Index + 4, &Disc->Sector, 0x04);
			pData += FileSize;
			pData = (pData + 2048 - 1) / 2048 * 2048;
			Disc->Sector += Disc->GetSectorSize(FileSize);
		}
		else {
			ULONG FileSize = 0;
			Write(_File, Index, &FileSize, 0x04);
			Write(_File, Index + 4, &Disc->Sector, 0x04);
		}
	}

	// Close
	fclose(_File);
	Align(2048, "%s\\SNDCORE.CDX", Dir);
}
// 
VOID CdxFile::DateLog(CONST CHAR * _Filename, FLOAT Version) {

	// get
	SYSTEMTIME SYS_TIME;
	GetSystemTime(&SYS_TIME);
	SYS_TIME.wHour -= 5;

	// Open
	_iobuf * _File = Open(CREATE_FILE, _Filename);
	if (!_File) { return; }

	// Print
	Print(_File, "v%.1f.%04d.%02d.%02d", Version, SYS_TIME.wYear, SYS_TIME.wMonth, SYS_TIME.wDay);

	// Terminate
	fclose(_File);

}
VOID CdxFile::DateLogS(CONST CHAR * _Filename, FLOAT Version) {

	// get
	SYSTEMTIME SYS_TIME;
	GetSystemTime(&SYS_TIME);
	SYS_TIME.wHour -= 5;

	// Open
	_iobuf * _File = Open(CREATE_FILE, _Filename);
	if (!_File) { return; }

	// Print
	Print(_File, "\r\n\t.rdata\r\n\t.globl\tEngineVer\r\nEngineVer:\r\n\t.ascii \"v%.1f.%04d.%02d.%02d\"\r\n\t.byte 0\r\n", Version, SYS_TIME.wYear, SYS_TIME.wMonth, SYS_TIME.wDay);

	// Terminate
	fclose(_File);

}