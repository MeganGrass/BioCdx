#pragma once
#include "E:\\Source\\Framework\\Bio2\\global.h"


class CdxFile :
	private System_File {
private:
public:

	// Framework
	System_Configuration_A * Config;
	System_String * String;
	Sony_PlayStation_Disc * Disc;
	LzoCompression *Lzo;

	// Boot
	CdxFile(VOID);
	~CdxFile(VOID);

	// 
	VOID Generic(CONST CHAR * FileList, CONST CHAR * OutName, ...);
	VOID Bss(CONST CHAR * Dir, CHAR * BgmTable);
	VOID BSSEx(CONST CHAR * Dir, CHAR * BgmTable);
	VOID Bgm(CONST CHAR * Dir);
	VOID Do2(CONST CHAR * Dir);
	VOID Pld(CONST CHAR * Dir);
	VOID PldCh(CONST CHAR * Dir);
	VOID Plw(CONST CHAR * Dir, ULONG ID);
	VOID Rdt(CONST CHAR * Dir, ULONG Player);
	VOID SndArms(CONST CHAR * Dir);
	VOID SndCore(CONST CHAR * Dir);

	// 
	VOID DateLog(CONST CHAR * _Filename, FLOAT Version);
	VOID DateLogS(CONST CHAR * _Filename, FLOAT Version);

};