/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 1995
 * FUNCTION : Seg-Y headers
-*/

static const char* rcsID = "$Id: segyhdr.cc,v 1.86 2010-09-30 07:43:51 cvsbert Exp $";


#include "segyhdr.h"
#include "string2.h"
#include "survinfo.h"
#include "ibmformat.h"
#include "settings.h"
#include "seisinfo.h"
#include "seispacketinfo.h"
#include "cubesampling.h"
#include "msgh.h"
#include "math2.h"
#include "envvars.h"
#include "timefun.h"
#include "linekey.h"
#include "posimpexppars.h"

#include <string.h>
#include <ctype.h>
#include <iostream>
#include <stdio.h>
#include <math.h>

namespace SEGY
{
const char* TrcHeaderDef::sXCoordByte()	{ return "X-coord byte"; }
const char* TrcHeaderDef::sYCoordByte()	{ return "Y-coord byte"; }
const char* TrcHeaderDef::sInlByte()	{ return "In-line byte"; }
const char* TrcHeaderDef::sCrlByte()	{ return "Cross-line byte"; }
const char* TrcHeaderDef::sOffsByte()	{ return "Offset byte"; }
const char* TrcHeaderDef::sAzimByte()	{ return "Azimuth byte"; }
const char* TrcHeaderDef::sTrNrByte()	{ return "Trace number byte"; }
const char* TrcHeaderDef::sPickByte()	{ return "Pick byte"; }
const char* TrcHeaderDef::sRefNrByte()	{ return "Reference number byte"; }
const char* TrcHeaderDef::sInlByteSz()	{ return "Nr bytes for In-line"; }
const char* TrcHeaderDef::sCrlByteSz()	{ return "Nr bytes for Cross-line"; }
const char* TrcHeaderDef::sOffsByteSz()	{ return "Nr bytes for Offset"; }
const char* TrcHeaderDef::sAzimByteSz()	{ return "Nr bytes for Azimuth"; }
const char* TrcHeaderDef::sTrNrByteSz()	{ return "Nr bytes for trace number"; }
const char* TrcHeaderDef::sPickByteSz()	{ return "Nr bytes for Pick"; }
const char* TrcHeaderDef::sRefNrByteSz() { return "Nr bytes for RefNr"; }
}

static bool sInfo2D = false;
bool& SEGY::TxtHeader::info2D()  { return sInfo2D; }

static void Ebcdic2Ascii(unsigned char*,int);
static void Ascii2Ebcdic(unsigned char*,int);

static const int cTxtHeadNrLines = 40;
static const int cTxtHeadCharsPerLine = 80;


SEGY::TxtHeader::TxtHeader( bool rev1 )
    : rev1_(rev1)
{
    clear();

    FileNameString buf;
    const char* res = Settings::common().find( "Company" );
    if ( !res ) res = "OpendTect";
    buf = "Created by: "; buf += res;
    buf += "     ("; buf += Time::getDateTimeString(); buf += ")";
    putAt( 1, 6, 75, buf );
    putAt( 2, 6, 75, BufferString("Survey: '", SI().name(),"'") );
    BinID bid = SI().sampling(false).hrg.start;
    Coord coord = SI().transform( bid );
    coord.x = fabs(coord.x); coord.y = fabs(coord.y);
    if ( !mIsEqual(bid.inl,coord.x,mDefEps)
      || !mIsEqual(bid.crl,coord.x,mDefEps)
      || !mIsEqual(bid.inl,coord.y,mDefEps)
      || !mIsEqual(bid.crl,coord.y,mDefEps) )
    {
	putAt( 13, 6, 75, "Survey setup:" );
	char* pbuf = buf.buf();
	coord = SI().transform( bid );
	bid.fill( pbuf ); buf += " = "; coord.fill( pbuf + strlen(buf) );
	putAt( 14, 6, 75, buf );
	bid.crl = SI().sampling(false).hrg.stop.crl;
	coord = SI().transform( bid );
	bid.fill( pbuf ); buf += " = "; coord.fill( pbuf + strlen(buf) );
	putAt( 15, 6, 75, buf );
	bid.inl = SI().sampling(false).hrg.stop.inl;
	bid.crl = SI().sampling(false).hrg.start.crl;
	coord = SI().transform( bid );
	bid.fill( pbuf ); buf += " = "; coord.fill( pbuf + strlen(buf) );
	putAt( 16, 6, 75, buf );
    }

    if ( !SI().zIsTime() )
    {
	buf = "Depth survey: 1 SEG-Y millisec = 1 ";
	buf += SI().getZUnitString(false);
	putAt( 18, 6, 75, buf.buf() );
    }
}


