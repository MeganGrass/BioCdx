
class LzoCompression {
private:
public:

	#define EI 9  /* typically 10..13 */
	#define EJ  4  /* typically 4..5 */
	#define P   1  /* If match length <= P then output one character */
	#define Nx (1 << EI)  /* buffer size */
	#define F ((1 << EJ) + 1)  /* lookahead buffer size */

	INT bit_buffer;
	INT bit_mask;
	ULONG codecount;
	ULONG textcount;
	UCHAR buffer[Nx * 2];
	FILE *infile, *outfile;

	ULONG FileSize;

	// Framework
	//System_Common * Common;
	System_File * File;
	//System_Script * Script;

	// Boot
	LzoCompression(VOID);
	~LzoCompression(VOID);

	// Compression
	VOID error(VOID);
	VOID putbit1(VOID);
	VOID putbit0(VOID);
	VOID flush_bit_buffer(VOID);
	VOID output1(INT c);
	VOID output2(INT x, INT y);
	BOOL Compress(CHAR * _Filename, CHAR * Output);

	// Decompression
	VOID Decompress(UCHAR *in, UCHAR *out);

};