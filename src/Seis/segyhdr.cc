/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : May 1995
 * FUNCTION : Seg-Y headers
-*/

static const char* rcsID = "$Id: segyhdr.cc,v 1.19 2004-04-26 15:50:56 bert Exp $";


#include "segyhdr.h"
#include "fixstring.h"
#include "string2.h"
#include "survinfo.h"
#include "ibmformat.h"
#include "settings.h"
#include "seisinfo.h"
#include <string.h>
#include <ctype.h>
#include <iostream>
#include <stdio.h>
#include <math.h>

const char* SegyTraceheaderDef::sXCoordByte = "X-coord byte";
const char* SegyTraceheaderDef::sYCoordByte = "Y-coord byte";
const char* SegyTraceheaderDef::sInlByte = "In-line byte";
const char* SegyTraceheaderDef::sCrlByte = "Cross-line byte";
const char* SegyTraceheaderDef::sPickByte = "Pick byte";
const char* SegyTraceheaderDef::sInlByteSz = "Nr bytes for In-line";
const char* SegyTraceheaderDef::sCrlByteSz = "Nr bytes for Cross-line";


static void Ebcdic2Ascii(unsigned char*,int);
static void Ascii2Ebcdic(unsigned char*,int);


SegyTxtHeader::SegyTxtHeader()
{
    char cbuf[3];
    int nrlines = SegyTxtHeaderLength / 80;

    memset( txt, ' ', SegyTxtHeaderLength );
    for ( int i=0; i<nrlines; i++ )
    {
	int i80 = i*80; txt[i80] = 'C';
	sprintf( cbuf, "%02d", i+1 );
	txt[i80+1] = cbuf[0]; txt[i80+2] = cbuf[1];
    }

    FileNameString buf;
    buf = "Created by: ";
    buf += Settings::common()[ "Company" ];
    putAt( 1, 6, 75, buf );
    putAt( 2, 6, 75, SI().name() );
    BinID bid = SI().range(false).start;
    Coord coord = SI().transform( bid );
    coord.x = fabs(coord.x); coord.y = fabs(coord.y);
    if ( !mIS_ZERO(bid.inl-coord.x) && !mIS_ZERO(bid.crl-coord.x)
      && !mIS_ZERO(bid.inl-coord.y) && !mIS_ZERO(bid.crl-coord.y) )
    {
	char* pbuf = buf.buf();
	coord = SI().transform( bid );
	bid.fill( pbuf ); buf += " = "; coord.fill( pbuf + strlen(buf) );
	putAt( 12, 6, 75, buf );
	bid.crl = SI().range(false).stop.crl;
	coord = SI().transform( bid );
	bid.fill( pbuf ); buf += " = "; coord.fill( pbuf + strlen(buf) );
	putAt( 13, 6, 75, buf );
	bid.inl = SI().range(false).stop.inl;
	bid.crl = SI().range(false).start.crl;
	coord = SI().transform( bid );
	bid.fill( pbuf ); buf += " = "; coord.fill( pbuf + strlen(buf) );
	putAt( 14, 6, 75, buf );
    }

    if ( !SI().zIsTime() )
    {
	buf = "Depth survey: 1 SEG-Y millisec = 1 ";
	buf += SI().getZUnit(false);
	putAt( 18, 6, 75, buf.buf() );
    }

    putAt( 38, 6, 75, "Geodetic datum: No data available" );
}


void SegyTxtHeader::setAscii()
{ if ( txt[0] != 'C' ) Ebcdic2Ascii( txt, SegyTxtHeaderLength ); }
void SegyTxtHeader::setEbcdic()
{ if ( txt[0] == 'C' ) Ascii2Ebcdic( txt, SegyTxtHeaderLength ); }


void SegyTxtHeader::setUserInfo( const char* txt )
{
    if ( !txt || !*txt ) return;

    FileNameString buf;
    int lnr = 16;
    while ( lnr < 35 )
    {
	char* ptr = buf.buf();
	int idx = 0;
	while ( *txt && *txt != '\n' && ++idx < 75 )
	    *ptr++ = *txt++;
	*ptr = '\0';
	putAt( lnr, 5, 80, buf );

	if ( !*txt ) break;
	lnr++;
	txt++;
    }
}