void SEGY::TxtHeader::clearText()
{
    memset( txt_, ' ', SegyTxtHeaderLength );
}


void SEGY::TxtHeader::setLineStarts()
{
    char cbuf[3];
    for ( int iln=0; iln<cTxtHeadNrLines; iln++ )
    {
	int i80 = iln*cTxtHeadCharsPerLine; txt_[i80] = 'C';
	sprintf( cbuf, "%02d", iln+1 );
	txt_[i80+1] = cbuf[0]; txt_[i80+2] = cbuf[1];
    }

    BufferString rvstr( "SEG Y REV" );
    rvstr += rev1_ ? 1 : 0;
    putAt( cTxtHeadNrLines-1, 6, 75, rvstr.buf() );
    putAt( cTxtHeadNrLines, 6, 75, "END TEXTUAL HEADER" );
}


void SEGY::TxtHeader::setAscii()
{ if ( txt_[0] != 'C' ) Ebcdic2Ascii( txt_, SegyTxtHeaderLength ); }
void SEGY::TxtHeader::setEbcdic()
{ if ( txt_[0] == 'C' ) Ascii2Ebcdic( txt_, SegyTxtHeaderLength ); }


void SEGY::TxtHeader::setUserInfo( const char* infotxt )
{
    if ( !infotxt || !*infotxt ) return;

    BufferString buf;
    int lnr = 16;
    while ( lnr < 35 )
    {
	char* ptr = buf.buf();
	int idx = 0;
	while ( *infotxt && *infotxt != '\n' && ++idx < 75 )
	    *ptr++ = *infotxt++;
	*ptr = '\0';
	putAt( lnr, 5, cTxtHeadCharsPerLine, buf );

	if ( !*infotxt ) break;
	lnr++;
	infotxt++;
    }
}


void SEGY::TxtHeader::setPosInfo( const SEGY::TrcHeaderDef& thd )
{

    putAt( 7, 6, 75, "Positions according to the SEG-Y Rev. 1 standard." );
    if ( !sInfo2D ) return;

    BufferString txt( "Trace number at byte: ", (int)thd.trnr, " (" );
    txt += (int)thd.trnrbytesz; txt += " bytes)";
    putAt( 8, 6, 6+txt.size(), txt.buf() );

    if ( !thd.linename.isEmpty() )
    {
	LineKey lk( thd.linename );
	putAt( 4, 6, 20, "Line name:" );
	putAt( 4, 20, 75, lk.lineName() );
	putAt( 4, 45, 75, lk.attrName() );
    }
    if ( thd.pinfo )
    {
	BufferString txt( "Trace number range: ",
			  thd.pinfo->crlrg.start, " - " );
	txt += thd.pinfo->crlrg.stop;
	putAt( 5, 6, 75, txt );
    }

    putAt( 36, 6, 75, "2-D line" );
}


void SEGY::TxtHeader::setStartPos( float sp )
{
    if ( mIsZero(sp,mDefEps) ) return;

    BufferString txt( "First sample at:", sp );
    putAt( 10, 6, 75, txt );
    putAt( 11, 10, 75, "(in bytes 109 (+) and 105 (-) of the trace header)");
}


void SEGY::TxtHeader::getText( BufferString& bs ) const
{
    char buf[cTxtHeadCharsPerLine+1];
    getFrom( 1, 1, cTxtHeadCharsPerLine, buf );
    bs = buf;
    for ( int iln=2; iln<=cTxtHeadNrLines; iln++ )
    {
	bs += "\n";
	getFrom( iln, 1, cTxtHeadCharsPerLine, buf );
	bs += buf;
    }
}


