/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Nov 2000
 * FUNCTION : Interpret data buffers
-*/

static const char* rcsID = "$Id: datainterp.cc,v 1.10 2001-12-21 15:05:33 bert Exp $";

#include "datainterp.h"
#include "datachar.h"
#include "ibmformat.h"
#include "separstr.h"

DefineEnumNames(DataCharacteristics,UserType,1,"Data storage") {
	"0 - auto",
	"1 - 8  bit signed",
        "2 - 8  bit unsigned",
        "3 - 16 bit signed",
        "4 - 16 bit unsigned",
        "5 - 32 bit signed",
        "6 - 32 bit unsigned",
        "7 - 32 bit floating point",
        "8 - 64 bit floating point",
	0 };

// sys/types.h is a bit of a mess, so ...
typedef signed char TS1;
typedef signed short TS2;
typedef unsigned char TU1;
typedef unsigned short TU2;
typedef float TF;
typedef double TD;

// This may be a problem on windows and 64 bit machines. Fix when appropriate
typedef signed int TS4;
typedef unsigned int TU4;

// But this is fundamental mathematics
const TS1 cMS1 = 127;
const TS2 cMS2 = 32767;
const TS4 cMS4 = 2147483647L;
const TU1 cMU1 = 255;
const TU2 cMU2 = 65535;
const TU4 cMU4 = 4294967295UL;


void DataCharacteristics::set( unsigned char c1, unsigned char c2 )
{
    // remember that the 'zero' member is always zero.
    littleendian = (c2 & 0x80) || (c2 & 0x01);
    setFrom( c1, littleendian );

    unsigned char f = (c2 & 0x0e) >> 1;
    if ( !f )
    {
	f = (c2 & 0x70) >> 4;
	unsigned char g = 0;
	if ( f&0x04 ) g |= 0x01;
	if ( f&0x02 ) g |= 0x02;
	if ( f&0x01 ) g |= 0x04;
	f = g;
    }
    fmt = (DataCharacteristics::Format)f;
};


void DataCharacteristics::set( const char* s )
{
    BinDataDesc::set( s );
    FileMultiString fms( s );
    const int sz = fms.size();
    if ( sz > 3 )
	fmt = matchStringCI( "ibm", fms[1] ) ? DataCharacteristics::Ibm
					     : DataCharacteristics::Ieee;
    if ( sz > 4 )
	littleendian = yesNoFromString( fms[4] );
}


DataCharacteristics::DataCharacteristics( DataCharacteristics::UserType ut )
	: BinDataDesc( ut<F32, ut>UI32 || (int)ut % 2)
	, fmt(Ieee)
	, littleendian(__islittle__)
{
    if ( ut == Auto )
	*this = DataCharacteristics();
    else
	nrbytes = (BinDataDesc::ByteCount)
		  (ut < SI16 ? 1 : (ut < SI32 ? 2 : (ut > F32 ? 8 : 4) ) );
}


void DataCharacteristics::dump( unsigned char& c1, unsigned char& c2 ) const
{
    union _DC_union { unsigned char c;
	struct bits { unsigned char islittle:1; unsigned char fmt:3;
	    	      unsigned char zero:4; } b; };
    _DC_union dc; dc.c = 0;

    BinDataDesc::dump( c1, c2 );
    dc.b.fmt = isIeee() ? 0 : 1;
    dc.b.islittle = littleendian;
    c2 = dc.c;
}


void DataCharacteristics::toString( char* buf ) const
{
    if ( !buf ) return;

    BinDataDesc::toString( buf );

    FileMultiString fms( buf );
    fms += isIeee() ? "IEEE" : "IBMmf";
    fms += getYesNoString( littleendian );

    strcpy( buf, (const char*)fms );
}


#define mSwapChars1(b2) \
	c = *( p + idx2 ); \
        *( p + idx2 ) = *( p + idx2 + b2 ); \
        *( p + idx2 + b2 ) = c