void SegyTxtHeader::setPosInfo( int xcoordbyte, int ycoordbyte,
				int inlbyte, int crlbyte )
{
    BufferString buf;
    buf = "Byte positions:";
    putAt( 5, 6, 6+buf.size(), buf );
    buf = "X-coordinate: "; buf += xcoordbyte;
    putAt( 6, 6, 6+buf.size(), buf );
    buf = "Y-coordinate: "; buf += ycoordbyte;
    putAt( 7, 6, 6+buf.size(), buf );
    buf = "In-line:      "; if ( inlbyte >= 0 ) buf += inlbyte;
    putAt( 8, 6, 6+buf.size(), buf );
    buf = "X-line:       "; if ( crlbyte >= 0 ) buf += crlbyte;
    putAt( 9, 6, 6+buf.size(), buf );
    if ( inlbyte < 0 || crlbyte < 0 )
	buf = "No I/X info available";
    else
    {
	buf = "I/X bytes: "; buf += inlbyte;
	buf += " / "; buf += crlbyte;
    }
    putAt( 36, 6, 75, buf );
}


void SegyTxtHeader::setStartPos( float sp )
{
    BufferString buf;
    if ( !mIS_ZERO(sp) )
    {
	buf = "First sample ";
	buf += SI().zIsTime() ? "time " : "depth ";
	buf += SI().getZUnit(); buf += ": ";
	buf += sp * SI().zFactor();
    }
    putAt( 37, 6, 75, buf );
}


void SegyTxtHeader::getText( BufferString& bs )
{
    char buf[80];
    bs = "";
    for ( int idx=0; idx<40; idx++ )
    {
	getFrom( idx, 3, 80, buf );
	if ( !buf[0] ) continue;
	if ( *(const char*)bs ) bs += "\n";
	bs += buf;
    }
}


void SegyTxtHeader::getFrom( int line, int pos, int endpos, char* str ) const
{
    if ( !str ) return;

    int charnr = (line-1)*80 + pos - 1;
    if ( endpos > 80 ) endpos = 80;
    int maxcharnr = (line-1)*80 + endpos;

    while ( isspace(txt[charnr]) && charnr < maxcharnr ) charnr++;
    while ( charnr < maxcharnr ) *str++ = txt[charnr++];
    *str = '\0';
    removeTrailingBlanks( str );
}


void SegyTxtHeader::putAt( int line, int pos, int endpos, const char* str )
{
    if ( !str || !*str ) return;

    int charnr = (line-1)*80 + pos - 1;
    if ( endpos > 80 ) endpos = 80;
    int maxcharnr = (line-1)*80 + endpos;

    while ( charnr < maxcharnr && *str )
	txt[charnr++] = *str++;
}


void SegyTxtHeader::print( ostream& stream ) const
{
    char buf[81];
    buf[80] = '\0';

    for ( int line=0; line<40; line++ )
    {
	memcpy( buf, &txt[80*line], 80 );
	stream << buf << endl;
    }
}


SegyBinHeader::SegyBinHeader()
	: needswap(false)
{
    memset( &jobid, 0, SegyBinHeaderLength );
    mfeet = format = 1;
    float fhdt = SeisTrcInfo::defaultSampleInterval() * 1000;
    if ( SI().zIsTime() && fhdt < 32.768 )
	fhdt *= 1000;
    hdt = (short)fhdt;
}


#define mSBHGet(memb,typ,nb) \
    if ( needswap ) swap_bytes( b, nb ); \
    memb = IbmFormat::as##typ( b ); \
    if ( needswap ) swap_bytes( b, nb ); \
    b += nb;

void SegyBinHeader::getFrom( const void* buf )
{
    unsigned char* b = (unsigned char*)buf;

    mSBHGet(jobid,Int,4);
    mSBHGet(lino,Int,4);
    mSBHGet(reno,Int,4);
    mSBHGet(ntrpr,Short,2);
    mSBHGet(nart,Short,2);
    mSBHGet(hdt,Short,2);
    mSBHGet(dto,Short,2);
    mSBHGet(hns,Short,2);
    mSBHGet(nso,Short,2);
    mSBHGet(format,Short,2);
    mSBHGet(fold,Short,2);
    mSBHGet(tsort,Short,2);
    mSBHGet(vscode,Short,2);
    mSBHGet(hsfs,Short,2);
    mSBHGet(hsfe,Short,2);
    mSBHGet(hslen,Short,2);
    mSBHGet(hstyp,Short,2);
    mSBHGet(schn,Short,2);
    mSBHGet(hstas,Short,2);
    mSBHGet(hstae,Short,2);
    mSBHGet(htatyp,Short,2);
    mSBHGet(hcorr,Short,2);
    mSBHGet(bgrcv,Short,2);
    mSBHGet(rcvm,Short,2);
    mSBHGet(mfeet,Short,2);
    mSBHGet(polyt,Short,2);
    mSBHGet(vpol,Short,2);

    for ( int i=0; i<SegyBinHeaderUnassShorts; i++ )
    {
	mSBHGet(hunass[i],Short,2);
    }
}


