
#include "stdafx.h"
#include FRAMEWORK_H

#include BIO2_LZO_H

#define getbi(_in,_buf,_mask,_x)	{\
	if (_mask == 0)\
		{\
		_buf = *in++;\
		_mask = 0x80;\
		}\
	_x= _buf & _mask ? 1 : 0;\
	_mask >>= 1;\
}\

#define getbit(_in, n, buf, mask, x)	{\
	UINT _n=n;\
	x = 0;\
	for (; _n > 0; _n--)\
	{\
		if (mask == 0)\
		{\
			buf = *_in++;\
			mask = 0x80;\
		}\
		x <<= 1;\
		if (buf & mask) x |= 1;\
		mask >>= 1;\
	}\
}\

// Boot
LzoCompression::LzoCompression(VOID) {
}
LzoCompression::~LzoCompression(VOID) {
}
// Compression
VOID LzoCompression::error(VOID) {
}
VOID LzoCompression::putbit1(VOID) {
    bit_buffer |= bit_mask;
    if ((bit_mask >>= 1) == 0) {
        if (fputc(bit_buffer, outfile) == EOF) error();
        bit_buffer = 0;  bit_mask = 128;  codecount++;
    }
}
VOID LzoCompression::putbit0(VOID) {
    if ((bit_mask >>= 1) == 0) {
        if (fputc(bit_buffer, outfile) == EOF) error();
        bit_buffer = 0;  bit_mask = 128;  codecount++;
    }
}
VOID LzoCompression::flush_bit_buffer(VOID) {
    if (bit_mask != 128) {
        if (fputc(bit_buffer, outfile) == EOF) error();
        codecount++;
    }
}
VOID LzoCompression::output1(INT c) {
    INT mask;
    
    putbit1();
    mask = 256;
    while (mask >>= 1) {
        if (c & mask) putbit1();
        else putbit0();
    }
}
VOID LzoCompression::output2(INT x, INT y) {
    INT mask;
    
    putbit0();
    mask = Nx;
    while (mask >>= 1) {
        if (x & mask) putbit1();
        else putbit0();
    }
    mask = (1 << EJ);
    while (mask >>= 1) {
        if (y & mask) putbit1();
        else putbit0();
    }
}
BOOL LzoCompression::Compress(CHAR * _Filename, CHAR * Output) {

	infile = File->Open(READ_FILE, _Filename);
	if (!infile) { return FALSE; }
	outfile = File->Open(CREATE_FILE, Output);
	if (!outfile) { return FALSE; }

	bit_buffer = 0;
	bit_mask = 128;
	codecount = 0;
	textcount = 0;
	int i, j, f1, x, y, r, s, bufferend, c;

	for(i = 0; i < Nx - F; i++) {buffer[i] = 0x20;}
	for(i = Nx - F; i < Nx * 2; i++) {
		if((c = fgetc(infile)) == EOF) {break;}
		buffer[i] = File->GetU8(c);
		textcount++;
	}
	bufferend = i;
	r = Nx - F;
	s = 0;
    while (r < bufferend) {
        f1 = (F <= bufferend - r) ? F : bufferend - r;
        x = 0;  y = 1;  c = buffer[r];
        for (i = r - 1; i >= s; i--)
            if (buffer[i] == c) {
                for (j = 1; j < f1; j++)
                    if (buffer[i + j] != buffer[r + j]) break;
                if (j > y) {
                    x = i;  y = j;
                }
            }
        if (y <= P) {  y = 1;  output1(c);  }
        else output2(x & (Nx - 1), y - 2);
        r += y;  s += y;
        if (r >= Nx * 2 - F) {
            for (i = 0; i < Nx; i++) buffer[i] = buffer[i + Nx];
            bufferend -= Nx;  r -= Nx;  s -= Nx;
            while (bufferend < Nx * 2) {
                if ((c = fgetc(infile)) == EOF) break;
                buffer[bufferend++] = File->GetU8(c);  textcount++;
            }
        }
    }
    flush_bit_buffer();

	// Complete
	fclose(infile);
	fclose(outfile);

	// 
	ULONG InSize = File->Size(_Filename);
	ULONG OutSize = File->Size(Output);
	UCHAR * _SrcBuf = File->Buffer(Output);
	UCHAR * _DstBuf = new UCHAR [OutSize+4];
	memcpy_s(&_DstBuf[0], 4, &InSize, 4);
	memcpy_s(&_DstBuf[4], OutSize, &_SrcBuf[0], OutSize);
	File->CreateFromSource(&_DstBuf[0], (OutSize+4), Output);
	delete [] _SrcBuf;
	delete [] _DstBuf;

	// Terminate
	return TRUE;
}
// Decompression
VOID LzoCompression::Decompress(UCHAR * in, UCHAR * out) {
	UINT c,
		i,
		j,
		k,
		r,
		dec_size,
		dec_pos,
		buf=0,
		mask;
	UCHAR *decbuffer = (UCHAR*)0x1f800000;

	for (r = 0; r < Nx - F; r++) decbuffer[r] = 0x20;
	r = Nx - F;

	dec_size = *(UINT*)&in[0];
	dec_pos = 0;
	mask = 0;

	in += 4;

	while (dec_pos < dec_size)
	{
		getbi(in, buf, mask, c);
		if (c)
		{
			getbit(in, 8, buf, mask, c);
			out[dec_pos++] = File->GetU8(c);
			decbuffer[r++] = File->GetU8(c);
			r &= (Nx - 1);
		}
		else
		{
			getbit(in, EI, buf, mask, i);
			getbit(in, EJ, buf, mask, j);
			for (k = 0; k <= j + 1; k++)
			{
				c = decbuffer[(i + k) & (Nx - 1)];
				out[dec_pos++] = File->GetU8(c);
				decbuffer[r++] = File->GetU8(c);
				r &= (Nx - 1);
			}
		}
	}
}