static void doswap2( void* buf, int bufsz )
{
    register unsigned char c;
    register unsigned char* p = (unsigned char*)buf;
    register int idx2 = 0;
    for ( register int idx=0; idx<bufsz; idx++ )
    {
	mSwapChars1( 1 );
	idx2 += 2;
    }
}

void DataInterpreter<float>::swap2( void* b, int s ) const { doswap2(b,s); }
void DataInterpreter<int>::swap2( void* b, int s ) const { doswap2(b,s); }
void DataInterpreter<double>::swap2( void* b, int s ) const { doswap2(b,s); }


#define mSwapChars2(b1,b2) \
	c = *( p + idx2 + b1 ); \
	*( p + idx2 + b1 ) = *( p + idx2 + b2 ); \
	*( p + idx2 + b2 ) = c

static void doswap4( void* buf, int bufsz )
{
    register unsigned char c;
    register unsigned char* p = (unsigned char*)buf;
    register int idx2 = 0;
    for ( register int idx=0; idx<bufsz; idx++ )
    {
	mSwapChars1( 3 );
	mSwapChars2( 1, 2 );
	idx2 += 4;
    }
}

void DataInterpreter<float>::swap4( void* b, int s ) const { doswap4(b,s); }
void DataInterpreter<int>::swap4( void* b, int s ) const { doswap4(b,s); }
void DataInterpreter<double>::swap4( void* b, int s ) const { doswap4(b,s); }


static void doswap8( void* buf, int bufsz )
{
    register unsigned char c;
    register unsigned char* p = (unsigned char*)buf;
    register int idx2 = 0;
    for ( register int idx=0; idx<bufsz; idx++ )
    {
	mSwapChars1( 7 );
	mSwapChars2( 1, 6 );
	mSwapChars2( 2, 5 );
	mSwapChars2( 3, 4 );
	idx2 += 8;
    }
}
void DataInterpreter<float>::swap8( void* b, int s ) const { doswap8(b,s); }
void DataInterpreter<int>::swap8( void* b, int s ) const { doswap8(b,s); }
void DataInterpreter<double>::swap8( void* b, int s ) const { doswap8(b,s); }