#define mSBHPut(memb,typ,nb) \
    IbmFormat::put##typ( memb, b ); \
    if ( needswap ) swap_bytes( b, nb ); \
    b += nb;


void SegyBinHeader::putTo( void* buf ) const
{
    unsigned char* b = (unsigned char*)buf;

    mSBHPut(jobid,Int,4);
    mSBHPut(lino,Int,4);
    mSBHPut(reno,Int,4);
    mSBHPut(ntrpr,Short,2);
    mSBHPut(nart,Short,2);
    mSBHPut(hdt,Short,2);
    mSBHPut(dto,Short,2);
    mSBHPut(hns,Short,2);
    mSBHPut(nso,Short,2);
    mSBHPut(format,Short,2);
    mSBHPut(fold,Short,2);
    mSBHPut(tsort,Short,2);
    mSBHPut(vscode,Short,2);
    mSBHPut(hsfs,Short,2);
    mSBHPut(hsfe,Short,2);
    mSBHPut(hslen,Short,2);
    mSBHPut(hstyp,Short,2);
    mSBHPut(schn,Short,2);
    mSBHPut(hstas,Short,2);
    mSBHPut(hstae,Short,2);
    mSBHPut(htatyp,Short,2);
    mSBHPut(hcorr,Short,2);
    mSBHPut(bgrcv,Short,2);
    mSBHPut(rcvm,Short,2);
    mSBHPut(mfeet,Short,2);
    mSBHPut(polyt,Short,2);
    mSBHPut(vpol,Short,2);

    for ( int i=0; i<SegyBinHeaderUnassShorts; i++ )
	{ mSBHPut(hunass[i],Short,2); }
}


static void Ebcdic2Ascii( unsigned char *chbuf, int len )
{
    int i;
    static unsigned char e2a[256] = {
0x00,0x01,0x02,0x03,0x9C,0x09,0x86,0x7F,0x97,0x8D,0x8E,0x0B,0x0C,0x0D,0x0E,0x0F,
0x10,0x11,0x12,0x13,0x9D,0x85,0x08,0x87,0x18,0x19,0x92,0x8F,0x1C,0x1D,0x1E,0x1F,
0x80,0x81,0x82,0x83,0x84,0x0A,0x17,0x1B,0x88,0x89,0x8A,0x8B,0x8C,0x05,0x06,0x07,
0x90,0x91,0x16,0x93,0x94,0x95,0x96,0x04,0x98,0x99,0x9A,0x9B,0x14,0x15,0x9E,0x1A,
0x20,0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0x5B,0x2E,0x3C,0x28,0x2B,0x21,
0x26,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,0xB0,0xB1,0x5D,0x24,0x2A,0x29,0x3B,0x5E,
0x2D,0x2F,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0x7C,0x2C,0x25,0x5F,0x3E,0x3F,
0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,0xC0,0xC1,0xC2,0x60,0x3A,0x23,0x40,0x27,0x3D,0x22,
0xC3,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,
0xCA,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,
0xD1,0x7E,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,
0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,
0x7B,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,
0x7D,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50,0x51,0x52,0xEE,0xEF,0xF0,0xF1,0xF2,0xF3,
0x5C,0x9F,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,
0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF
    };

    for ( i=0; i<len; i++ ) chbuf[i] = e2a[chbuf[i]];
}