void SEGY::TxtHeader::setText( const char* txt )
{
    clearText();

    BufferString bs( txt );
    char* ptr = bs.buf();
    for ( int iln=1; iln<=cTxtHeadNrLines; iln++ )
    {
	char* endptr = strchr( ptr, '\n' );
	if ( !endptr ) break;
	*endptr = '\0';

	putAt( iln, 1, cTxtHeadCharsPerLine, ptr );
	ptr = endptr + 1; if ( !*ptr ) break;
    }

    setLineStarts();
}


void SEGY::TxtHeader::getFrom( int line, int pos, int endpos, char* str ) const
{
    if ( !str ) return;

    int charnr = (line-1)*cTxtHeadCharsPerLine + pos - 1;
    if ( endpos > cTxtHeadCharsPerLine ) endpos = cTxtHeadCharsPerLine;
    int maxcharnr = (line-1)*cTxtHeadCharsPerLine + endpos;

    while ( isspace(txt_[charnr]) && charnr < maxcharnr ) charnr++;
    while ( charnr < maxcharnr ) *str++ = txt_[charnr++];
    *str = '\0';
    removeTrailingBlanks( str );
}


void SEGY::TxtHeader::putAt( int line, int pos, int endpos, const char* str )
{
    if ( !str || !*str ) return;

    int charnr = (line-1)*cTxtHeadCharsPerLine + pos - 1;
    if ( endpos > cTxtHeadCharsPerLine ) endpos = cTxtHeadCharsPerLine;
    int maxcharnr = (line-1)*cTxtHeadCharsPerLine + endpos;

    while ( charnr < maxcharnr && *str )
    {
	txt_[charnr] = *str;
	charnr++; str++;
    }
}


void SEGY::TxtHeader::dump( std::ostream& stream ) const
{
    BufferString buf; getText( buf );
    stream << buf << std::endl;
}


static const unsigned char* getBytes( const unsigned char* inpbuf,
				      bool needswap, int bytenr, int nrbytes )
{
    static unsigned char swpbuf[4];
    if ( !needswap ) return inpbuf + bytenr;

    memcpy( swpbuf, inpbuf+bytenr, nrbytes );
    SwapBytes( swpbuf, nrbytes );
    return swpbuf;
}


#define mGetBytes(bnr,nrb) getBytes( buf, needswap, bnr, nrb )


SEGY::BinHeader::BinHeader( bool rev1 )
	: needswap(false)
    	, isrev1(rev1 ? 256 : 0)
    	, nrstzs(0)
    	, fixdsz(1)
{
    memset( &jobid, 0, SegyBinHeaderLength );
    format = 1;
    mfeet = SI().xyInFeet() ? 2 : 1;
    float fhdt = SeisTrcInfo::defaultSampleInterval() * 1000;
    if ( SI().zIsTime() && fhdt < 32.768 )
	fhdt *= 1000;
    hdt = (short)fhdt;
}


#define mSBHGet(memb,typ,nb) \
    if ( needswap ) SwapBytes( b, nb ); \
    memb = IbmFormat::as##typ( b ); \
    if ( needswap ) SwapBytes( b, nb ); \
    b += nb;

void SEGY::BinHeader::getFrom( const void* buf )
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

    for ( int i=0; i<SegyBinHeaderUnassUShorts; i++ )
    {
	mSBHGet(hunass[i],UnsignedShort,2);
    }
    isrev1 = hunass[120];
    fixdsz = hunass[121];
    nrstzs = hunass[122];
}


#define mSBHPut(memb,typ,nb) \
    IbmFormat::put##typ( memb, b ); \
    if ( needswap ) SwapBytes( b, nb ); \
    b += nb;


void SEGY::BinHeader::putTo( void* buf ) const
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

    unsigned short* v = const_cast<unsigned short*>( hunass );
    v[120] = isrev1; v[121] = fixdsz; v[122] = nrstzs;
    for ( int i=0; i<SegyBinHeaderUnassUShorts; i++ )
	{ mSBHPut(hunass[i],UnsignedShort,2); }
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


// Comments added after request from Hamish McIntyre