#define mDefDIG(rettyp,typ) \
rettyp DataInterpreter<rettyp>::get##typ( const void* buf, int nr ) const \
{ return (rettyp)( *(((T##typ*)buf) + nr) ); }

mDefDIG(float,S1)
mDefDIG(float,S2)
mDefDIG(float,S4)
mDefDIG(float,U1)
mDefDIG(float,U2)
mDefDIG(float,U4)
mDefDIG(float,F)
mDefDIG(float,D)
mDefDIG(double,S1)
mDefDIG(double,S2)
mDefDIG(double,S4)
mDefDIG(double,U1)
mDefDIG(double,U2)
mDefDIG(double,U4)
mDefDIG(double,F)
mDefDIG(double,D)
mDefDIG(int,S1)
mDefDIG(int,S2)
mDefDIG(int,S4)
mDefDIG(int,U1)
mDefDIG(int,U2)
mDefDIG(int,U4)

#define mDefDIGF2I(rettyp,typ) \
rettyp DataInterpreter<rettyp>::get##typ( const void* buf, int nr ) const \
{ T##typ t = *(((T##typ*)buf) + nr); return (rettyp)(t > 0 ? t+.5:t-.5); }

mDefDIG(int,F)
mDefDIG(int,D)


#define mDefDIGswp(rettyp,typ) \
rettyp DataInterpreter<rettyp>::get##typ##swp( const void* buf, int nr ) const \
{ \
    T##typ t = *(((T##typ*)buf) + nr); \
    swap_bytes( &t, sizeof(T##typ) ); \
    return (rettyp)t; \
}

mDefDIGswp(float,S2)
mDefDIGswp(float,S4)
mDefDIGswp(float,U2)
mDefDIGswp(float,U4)
mDefDIGswp(float,F)
mDefDIGswp(float,D)
mDefDIGswp(double,S2)
mDefDIGswp(double,S4)
mDefDIGswp(double,U2)
mDefDIGswp(double,U4)
mDefDIGswp(double,F)
mDefDIGswp(double,D)
mDefDIGswp(int,S2)
mDefDIGswp(int,S4)
mDefDIGswp(int,U2)
mDefDIGswp(int,U4)


#define mDefDIGF2Iswp(rettyp,typ) \
rettyp DataInterpreter<rettyp>::get##typ##swp( const void* buf, int nr ) const \
{ \
    T##typ t = *(((T##typ*)buf) + nr); \
    swap_bytes( &t, sizeof(T##typ) ); \
    return (rettyp)(t > 0 ? t+.5:t-.5); \
}
mDefDIGF2Iswp(int,F)
mDefDIGF2Iswp(int,D)


#define mDefDIPS(inptyp,typ) \
void DataInterpreter<inptyp>::put##typ( void* buf, int nr, inptyp f ) const \
{ \
    *(((T##typ*)buf)+nr) = f > cM##typ ? cM##typ \
		  : ( f < -cM##typ ? -cM##typ : (T##typ)(f + (f<0?-.5:.5)) ); \
}

mDefDIPS(float,S1)
mDefDIPS(float,S2)
mDefDIPS(float,S4)
mDefDIPS(double,S1)
mDefDIPS(double,S2)
mDefDIPS(double,S4)

#define mDefDIPIS(inptyp,typ) \
void DataInterpreter<inptyp>::put##typ( void* buf, int nr, inptyp f ) const \
{ *(((T##typ*)buf)+nr) = f; }

#define mDefDIPISc(inptyp,typ) \
void DataInterpreter<inptyp>::put##typ( void* buf, int nr, inptyp f ) const \
{ \
    *(((T##typ*)buf)+nr) = f > cM##typ ? cM##typ \
			 : ( f < -cM##typ ? -cM##typ : (T##typ)f ); \
}

mDefDIPISc(int,S1)
mDefDIPISc(int,S2)
mDefDIPIS(int,S4)


#define mDefDIPU(inptyp,typ) \
void DataInterpreter<inptyp>::put##typ( void* buf, int nr, inptyp f ) const \
{ \
    *(((T##typ*)buf)+nr) = f > cM##typ ? cM##typ \
				       : (f < 0 ? 0 : (T##typ)(f + .5)); \
}

mDefDIPU(float,U1)
mDefDIPU(float,U2)
mDefDIPU(float,U4)
mDefDIPU(double,U1)
mDefDIPU(double,U2)
mDefDIPU(double,U4)


#define mDefDIPIU(inptyp,typ) \
void DataInterpreter<inptyp>::put##typ( void* buf, int nr, inptyp f ) const \
{ *(((T##typ*)buf)+nr) = f < 0 ? 0 : (T##typ)f; }

#define mDefDIPIUc(inptyp,typ) \
void DataInterpreter<inptyp>::put##typ( void* buf, int nr, inptyp f ) const \
{ *(((T##typ*)buf)+nr) = f > cM##typ ? cM##typ : (f < 0 ? 0 : (T##typ)f); }

mDefDIPIUc(int,U1)
mDefDIPIUc(int,U2)
mDefDIPIU(int,U4)


#define mDefDIPF(inptyp,typ) \
void DataInterpreter<inptyp>::put##typ( void* buf, int nr, inptyp f ) const \
{ *(((T##typ*)buf)+nr) = (T##typ)f; }

mDefDIPF(float,F)
mDefDIPF(float,D)
mDefDIPF(double,F)
mDefDIPF(double,D)
mDefDIPF(int,F)
mDefDIPF(int,D)


#define mDefDIPSswp(inptyp,typ) \
void DataInterpreter<inptyp>::put##typ##swp(void* buf,int nr,inptyp f) const \
{ \
    *(((T##typ*)buf)+nr) = f > cM##typ ? cM##typ \
		  : ( f < -cM##typ ? -cM##typ : (T##typ)(f + (f<0?-.5:.5)) ); \
    swap_bytes( ((T##typ*)buf)+nr, sizeof(T##typ) ); \
}

mDefDIPSswp(float,S2)
mDefDIPSswp(float,S4)
mDefDIPSswp(double,S2)
mDefDIPSswp(double,S4)

#define mDefDIPISswp(inptyp,typ) \
void DataInterpreter<inptyp>::put##typ##swp(void* buf,int nr,inptyp f) const \
{ \
    *(((T##typ*)buf)+nr) = (T##typ)f; \
    swap_bytes( ((T##typ*)buf)+nr, sizeof(T##typ) ); \
}

#define mDefDIPIScswp(inptyp,typ) \
void DataInterpreter<inptyp>::put##typ##swp(void* buf,int nr,inptyp f) const \
{ \
    *(((T##typ*)buf)+nr) = f > cM##typ ? cM##typ \
		  : ( f < -cM##typ ? -cM##typ : (T##typ)f ); \
    swap_bytes( ((T##typ*)buf)+nr, sizeof(T##typ) ); \
}

mDefDIPIScswp(int,S2)
mDefDIPISswp(int,S4)


#define mDefDIPUswp(inptyp,typ) \
void DataInterpreter<inptyp>::put##typ##swp(void* buf,int nr,inptyp f) const \
{ \
    *(((T##typ*)buf)+nr) = f > cM##typ ? cM##typ \
		  : (f < 0 ? 0 : (T##typ)(f + .5)); \
    swap_bytes( ((T##typ*)buf)+nr, sizeof(T##typ) ); \
}

mDefDIPUswp(float,U2)
mDefDIPUswp(float,U4)
mDefDIPUswp(double,U2)
mDefDIPUswp(double,U4)

#define mDefDIPUIswp(inptyp,typ) \
void DataInterpreter<inptyp>::put##typ##swp(void* buf,int nr,inptyp f) const \
{ \
    *(((T##typ*)buf)+nr) = f < 0 ? 0 : (T##typ)f; \
    swap_bytes( ((T##typ*)buf)+nr, sizeof(T##typ) ); \
}

#define mDefDIPUIcswp(inptyp,typ) \
void DataInterpreter<inptyp>::put##typ##swp(void* buf,int nr,inptyp f) const \
{ \
    *(((T##typ*)buf)+nr) = f > cM##typ ? cM##typ : (f < 0 ? 0 : (T##typ)f); \
    swap_bytes( ((T##typ*)buf)+nr, sizeof(T##typ) ); \
}

mDefDIPUIcswp(int,U2)
mDefDIPUIswp(int,U4)


#define mDefDIPFswp(inptyp,typ) \
void DataInterpreter<inptyp>::put##typ##swp(void* buf,int nr,inptyp f) const \
{ \
    *(((T##typ*)buf)+nr) = (T##typ)f; \
    swap_bytes( ((T##typ*)buf)+nr, sizeof(T##typ) ); \
}

mDefDIPFswp(float,F)
mDefDIPFswp(float,D)
mDefDIPFswp(double,F)
mDefDIPFswp(double,D)
mDefDIPFswp(int,F)
mDefDIPFswp(int,D)


#define mDefDIGIbmswp(rettyp,typ,fntyp) \
rettyp DataInterpreter<rettyp>::get##typ##Ibmswp(const void* buf,int nr) const \
{ \
     T##typ x = *( ((T##typ*)buf)+nr ); \
     swap_bytes( &x, sizeof(T##typ) ); \
     return (rettyp)IbmFormat::as##fntyp( &x ); \
}

#define mDefDIGIbm(rettyp,typ,fntyp) \
rettyp DataInterpreter<rettyp>::get##typ##Ibm( const void* buf, int nr ) const \
{ return (rettyp)IbmFormat::as##fntyp( ((T##typ*)buf)+nr ); }

mDefDIGIbm(float,S2,Short)
mDefDIGIbm(float,S4,Int)
mDefDIGIbm(float,F,Float)
mDefDIGIbm(double,S2,Short)
mDefDIGIbm(double,S4,Int)
mDefDIGIbm(double,F,Float)
mDefDIGIbm(int,S2,Short)
mDefDIGIbm(int,S4,Int)
mDefDIGIbm(int,F,Float)


#define mDefDIGIbmswp(rettyp,typ,fntyp) \
rettyp DataInterpreter<rettyp>::get##typ##Ibmswp(const void* buf,int nr) const \
{ \
     T##typ x = *( ((T##typ*)buf)+nr ); \
     swap_bytes( &x, sizeof(T##typ) ); \
     return (rettyp)IbmFormat::as##fntyp( &x ); \
}

mDefDIGIbmswp(float,S2,Short)
mDefDIGIbmswp(float,S4,Int)
mDefDIGIbmswp(float,F,Float)
mDefDIGIbmswp(double,S2,Short)
mDefDIGIbmswp(double,S4,Int)
mDefDIGIbmswp(double,F,Float)
mDefDIGIbmswp(int,S2,Short)
mDefDIGIbmswp(int,S4,Int)
mDefDIGIbmswp(int,F,Float)


#define mDefDIPSIbm(inptyp,typ,fntyp) \
void DataInterpreter<inptyp>::put##typ##Ibm(void* buf,int nr,inptyp f) const \
{ \
    IbmFormat::put##fntyp( f > cM##typ ? cM##typ : ( f < -cM##typ ? -cM##typ \
		: (T##typ)(f + (f<0?-.5:.5)) ), ((T##typ*)buf)+nr ); \
}

mDefDIPSIbm(float,S2,Short)
mDefDIPSIbm(float,S4,Int)
mDefDIPSIbm(double,S2,Short)
mDefDIPSIbm(double,S4,Int)
mDefDIPSIbm(int,S2,Short)
mDefDIPSIbm(int,S4,Int)


#define mDefDIPFIbm(inptyp,typ,fntyp) \
void DataInterpreter<inptyp>::put##typ##Ibm(void* buf,int nr,inptyp f) const \
{ IbmFormat::put##fntyp( f, ((T##typ*)buf)+nr ); }

mDefDIPFIbm(float,F,Float)
mDefDIPFIbm(double,F,Float)
mDefDIPFIbm(int,F,Float)


#define mDefDIPSIbmswp(inptyp,typ,fntyp) \
void DataInterpreter<inptyp>::put##typ##Ibm##swp(void* buf,int nr,inptyp f) \
const { \
    IbmFormat::put##fntyp( f > cM##typ ? cM##typ : ( f < -cM##typ ? -cM##typ \
		: (T##typ)(f + (f<0?-.5:.5)) ), ((T##typ*)buf)+nr ); \
    swap_bytes( ((T##typ*)buf)+nr, sizeof(T##typ) ); \
}

mDefDIPSIbmswp(float,S2,Short)
mDefDIPSIbmswp(float,S4,Int)
mDefDIPSIbmswp(double,S2,Short)
mDefDIPSIbmswp(double,S4,Int)
mDefDIPSIbmswp(int,S2,Short)
mDefDIPSIbmswp(int,S4,Int)


#define mDefDIPFIbmswp(inptyp,typ,fntyp) \
void DataInterpreter<inptyp>::put##typ##Ibm##swp(void* buf,int nr,inptyp f) \
const { \
    IbmFormat::put##fntyp( f, ((T##typ*)buf)+nr ); \
    swap_bytes( ((T##typ*)buf)+nr, sizeof(T##typ) ); \
}

mDefDIPFIbmswp(float,F,Float)
mDefDIPFIbmswp(double,F,Float)
mDefDIPFIbmswp(int,F,Float)


#define mTheType float
#include "i_datainterp.h"
#undef mTheType
#define mTheType double
#include "i_datainterp.h"
#undef mTheType
#define mTheType int
#include "i_datainterp.h"