static void Ascii2Ebcdic( unsigned char *chbuf, int len )
{
    int i;
    static unsigned char a2e[256] = {
0x00,0x01,0x02,0x03,0x37,0x2D,0x2E,0x2F,0x16,0x05,0x25,0x0B,0x0C,0x0D,0x0E,0x0F,
0x10,0x11,0x12,0x13,0x3C,0x3D,0x32,0x26,0x18,0x19,0x3F,0x27,0x1C,0x1D,0x1E,0x1F,
0x40,0x4F,0x7F,0x7B,0x5B,0x6C,0x50,0x7D,0x4D,0x5D,0x5C,0x4E,0x6B,0x60,0x4B,0x61,
0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0x7A,0x5E,0x4C,0x7E,0x6E,0x6F,
0x7C,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,
0xD7,0xD8,0xD9,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0x4A,0xE0,0x5A,0x5F,0x6D,
0x79,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x91,0x92,0x93,0x94,0x95,0x96,
0x97,0x98,0x99,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xC0,0x6A,0xD0,0xA1,0x07,
0x20,0x21,0x22,0x23,0x24,0x15,0x06,0x17,0x28,0x29,0x2A,0x2B,0x2C,0x09,0x0A,0x1B,
0x30,0x31,0x1A,0x33,0x34,0x35,0x36,0x08,0x38,0x39,0x3A,0x3B,0x04,0x14,0x3E,0xE1,
0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x51,0x52,0x53,0x54,0x55,0x56,0x57,
0x58,0x59,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x70,0x71,0x72,0x73,0x74,0x75,
0x76,0x77,0x78,0x80,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,0x90,0x9A,0x9B,0x9C,0x9D,0x9E,
0x9F,0xA0,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,
0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xDA,0xDB,
0xDC,0xDD,0xDE,0xDF,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF
    };

    for ( i=0; i<len; i++ ) chbuf[i] = a2e[chbuf[i]];
}

#define prmem(mem) if (mem) { stream << #mem << "\t= " << mem << endl; }

void SegyBinHeader::print( ostream& stream ) const
{
    prmem(jobid); prmem(lino); prmem(reno); prmem(ntrpr);
    prmem(nart); prmem(hdt); prmem(dto); prmem(hns); prmem(nso);
    prmem(format); prmem(fold); prmem(tsort); prmem(vscode); prmem(hsfs);
    prmem(hsfe); prmem(hslen); prmem(hstyp); prmem(schn); prmem(hstas);
    prmem(hstae); prmem(htatyp); prmem(hcorr); prmem(bgrcv); prmem(rcvm);
    prmem(mfeet); prmem(polyt); prmem(vpol);
    for ( int i=0; i<SegyBinHeaderUnassShorts; i++ )
	if ( hunass[i] != 0 )
	    stream << "hunass[" << i << "] = " << hunass[i] << endl;
}


#define mPrHead(mem,byt,fun) \
strm << '\t' << #mem << '\t' << byt+1 \
     << '\t' << IbmFormat::as##fun( buf+byt ) << '\n'

void SegyTraceheader::print( ostream& strm ) const
{
    mPrHead(tracl,0,Int); mPrHead(tracr,4,Int);
    mPrHead(fldr,8,Int); mPrHead(tracf,12,Int);
    mPrHead(ep,16,Int); mPrHead(cdp,20,Int);
    mPrHead(cdpt,24,Int); mPrHead(trid,28,Short); mPrHead(nvs,30,Short);
    mPrHead(nhs,32,Short); mPrHead(duse,34,Short); mPrHead(offs,36,Int);
    mPrHead(gelev,40,Int); mPrHead(selev,44,Int); mPrHead(sdepth,48,Int);
    mPrHead(gdel,52,Int); mPrHead(sdel,56,Int); mPrHead(swdep,60,Int);
    mPrHead(gwdep,64,Int); mPrHead(scalel,68,Short); mPrHead(scalco,70,Short);
    mPrHead(sx,72,Int); mPrHead(sy,76,Int); mPrHead(gx,80,Int);
    mPrHead(gy,84,Int); mPrHead(counit,88,Short); mPrHead(wevel,90,Short);
    mPrHead(swevel,92,Short); mPrHead(sut,94,Short); mPrHead(gut,96,Short);
    mPrHead(sstat,98,Short); mPrHead(gstat,100,Short); mPrHead(tstat,102,Short);
    mPrHead(laga,104,Short); mPrHead(lagb ,106,Short); mPrHead(delrt,108,Short);
    mPrHead(muts,110,Short); mPrHead(mute,112,Short);
    mPrHead(ns,114,UnsignedShort); mPrHead(dt,116,UnsignedShort);
    mPrHead(gain,118,Short);

    for ( int idx=0; idx<30; idx++ )
    {
	int byt = 120 + idx*2;
	if ( *(short*)(buf+byt) ) mPrHead(-,byt,Short);
    }
    for ( int idx=0; idx<15; idx++ )
    {
	int byt = 180 + idx*4;
	if ( *(int*)(buf+byt) ) mPrHead(-,byt,Int);
    }
}