#define mPrHead(mem,byt,comm) \
	if ( mem ) \
	{ \
	    strm << '\t' << #mem << '\t' << byt+1 << '\t' << mem \
		 << "\t(" << comm << ')'; \
	    strm << '\n'; \
	}

void SEGY::BinHeader::dump( std::ostream& strm ) const
{
    mPrHead( jobid, 0, "job identification number" )
    mPrHead( lino, 4, "line number (only one line per reel)" )
    mPrHead( reno, 8, "reel number" )
    mPrHead( ntrpr, 12, "number of data traces per record" )
    mPrHead( nart, 14, "number of auxiliary traces per record" )
    mPrHead( hdt, 16, "sample interval (micro secs or mm)" )
    mPrHead( dto, 18, "same for original field recording" )
    mPrHead( hns, 20, "number of samples per trace" )
    mPrHead( nso, 22, "same for original field recording" )
    mPrHead( format, 24, "sample format (1=float, 3=16 bit, 8=8-bit)" )
    mPrHead( fold, 26, "CDP fold expected per CDP ensemble" )
    mPrHead( tsort, 28, "trace sorting code" )
    mPrHead( vscode, 30, "vertical sum code" )
    mPrHead( hsfs, 32, "sweep frequency at start" )
    mPrHead( hsfe, 34, "sweep frequency at end" )
    mPrHead( hslen, 36, "sweep length (ms)" )
    mPrHead( hstyp, 38, "sweep type code" )
    mPrHead( schn, 40, "trace number of sweep channel" )
    mPrHead( hstas, 42, "sweep trace taper length at start" )
    mPrHead( hstae, 44, "sweep trace taper length at end" )
    mPrHead( htatyp, 46, "sweep trace taper type code" )
    mPrHead( hcorr, 48, "correlated data traces code" )
    mPrHead( bgrcv, 50, "binary gain recovered code" )
    mPrHead( rcvm, 52, "amplitude recovery method code" )
    mPrHead( mfeet, 54, "measurement system code (1=m 2=ft)" )
    mPrHead( polyt, 56, "impulse signal polarity code" )
    mPrHead( vpol, 58, "vibratory polarity code" )

    SEGY::BinHeader& self = *const_cast<SEGY::BinHeader*>(this);
    unsigned short tmpisrev1 = isrev1;
    if ( isrev1 ) self.isrev1 = 1;
    mPrHead( isrev1, 300, "[REV1 only] SEG-Y revision code" )
    self.isrev1 = tmpisrev1;
    mPrHead( fixdsz, 302, "[REV1 only] Fixed trace size?" )
    mPrHead( nrstzs, 304, "[REV1 only] Number of extra headers" );

    for ( int i=0; i<SegyBinHeaderUnassUShorts; i++ )
	if ( hunass[i] != 0 && (i < 120 || i > 122) )
	    strm << "\tExtra\t" << 60 + 2*i << '\t' << hunass[i]
		 << "\t(Non-standard - unassigned)\n";
    strm << std::endl;
}


SEGY::TrcHeader::TrcHeader( unsigned char* b, bool rev1,
			    const SEGY::TrcHeaderDef& hd )
    : buf(b)
    , hdef(hd)
    , needswap(false)
    , isrev1(rev1)
    , seqnr(1)
    , lineseqnr(1)
    , previnl(-1)
    , isusable(true)
    , nonrectcoords(false)
{
}


void SEGY::TrcHeader::initRead()
{
    const int trid = IbmFormat::asShort( mGetBytes(28,2) );
    isusable = trid < 2 || trid > 10;
    const int counit = IbmFormat::asShort( mGetBytes(88,2) );
    nonrectcoords = counit > 1 && counit < 5;
}


#undef mSEGYTrcDef
#define mSEGYTrcDef(attrnr,mem,byt,fun,nrbyts) \
    strm << '\t' << #mem << '\t' << byt+1 << '\t' \
         << IbmFormat::as##fun( getBytes(buf,needswap,byt,nrbyts) ) << '\n'
#undef mSEGYTrcDefUnass
#define mSEGYTrcDefUnass(attrnr,byt,fun,nrbyts) \
    if ( IbmFormat::as##fun( getBytes(buf,needswap,byt,nrbyts) ) ) \
	strm << '\t' << '-' << '\t' << byt+1 << '\t' \
	     << IbmFormat::as##fun( getBytes(buf,needswap,byt,nrbyts) ) << '\n'

void SEGY::TrcHeader::dump( std::ostream& strm ) const
{
#include "segyhdr.inc"
}


#undef mSEGYTrcDef
#define mSEGYTrcDef(attrnr,mem,byt,fun,nrbyts) \
    if ( nr == attrnr ) \
	return SEGY::TrcHeader::Val( byt, #mem, \
		IbmFormat::as##fun( getBytes(buf,needswap,byt,nrbyts) ) )
#undef mSEGYTrcDefUnass
#define mSEGYTrcDefUnass(attrnr,byt,fun,nrbyts) \
    mSEGYTrcDef(attrnr,-,byt,fun,nrbyts)

SEGY::TrcHeader::Val SEGY::TrcHeader::getVal( int nr ) const
{
#include "segyhdr.inc"
    return Val( -1, "", 0 );
}


unsigned short SEGY::TrcHeader::nrSamples() const
{
    return IbmFormat::asUnsignedShort( getBytes(buf,needswap,114,2) );
}


#define mSTHPut(memb,b,typ,nb) { \
    IbmFormat::put##typ( memb, buf+b ); \
    if ( needswap ) SwapBytes( buf+b, nb ); }
#define mSTHPutShort(memb,b) mSTHPut(memb,b,Short,2)
#define mSTHPutUnsignedShort(memb,b) mSTHPut(memb,b,UnsignedShort,2)
#define mSTHPutInt(memb,b) mSTHPut(memb,b,Int,4)

void SEGY::TrcHeader::putSampling( SamplingData<float> sdin, unsigned short ns )
{
    SamplingData<float> sd( sdin );
    mPIEPAdj(Z,sd.start,false); mPIEPAdj(Z,sd.step,false);

    const float zfac = SI().zFactor();
    float drt = sd.start * zfac;
    short delrt = (short)mNINT(drt);
    mSTHPutShort(-delrt,104); // For HRS and Petrel
    mSTHPutShort(delrt,108);
    unsigned short sr_us = (unsigned short)( sd.step * 1e3 * zfac + .5 );
    mSTHPutUnsignedShort(sr_us,116);
    if ( ns != 0 )
	mSTHPutUnsignedShort(ns,114);
}


static void putRev1Flds( const SeisTrcInfo& ti, unsigned char* buf,
			 bool needswap )
{
    Coord crd( ti.coord ); mPIEPAdj(Coord,crd,false);
    const int icx = mNINT(crd.x*10); const int icy = mNINT(crd.y*10);
    mSTHPutInt(icx,180);
    mSTHPutInt(icy,184);
    BinID bid( ti.binid ); mPIEPAdj(BinID,bid,false);
    mSTHPutInt(bid.inl,188); mSTHPutInt(bid.crl,192);
    int tnr = ti.nr; mPIEPAdj(TrcNr,tnr,false);
    if ( ti.refnr != 0 && !mIsUdf(ti.refnr) )
    {
	tnr = mNINT(ti.refnr*10);
	const short scl = -10; mSTHPutShort(scl,200);
    }
    mSTHPutInt(tnr,196);
}


void SEGY::TrcHeader::use( const SeisTrcInfo& ti )
{
    if ( !isrev1 ) // starting default
	putRev1Flds( ti, buf, needswap );

    mSTHPutShort(1,28); // trid
    mSTHPutShort(1,34); // duse
    mSTHPutShort(1,38); // counit

    const bool is2d = SEGY::TxtHeader::info2D();
    if ( !is2d && ti.binid.inl != previnl )
	lineseqnr = 1;
    previnl = ti.binid.inl;
    int nr2put = is2d ? seqnr : lineseqnr;
    mSTHPutInt(nr2put,0);
    mSTHPutInt(seqnr,4);
    seqnr++; lineseqnr++;
    if ( is2d ) 
	{ nr2put = ti.nr; mPIEPAdj(TrcNr,nr2put,false); }
    else
	{ nr2put = ti.binid.crl; mPIEPAdj(Inl,nr2put,false); }
    mSTHPutInt(nr2put,20);

    mSTHPutShort(-10,70); // scalco
    Coord crd( ti.coord );
    if ( mIsUdf(crd.x) ) crd.x = crd.y = 0;
    mPIEPAdj(Coord,crd,false);
    const int icx = mNINT(crd.x*10); const int icy = mNINT(crd.y*10);
    mSTHPutInt(icx,hdef.xcoord-1);
    mSTHPutInt(icy,hdef.ycoord-1);

#define mSTHPutIntMemb(timemb,hdmemb) \
    if ( timemb > 0 && !mIsUdf(timemb) && hdef.hdmemb != 255 ) \
    { \
	if ( hdef.hdmemb##bytesz == 2 ) \
	    mSTHPutShort(timemb,hdef.hdmemb-1) \
	else \
	    mSTHPutInt(timemb,hdef.hdmemb-1) \
    }

    BinID bid( ti.binid ); mPIEPAdj(BinID,bid,false);
    mSTHPutIntMemb(bid.inl,inl);
    mSTHPutIntMemb(bid.crl,crl);
    int tinr = ti.nr; mPIEPAdj(TrcNr,tinr,false);
    mSTHPutIntMemb(tinr,trnr);
    float tioffs = ti.offset; mPIEPAdj(Offset,tioffs,false);
    int iv = mNINT( tioffs ); mSTHPutIntMemb(iv,offs);
    iv = mNINT( ti.azimuth * 360 / M_PI ); mSTHPutIntMemb(iv,azim);

    int itemp;
#define mSTHPutFloatMemb(memb,fac) \
    if ( ti.memb > 0 && !mIsUdf(ti.memb) && hdef.memb != 255 ) \
    { \
	itemp = mNINT(ti.memb*fac); \
	if ( hdef.memb##bytesz == 2 ) \
	    mSTHPutShort(itemp,hdef.memb-1) \
	else \
	    mSTHPutInt(itemp,hdef.memb-1) \
    }

    const float zfac = SI().zFactor();
    mSTHPutFloatMemb(pick,zfac)
    mSTHPutFloatMemb(refnr,10)

    // Absolute priority, therefore possibly overwriting previous
    putSampling( ti.sampling, 0 ); // 0=ns must be set elsewhere

    if ( isrev1 ) // Now this overrules everything
	putRev1Flds( ti, buf, needswap );
}


float SEGY::TrcHeader::postScale( int numbfmt ) const
{
    // There seems to be software (Paradigm?) putting this on byte 189
    // Then we'd expect this to be 4 byte. Sigh. How far do we need to go
    // to support crap from SEG-Y vandals?
    static bool postscale_byte_established = false;
    static int postscale_byte;
    static bool postscale_isint;
    if ( !postscale_byte_established )
    {
	postscale_byte_established = true;
	postscale_byte = GetEnvVarIVal( "OD_SEGY_TRCSCALE_BYTE", 169 );
	postscale_isint = GetEnvVarYN( "OD_SEGY_TRCSCALE_4BYTE" );
    }

    unsigned char* ptr = buf + postscale_byte - 1 ;
    short trwf = postscale_isint ?
	IbmFormat::asInt( mGetBytes(postscale_byte-1,4) )
      : IbmFormat::asShort( mGetBytes(postscale_byte-1,2) );
    if ( trwf == 0 || trwf > 50 || trwf < -50 ) return 1;

    // According to the standard, trwf cannot be negative, but hey, standards
    // are for wimps, right?
    const bool isneg = trwf < 0;
    if ( isneg )
	trwf = -trwf;
    float ret = Math::IntPowerOf( ((float)2), trwf );
    return isneg ? ret : 1. / ret;
}


static void getRev1Flds( SeisTrcInfo& ti, const unsigned char* buf,
			 bool needswap )
{
    ti.coord.x = IbmFormat::asInt( mGetBytes(180,4) );
    ti.coord.y = IbmFormat::asInt( mGetBytes(184,4) );
    ti.binid.inl = IbmFormat::asInt( mGetBytes(188,4) );
    ti.binid.crl = IbmFormat::asInt( mGetBytes(192,4) );
    ti.refnr = IbmFormat::asInt( mGetBytes(196,4) );
    short scalnr = IbmFormat::asShort( mGetBytes(200,2) );
    if ( scalnr )
    {
	ti.refnr *= (scalnr > 0 ? scalnr : -1./scalnr);
	ti.nr = mNINT(ti.refnr);
    }
    mPIEPAdj(Coord,ti.coord,true);
    mPIEPAdj(BinID,ti.binid,true);
    mPIEPAdj(TrcNr,ti.nr,true);
}


void SEGY::TrcHeader::fill( SeisTrcInfo& ti, float extcoordsc ) const
{
    if ( mIsZero(extcoordsc,1e-8)
	    && !GetEnvVarYN("OD_ALLOW_ZERO_COORD_SCALING") )
    {
	static bool warningdone = false;
	if ( !warningdone )
	{
	    ErrMsg( "Replacing requested zero scaling with 1" );
	    warningdone = true;
	}
	extcoordsc = 1;
    }

    if ( !isrev1 )
	getRev1Flds( ti, buf, needswap );

    const float zfac = 1. / SI().zFactor();
    short delrt = IbmFormat::asShort( mGetBytes(108,2) );
    if ( delrt == 0 )
    {
	delrt = -IbmFormat::asShort( mGetBytes(104,2) ); // HRS and Petrel
	static const bool smt_bad_laga = GetEnvVarYN("OD_SEGY_BAD_LAGA");
	if ( smt_bad_laga )
	    delrt = -delrt;
	float startz = delrt * zfac;
	if ( startz < -5000 || startz > 10000 )
	    delrt = 0;
    }
    ti.sampling.start = delrt * zfac;
    ti.sampling.step = IbmFormat::asUnsignedShort( mGetBytes(116,2) )
			* zfac * 0.001;
    mPIEPAdj(Z,ti.sampling.start,true);
    mPIEPAdj(Z,ti.sampling.step,true);

    ti.pick = ti.refnr = mUdf(float);
    ti.nr = IbmFormat::asInt( mGetBytes(0,4) );
    int itemp;
#define mGetFloatVal(memb,fac) \
    if ( hdef.memb != 255 ) \
    {\
	itemp = hdef.memb##bytesz == 2 \
	    ? IbmFormat::asShort( mGetBytes(hdef.memb-1,2) ) \
	    : IbmFormat::asInt( mGetBytes(hdef.memb-1,4) ); \
	ti.memb = ((float)itemp) * fac; \
    }
    mGetFloatVal(pick,0.001);
    float nrfac = 1;
    short scalnr = IbmFormat::asShort( mGetBytes(200,2) );
    if ( scalnr == -10 || scalnr == -100 || scalnr == -1000 )
	nrfac = 1 / ((float)(-scalnr));
    mGetFloatVal(refnr,nrfac);

    mPIEPAdj(Z,ti.pick,true);
    ti.coord.x = ti.coord.y = 0;
    if ( hdef.xcoord != 255 )
	ti.coord.x = IbmFormat::asInt( mGetBytes(hdef.xcoord-1,4));
    if ( hdef.ycoord != 255 )
	ti.coord.y = IbmFormat::asInt( mGetBytes(hdef.ycoord-1,4));

#define mGetIntVal(timemb,hdmemb) \
    if ( hdef.hdmemb != 255 ) \
	ti.timemb = hdef.hdmemb##bytesz == 2 \
	? IbmFormat::asShort( mGetBytes(hdef.hdmemb-1,2) ) \
	: IbmFormat::asInt( mGetBytes(hdef.hdmemb-1,4) )
	
    ti.binid.inl = ti.binid.crl = 0;
    mGetIntVal(binid.inl,inl);
    mGetIntVal(binid.crl,crl);
    mPIEPAdj(BinID,ti.binid,true);
    mGetIntVal(offset,offs);
    if ( ti.offset < 0 ) ti.offset = -ti.offset;
    mGetIntVal(azimuth,azim);
    ti.azimuth *= M_PI / 360;
    if ( hdef.trnr < 254 )
	{ mGetIntVal(nr,trnr); }
    else
    {
	// Trick to set trace number to sequence number if byte location is 255
	static int seqnr;
	if ( hdef.trnr == 254 )
	    seqnr++;
	else
	    { seqnr = 1; const_cast<SEGY::TrcHeaderDef&>(hdef).trnr = 254; }
	ti.nr = seqnr;
    }
    mPIEPAdj(TrcNr,ti.nr,true);

    if ( isrev1 )
    {
	const int oldnr = ti.nr;
	getRev1Flds( ti, buf, needswap );
	if ( oldnr ) ti.nr = oldnr;
    }

    const double scale = getCoordScale( extcoordsc );
    ti.coord.x *= scale; ti.coord.y *= scale;
    mPIEPAdj(Coord,ti.coord,true);
}


double SEGY::TrcHeader::getCoordScale( float extcoordsc ) const
{
    if ( !mIsUdf(extcoordsc) ) return extcoordsc;
    const short scalco = IbmFormat::asShort( mGetBytes(70,2) );
    return scalco ? (scalco > 0 ? scalco : -1./scalco) : 1;
}


Coord SEGY::TrcHeader::getCoord( bool rcv, float extcoordsc )
{
    double scale = getCoordScale( extcoordsc );
    Coord ret(	IbmFormat::asInt( mGetBytes(rcv?80:72,4) ),
		IbmFormat::asInt( mGetBytes(rcv?84:76,4) ) );
    ret.x *= scale; ret.y *= scale;
    return ret;
}


#define mGtFromPar(str,memb) \
    res = iopar.find( str ); \
    if ( res && *res ) memb = atoi( res )

void SEGY::TrcHeaderDef::usePar( const IOPar& iopar )
{
    const char*
    mGtFromPar( sInlByte(), inl );
    mGtFromPar( sCrlByte(), crl );
    mGtFromPar( sOffsByte(), offs );
    mGtFromPar( sAzimByte(), azim );
    mGtFromPar( sTrNrByte(), trnr );
    mGtFromPar( sXCoordByte(), xcoord );
    mGtFromPar( sYCoordByte(), ycoord );
    mGtFromPar( sPickByte(), pick );
    mGtFromPar( sRefNrByte(), refnr );
    mGtFromPar( sInlByteSz(), inlbytesz );
    mGtFromPar( sCrlByteSz(), crlbytesz );
    mGtFromPar( sOffsByteSz(), offsbytesz );
    mGtFromPar( sAzimByteSz(), azimbytesz );
    mGtFromPar( sTrNrByteSz(), trnrbytesz );
    mGtFromPar( sPickByteSz(), pickbytesz );
    mGtFromPar( sRefNrByteSz(), refnrbytesz );
}


void SEGY::TrcHeaderDef::fromSettings()
{
    const IOPar* useiop = &Settings::common();
    IOPar* subiop = useiop->subselect( "SEG-Y" );
    if ( subiop && subiop->size() )
	useiop = subiop;
    usePar( *useiop );
    delete subiop;
}

#define mPutToPar(str,memb) \
    iopar.set( IOPar::compKey(key,str), memb )


void SEGY::TrcHeaderDef::fillPar( IOPar& iopar, const char* key ) const
{
    mPutToPar( sInlByte(), inl );
    mPutToPar( sCrlByte(), crl );
    mPutToPar( sOffsByte(), offs );
    mPutToPar( sAzimByte(), azim );
    mPutToPar( sTrNrByte(), trnr );
    mPutToPar( sXCoordByte(), xcoord );
    mPutToPar( sYCoordByte(), ycoord );
    mPutToPar( sPickByte(), pick );
    mPutToPar( sRefNrByte(), refnr );
    mPutToPar( sInlByteSz(), inlbytesz );
    mPutToPar( sCrlByteSz(), crlbytesz );
    mPutToPar( sOffsByteSz(), offsbytesz );
    mPutToPar( sAzimByteSz(), azimbytesz );
    mPutToPar( sTrNrByteSz(), trnrbytesz );
    mPutToPar( sPickByteSz(), pickbytesz );
    mPutToPar( sRefNrByteSz(), refnrbytesz );
}