unsigned short SegyTraceheader::nrSamples() const
{
    if ( needswap ) swap_bytes( ((unsigned char*)buf)+114, 2 );
    unsigned short rv = IbmFormat::asUnsignedShort( buf+114 );
    if ( needswap ) swap_bytes( ((unsigned char*)buf)+114, 2 );
    return rv;
}


void SegyTraceheader::putSampling( SamplingData<float> sd, unsigned short ns )
{
    const float zfac = SI().zFactor();
    float drt = sd.start * zfac;
    short delrt = (short)mNINT(drt);
    IbmFormat::putShort( delrt, buf+108 );
    IbmFormat::putUnsignedShort( (unsigned short)
	    			 (sd.step*1e3*zfac+.5), buf+116 );
    IbmFormat::putUnsignedShort( ns, buf+114 );
}


void SegyTraceheader::use( const SeisTrcInfo& ti )
{
    // First put some Statoil standards
    IbmFormat::putInt( seqnr++, buf );
    IbmFormat::putShort( 1, buf+88 );   // counit
    IbmFormat::putInt( ti.binid.inl, buf+4 );
    IbmFormat::putInt( ti.binid.crl, buf+20 );
    if ( !hdef.isClashing(180) )
	IbmFormat::putInt( mNINT(ti.coord.x * 10), buf+180 );
    if ( !hdef.isClashing(184) )
	IbmFormat::putInt( mNINT(ti.coord.y * 10), buf+184 );
    if ( !hdef.isClashing(188) )
	IbmFormat::putInt( ti.binid.inl, buf+188 );
    if ( !hdef.isClashing(192) )
	IbmFormat::putInt( ti.binid.crl, buf+192 );

    // Now the more general standards
    IbmFormat::putInt( ti.nr, buf+16 ); // put number at 'ep'
    IbmFormat::putShort( 1, buf+28 );
    IbmFormat::putInt( mNINT(ti.offset), buf+36 );
    IbmFormat::putInt( mNINT(ti.azimuth*1e6), buf+40 );
    IbmFormat::putShort( -10, buf+70 ); // scalco
    if ( mIsUndefined(ti.coord.x) )
    {
	if ( hdef.xcoord != 255 )
	    IbmFormat::putInt( 0, buf+hdef.xcoord-1 );
	if ( hdef.ycoord != 255 )
	    IbmFormat::putInt( 0, buf+hdef.ycoord-1 );
    }
    else
    {
	if ( hdef.xcoord != 255 )
	    IbmFormat::putInt( mNINT(ti.coord.x * 10), buf+hdef.xcoord-1 );
	if ( hdef.ycoord != 255 )
	    IbmFormat::putInt( mNINT(ti.coord.y * 10), buf+hdef.ycoord-1 );
    }
    if ( ti.binid.inl > 0 && hdef.inl != 255 )
    {
	if ( hdef.inlbytesz == 2 )
	    IbmFormat::putShort( ti.binid.inl, buf+hdef.inl-1 );
	else
	    IbmFormat::putInt( ti.binid.inl, buf+hdef.inl-1 );
    }
    if ( ti.binid.crl > 0 && hdef.crl != 255 )
    {
	if ( hdef.crlbytesz == 2 )
	    IbmFormat::putShort( ti.binid.crl, buf+hdef.crl-1 );
	else
	    IbmFormat::putInt( ti.binid.crl, buf+hdef.crl-1 );
    }

    const float zfac = SI().zFactor();
    if ( !mIsUndefined(ti.pick) && hdef.pick != 255 )
	IbmFormat::putInt( mNINT(ti.pick*zfac), buf+hdef.pick-1 );

    // Absolute priority, therefore possibly overwriting previous
    float drt = ti.sampling.start * zfac;
    short delrt = (short)mNINT(drt);
    IbmFormat::putShort( delrt, buf+108 );
    IbmFormat::putUnsignedShort( (unsigned short)(ti.sampling.step*zfac*1e3+.5),
				 buf+116 );
}


float SegyTraceheader::postScale( int numbfmt ) const
{
    if ( numbfmt != 2 && numbfmt != 3 && numbfmt != 5 ) return 1.;

    short trwf = IbmFormat::asShort( buf+168 );
    return trwf > 0 && trwf < 100 ? (float)(1. / IntPowerOf(2,trwf)) : 1.;
}


void SegyTraceheader::fill( SeisTrcInfo& ti, float extcoordsc ) const
{
    ti.nr = IbmFormat::asInt( buf+0 );
    const float zfac = 1. / SI().zFactor();
    ti.sampling.start = ((float)IbmFormat::asShort(buf+108)) * zfac;
    ti.sampling.step = IbmFormat::asUnsignedShort( buf+116 ) * zfac * 0.001;
    ti.pick = ti.refpos = mUndefValue;
    if ( hdef.pick != 255 ) ti.pick = IbmFormat::asInt( buf+hdef.pick ) * zfac;
    ti.coord.x = ti.coord.y = 0;
    if ( hdef.xcoord != 255 ) ti.coord.x = IbmFormat::asInt( buf+hdef.xcoord-1);
    if ( hdef.ycoord != 255 ) ti.coord.y = IbmFormat::asInt( buf+hdef.ycoord-1);
    ti.binid.inl = ti.binid.crl = 0;
    if ( hdef.inl != 255 )
	ti.binid.inl = hdef.inlbytesz == 2 ? IbmFormat::asShort(buf+hdef.inl-1)
					   : IbmFormat::asInt(buf+hdef.inl-1);
    if ( hdef.crl != 255 )
	ti.binid.crl = hdef.crlbytesz == 2 ? IbmFormat::asShort(buf+hdef.crl-1)
					   : IbmFormat::asInt(buf+hdef.crl-1);
    ti.offset = IbmFormat::asInt( buf+36 );

    double scale = getCoordScale( extcoordsc );
    ti.coord.x *= scale;
    ti.coord.y *= scale;
}


double SegyTraceheader::getCoordScale( float extcoordsc ) const
{
    if ( !mIsUndefined(extcoordsc) ) return extcoordsc;
    short scalco = IbmFormat::asShort( buf+70 );
    return scalco ? (scalco > 0 ? scalco : -1./scalco) : 1;
}


Coord SegyTraceheader::getCoord( bool rcv, float extcoordsc )
{
    double scale = getCoordScale( extcoordsc );
    Coord ret(	IbmFormat::asInt( buf+(rcv?80:72) ),
		IbmFormat::asInt( buf+(rcv?84:76) ) );
    ret.x *= scale;
    ret.y *= scale;
    return ret;
}


void SegyTraceheaderDef::usePar( const IOPar& iopar )
{
    const char* res = iopar[ sInlByte ];
    if ( *res ) inl = atoi( res );
    res = iopar[ sCrlByte ];
    if ( *res ) crl = atoi( res );
    res = iopar[ sXCoordByte ];
    if ( *res ) xcoord = atoi( res );
    res = iopar[ sYCoordByte ];
    if ( *res ) ycoord = atoi( res );
    res = iopar[ sInlByteSz ];
    if ( *res ) inlbytesz = atoi( res );
    res = iopar[ sCrlByteSz ];
    if ( *res ) crlbytesz = atoi( res );
    res = iopar[ sPickByte ];
    if ( *res ) pick = atoi( res );
}


void SegyTraceheaderDef::fromSettings()
{
    const IOPar* useiop = &Settings::common();
    IOPar* subiop = useiop->subselect( "SEG-Y" );
    if ( subiop && subiop->size() )
	useiop = subiop;
    usePar( *useiop );
    delete subiop;
}


void SegyTraceheaderDef::fillPar( IOPar& iopar, const char* key ) const
{
    iopar.set( IOPar::compKey(key,sInlByte), inl );
    iopar.set( IOPar::compKey(key,sCrlByte), crl );
    iopar.set( IOPar::compKey(key,sXCoordByte), xcoord );
    iopar.set( IOPar::compKey(key,sYCoordByte), ycoord );
    iopar.set( IOPar::compKey(key,sInlByteSz), inlbytesz );
    iopar.set( IOPar::compKey(key,sCrlByteSz), crlbytesz );
    iopar.set( IOPar::compKey(key,sPickByte), pick );
}
