/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Nov 2000
 * FUNCTION : Interpret data buffers
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "datainterp.h"

#include "datachar.h"
#include "iopar.h"
#include "ibmformat.h"
#include "separstr.h"
#include "strmoper.h"

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
        "9 - 64 bit signed",
	0 };

// sys/types.h is a bit of a mess, so ...
typedef signed char TS1;
typedef signed short TS2;
typedef unsigned char TU1;
typedef unsigned short TU2;
typedef float TF;
typedef double TD;

// This may be a problem on 64 bit machines. Fix when appropriate
typedef signed int TS4;
typedef unsigned int TU4;
typedef od_int64 TS8;

// But this is fundamental mathematics
const TS1 cMS1 = 127;
const TS2 cMS2 = 32767;
const TS4 cMS4 = 2147483647L;
const TS8 cMS8 = 9223372036854775807LL;
const TU1 cMU1 = 255;
const TU2 cMU2 = 65535;
const TU4 cMU4 = 4294967295UL;


void DataCharacteristics::set( unsigned char c1, unsigned char c2 )
{
    // remember that the 'zero' member is always zero.
    littleendian_ = (c2 & 0x80) || (c2 & 0x01);
    setFrom( c1, littleendian_ );

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
    fmt_ = (DataCharacteristics::Format)f;
};


void DataCharacteristics::set( const char* s )
{
    BinDataDesc::set( s );
    FileMultiString fms( s );
    const int sz = fms.size();
    if ( sz > 3 )
	fmt_ = matchStringCI( "ibm", fms[1] ) ? DataCharacteristics::Ibm
					     : DataCharacteristics::Ieee;
    if ( sz > 4 )
	littleendian_ = toBool( fms[4], true );
}


DataCharacteristics::DataCharacteristics( DataCharacteristics::UserType ut )
	: BinDataDesc( ut!=F32 && ut!=F64, ut>UI32 || (int)ut % 2)
	, fmt_(Ieee)
	, littleendian_(__islittle__)
{
    if ( ut == Auto )
	*this = DataCharacteristics();
    else
	nrbytes_ = (BinDataDesc::ByteCount)
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
    dc.b.islittle = littleendian_;
    c2 = dc.c;
}


void DataCharacteristics::toString( char* buf ) const
{
    if ( !buf ) return;

    BinDataDesc::toString( buf );

    FileMultiString fms( buf );
    fms += isIeee() ? "IEEE" : "IBMmf";
    fms += getYesNoString( littleendian_ );

    strcpy( buf, (const char*)fms );
}


DataCharacteristics::UserType DataCharacteristics::userType() const
{
    switch ( nrBytes() )
    {
    case N1: return isSigned() ? SI8 : UI8;
    case N2: return isSigned() ? SI16 : UI16;
    case N4: return isInteger() ? (isSigned() ? SI32 : UI32) : F32;
    case N8: return isInteger() ? SI64 : F64;
    }
    return Auto;
}


#define mSwapChars1(b2) \
	c = *( p + idx2 ); \
        *( p + idx2 ) = *( p + idx2 + b2 ); \
        *( p + idx2 + b2 ) = c

static void doswap2( void* buf, od_int64 bufsz )
{
    register unsigned char c;
    register unsigned char* p = (unsigned char*)buf;
    register od_int64 idx2 = 0;
    for ( register od_int64 idx=0; idx<bufsz; idx++ )
    {
	mSwapChars1( 1 );
	idx2 += 2;
    }
}


#define mSwapChars2(b1,b2) \
	c = *( p + idx2 + b1 ); \
	*( p + idx2 + b1 ) = *( p + idx2 + b2 ); \
	*( p + idx2 + b2 ) = c

static void doswap4( void* buf, od_int64 bufsz )
{
    register unsigned char c;
    register unsigned char* p = (unsigned char*)buf;
    register od_int64 idx2 = 0;
    for ( register od_int64 idx=0; idx<bufsz; idx++ )
    {
	mSwapChars1( 3 );
	mSwapChars2( 1, 2 );
	idx2 += 4;
    }
}


static void doswap8( void* buf, od_int64 bufsz )
{
    register unsigned char c;
    register unsigned char* p = (unsigned char*)buf;
    register od_int64 idx2 = 0;
    for ( register od_int64 idx=0; idx<bufsz; idx++ )
    {
	mSwapChars1( 7 );
	mSwapChars2( 1, 6 );
	mSwapChars2( 2, 5 );
	mSwapChars2( 3, 4 );
	idx2 += 8;
    }
}


#define mDefSwapFn(typ,N) \
template <> \
void DataInterpreter<typ>::swap##N( void* b, od_int64 s ) const \
{ doswap##N(b,s); }

mDefSwapFn(float,2) mDefSwapFn(float,4) mDefSwapFn(float,8)
mDefSwapFn(int,2) mDefSwapFn(int,4) mDefSwapFn(int,8)
mDefSwapFn(double,2) mDefSwapFn(double,4) mDefSwapFn(double,8)
mDefSwapFn(od_int64,2) mDefSwapFn(od_int64,4) mDefSwapFn(od_int64,8)


#define mDefDIG(rettyp,typ) \
template <> \
rettyp DataInterpreter<rettyp>::get##typ( const void* buf, od_int64 nr ) const \
{ return (rettyp)( *(((T##typ*)buf) + nr) ); }

mDefDIG(float,S1)
mDefDIG(float,S2)
mDefDIG(float,S4)
mDefDIG(float,S8)
mDefDIG(float,U1)
mDefDIG(float,U2)
mDefDIG(float,U4)
mDefDIG(float,F)
mDefDIG(float,D)
mDefDIG(double,S1)
mDefDIG(double,S2)
mDefDIG(double,S4)
mDefDIG(double,S8)
mDefDIG(double,U1)
mDefDIG(double,U2)
mDefDIG(double,U4)
mDefDIG(double,F)
mDefDIG(double,D)
mDefDIG(int,S1)
mDefDIG(int,S2)
mDefDIG(int,S4)
mDefDIG(int,S8)
mDefDIG(int,U1)
mDefDIG(int,U2)
mDefDIG(int,U4)
mDefDIG(od_int64,S1)
mDefDIG(od_int64,S2)
mDefDIG(od_int64,S4)
mDefDIG(od_int64,S8)
mDefDIG(od_int64,U1)
mDefDIG(od_int64,U2)
mDefDIG(od_int64,U4)

#define mDefDIGF2I(rettyp,typ) \
template <> \
rettyp DataInterpreter<rettyp>::get##typ( const void* buf, od_int64 nr ) const \
{ T##typ t = *(((T##typ*)buf) + nr); return (rettyp)(t > 0 ? t+.5:t-.5); }

mDefDIGF2I(int,F)
mDefDIGF2I(int,D)
mDefDIGF2I(od_int64,F)
mDefDIGF2I(od_int64,D)


#define mDefDIGswp(rettyp,typ) \
template <> \
rettyp DataInterpreter<rettyp>::get##typ##swp( const void* buf, \
					       od_int64 nr ) const \
{ \
    T##typ t = *(((T##typ*)buf) + nr); \
    SwapBytes( &t, sizeof(T##typ) ); \
    return (rettyp)t; \
}

mDefDIGswp(float,S2)
mDefDIGswp(float,S4)
mDefDIGswp(float,S8)
mDefDIGswp(float,U2)
mDefDIGswp(float,U4)
mDefDIGswp(float,F)
mDefDIGswp(float,D)
mDefDIGswp(double,S2)
mDefDIGswp(double,S4)
mDefDIGswp(double,S8)
mDefDIGswp(double,U2)
mDefDIGswp(double,U4)
mDefDIGswp(double,F)
mDefDIGswp(double,D)
mDefDIGswp(int,S2)
mDefDIGswp(int,S4)
mDefDIGswp(int,S8)
mDefDIGswp(int,U2)
mDefDIGswp(int,U4)
mDefDIGswp(od_int64,S2)
mDefDIGswp(od_int64,S4)
mDefDIGswp(od_int64,S8)
mDefDIGswp(od_int64,U2)
mDefDIGswp(od_int64,U4)


#define mDefDIGF2Iswp(rettyp,typ) \
template <> \
rettyp DataInterpreter<rettyp>::get##typ##swp( const void* buf, \
					       od_int64 nr ) const \
{ \
    T##typ t = *(((T##typ*)buf) + nr); \
    SwapBytes( &t, sizeof(T##typ) ); \
    return (rettyp)(t > 0 ? t+.5:t-.5); \
}
mDefDIGF2Iswp(int,F)
mDefDIGF2Iswp(int,D)
mDefDIGF2Iswp(od_int64,F)
mDefDIGF2Iswp(od_int64,D)


#define mDefDIPS(inptyp,typ) \
template <> \
void DataInterpreter<inptyp>::put##typ( void* buf,od_int64 nr,inptyp f) const \
{ \
    *(((T##typ*)buf)+nr) = f > cM##typ ? cM##typ \
		  : ( f < -cM##typ ? -cM##typ : (T##typ)(f + (f<0?-.5:.5)) ); \
}

mDefDIPS(float,S1)
mDefDIPS(float,S2)
mDefDIPS(float,S4)
mDefDIPS(float,S8)
mDefDIPS(double,S1)
mDefDIPS(double,S2)
mDefDIPS(double,S4)
mDefDIPS(double,S8)

#define mDefDIPIS(inptyp,typ) \
template <> \
void DataInterpreter<inptyp>::put##typ( void* buf, od_int64 nr,inptyp f) const \
{ *(((T##typ*)buf)+nr) = f; }

#define mDefDIPISc(inptyp,typ) \
template <> \
void DataInterpreter<inptyp>::put##typ( void* buf, od_int64 nr,inptyp f) const \
{ \
    *(((T##typ*)buf)+nr) = f > cM##typ ? cM##typ \
			 : ( f < -cM##typ ? -cM##typ : (T##typ)f ); \
}

mDefDIPISc(int,S1)
mDefDIPISc(int,S2)
mDefDIPIS(int,S4)
mDefDIPIS(int,S8)
mDefDIPISc(od_int64,S1)
mDefDIPISc(od_int64,S2)
mDefDIPISc(od_int64,S4)
mDefDIPIS(od_int64,S8)


#define mDefDIPU(inptyp,typ) \
template <> \
void DataInterpreter<inptyp>::put##typ( void* buf, od_int64 nr,inptyp f) const \
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
template <> \
void DataInterpreter<inptyp>::put##typ( void* buf, od_int64 nr,inptyp f) const \
{ *(((T##typ*)buf)+nr) = f < 0 ? 0 : (T##typ)f; }

#define mDefDIPIUc(inptyp,typ) \
template <> \
void DataInterpreter<inptyp>::put##typ( void* buf, od_int64 nr, \
					    inptyp f) const \
{ *(((T##typ*)buf)+nr) = f > cM##typ ? cM##typ : (f < 0 ? 0 : (T##typ)f); }

mDefDIPIUc(int,U1)
mDefDIPIUc(int,U2)
mDefDIPIU(int,U4)
mDefDIPIUc(od_int64,U1)
mDefDIPIUc(od_int64,U2)
mDefDIPIUc(od_int64,U4)


#define mDefDIPF(inptyp,typ) \
template <> \
void DataInterpreter<inptyp>::put##typ( void* buf, od_int64 nr, \
       				    	inptyp f ) const \
{ *(((T##typ*)buf)+nr) = (T##typ)f; }

mDefDIPF(float,F)
mDefDIPF(float,D)
mDefDIPF(double,F)
mDefDIPF(double,D)
mDefDIPF(int,F)
mDefDIPF(int,D)
mDefDIPF(od_int64,F)
mDefDIPF(od_int64,D)


#define mDefDIPSswp(inptyp,typ) \
template <> \
void DataInterpreter<inptyp>::put##typ##swp(void* buf,od_int64 nr, \
					    inptyp f) const \
{ \
    *(((T##typ*)buf)+nr) = f > cM##typ ? cM##typ \
		  : ( f < -cM##typ ? -cM##typ : (T##typ)(f + (f<0?-.5:.5)) ); \
    SwapBytes( ((T##typ*)buf)+nr, sizeof(T##typ) ); \
}

mDefDIPSswp(float,S2)
mDefDIPSswp(float,S4)
mDefDIPSswp(float,S8)
mDefDIPSswp(double,S2)
mDefDIPSswp(double,S4)
mDefDIPSswp(double,S8)

#define mDefDIPISswp(inptyp,typ) \
template <> \
void DataInterpreter<inptyp>::put##typ##swp(void* buf,od_int64 nr, \
					    inptyp f) const \
{ \
    *(((T##typ*)buf)+nr) = (T##typ)f; \
    SwapBytes( ((T##typ*)buf)+nr, sizeof(T##typ) ); \
}

#define mDefDIPIScswp(inptyp,typ) \
template <> \
void DataInterpreter<inptyp>::put##typ##swp(void* buf,od_int64 nr, \
					    inptyp f) const \
{ \
    *(((T##typ*)buf)+nr) = f > cM##typ ? cM##typ \
		  : ( f < -cM##typ ? -cM##typ : (T##typ)f ); \
    SwapBytes( ((T##typ*)buf)+nr, sizeof(T##typ) ); \
}

mDefDIPIScswp(int,S2)
mDefDIPISswp(int,S4)
mDefDIPISswp(int,S8)
mDefDIPIScswp(od_int64,S2)
mDefDIPIScswp(od_int64,S4)
mDefDIPISswp(od_int64,S8)


#define mDefDIPUswp(inptyp,typ) \
template <> \
void DataInterpreter<inptyp>::put##typ##swp(void* buf,od_int64 nr, \
					    inptyp f) const \
{ \
    *(((T##typ*)buf)+nr) = f > cM##typ ? cM##typ \
		  : (f < 0 ? 0 : (T##typ)(f + .5)); \
    SwapBytes( ((T##typ*)buf)+nr, sizeof(T##typ) ); \
}

mDefDIPUswp(float,U2)
mDefDIPUswp(float,U4)
mDefDIPUswp(double,U2)
mDefDIPUswp(double,U4)

#define mDefDIPUIswp(inptyp,typ) \
template <> \
void DataInterpreter<inptyp>::put##typ##swp(void* buf,od_int64 nr, \
					    inptyp f) const \
{ \
    *(((T##typ*)buf)+nr) = f < 0 ? 0 : (T##typ)f; \
    SwapBytes( ((T##typ*)buf)+nr, sizeof(T##typ) ); \
}

#define mDefDIPUIcswp(inptyp,typ) \
template <> \
void DataInterpreter<inptyp>::put##typ##swp(void* buf, \
					    od_int64 nr,inptyp f) const \
{ \
    *(((T##typ*)buf)+nr) = f > cM##typ ? cM##typ : (f < 0 ? 0 : (T##typ)f); \
    SwapBytes( ((T##typ*)buf)+nr, sizeof(T##typ) ); \
}

mDefDIPUIcswp(int,U2)
mDefDIPUIswp(int,U4)
mDefDIPUIcswp(od_int64,U2)
mDefDIPUIcswp(od_int64,U4)


#define mDefDIPFswp(inptyp,typ) \
template <> \
void DataInterpreter<inptyp>::put##typ##swp(void* buf, \
					    od_int64 nr,inptyp f) const \
{ \
    *(((T##typ*)buf)+nr) = (T##typ)f; \
    SwapBytes( ((T##typ*)buf)+nr, sizeof(T##typ) ); \
}

mDefDIPFswp(float,F)
mDefDIPFswp(float,D)
mDefDIPFswp(double,F)
mDefDIPFswp(double,D)
mDefDIPFswp(int,F)
mDefDIPFswp(int,D)
mDefDIPFswp(od_int64,F)
mDefDIPFswp(od_int64,D)


#define mDefDIGIbmswp(rettyp,typ,fntyp) \
template <> \
rettyp DataInterpreter<rettyp>::get##typ##Ibmswp(const void* buf, \
					    od_int64 nr) const \
{ \
     T##typ x = *( ((T##typ*)buf)+nr ); \
     SwapBytes( &x, sizeof(T##typ) ); \
     return (rettyp)IbmFormat::as##fntyp( &x ); \
}

#define mDefDIGIbm(rettyp,typ,fntyp) \
template <> \
rettyp DataInterpreter<rettyp>::get##typ##Ibm( const void* buf, \
       				    	od_int64 nr ) const \
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
mDefDIGIbm(od_int64,S2,Short)
mDefDIGIbm(od_int64,S4,Int)
mDefDIGIbm(od_int64,F,Float)


#define mDefDIGIbmswp(rettyp,typ,fntyp) \
template <> \
rettyp DataInterpreter<rettyp>::get##typ##Ibmswp(const void* buf, \
					    od_int64 nr) const \
{ \
     T##typ x = *( ((T##typ*)buf)+nr ); \
     SwapBytes( &x, sizeof(T##typ) ); \
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
mDefDIGIbmswp(od_int64,S2,Short)
mDefDIGIbmswp(od_int64,S4,Int)
mDefDIGIbmswp(od_int64,F,Float)


#define mDefDIPSIbm(inptyp,typ,fntyp) \
template <> \
void DataInterpreter<inptyp>::put##typ##Ibm(void* buf,od_int64 nr, \
					    inptyp f) const \
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
mDefDIPSIbm(od_int64,S2,Short)
mDefDIPSIbm(od_int64,S4,Int)


#define mDefDIPFIbm(inptyp) \
template <> \
void DataInterpreter<inptyp>::putFIbm(void* buf,od_int64 nr,\
						 inptyp f) const \
{ IbmFormat::putFloat( (float) f, ((TF*)buf)+nr ); }

mDefDIPFIbm(float)
mDefDIPFIbm(double)
mDefDIPFIbm(int)
mDefDIPFIbm(od_int64)


#define mDefDIPSIbmswp(inptyp,typ,fntyp) \
template <> \
void DataInterpreter<inptyp>::put##typ##Ibm##swp(void* buf,\
						 od_int64 nr,inptyp f) \
const { \
    IbmFormat::put##fntyp( f > cM##typ ? cM##typ : ( f < -cM##typ ? -cM##typ \
		: (T##typ)(f + (f<0?-.5:.5)) ), ((T##typ*)buf)+nr ); \
    SwapBytes( ((T##typ*)buf)+nr, sizeof(T##typ) ); \
}

mDefDIPSIbmswp(float,S2,Short)
mDefDIPSIbmswp(float,S4,Int)
mDefDIPSIbmswp(double,S2,Short)
mDefDIPSIbmswp(double,S4,Int)
mDefDIPSIbmswp(int,S2,Short)
mDefDIPSIbmswp(int,S4,Int)
mDefDIPSIbmswp(od_int64,S2,Short)
mDefDIPSIbmswp(od_int64,S4,Int)


#define mDefDIPFIbmswp(inptyp) \
template <> \
void DataInterpreter<inptyp>::putFIbmswp(void* buf,od_int64 nr,\
						 inptyp f) \
const { \
    IbmFormat::putFloat( (float) f, ((TF*)buf)+nr ); \
    SwapBytes( ((TF*)buf)+nr, sizeof(TF) ); \
}

mDefDIPFIbmswp(float)
mDefDIPFIbmswp(double)
mDefDIPFIbmswp(int)
mDefDIPFIbmswp(od_int64)


#define mTheType float
#include "i_datainterp.h"
#undef mTheType
#define mTheType double
#include "i_datainterp.h"
#undef mTheType
#define mTheType int
#include "i_datainterp.h"
#undef mTheType
#define mTheType od_int64
#include "i_datainterp.h"


#define mImpl( type ) \
template <> \
type DataInterpreter<type>::get( std::istream& strm ) const \
{ \
    char buf[16]; \
    StrmOper::readBlock( strm, buf, nrBytes() ); \
    return get( buf, 0 ); \
} \
 \
template <> \
bool DataInterpreter<type>::get( const DataInterpreter<type>* di, \
				 std::istream& strm, type& res ) \
{ \
    if ( di ) \
    { \
	char buf[16]; \
        if ( !StrmOper::readBlock( strm, buf, di->nrBytes() ) ) \
	    return false;\
	res = di->get( buf, 0 ); \
	return true; \
    } \
    \
    return StrmOper::readBlock( strm, &res, sizeof(type) ); \
} \
 \
template <> \
type DataInterpreter<type>::get( const DataInterpreter<type>* di, \
std::istream& strm ) \
{ \
type val; \
get( di, strm, val ); \
return val; \
} \
template <> DataInterpreter<type>* \
DataInterpreter<type>::create( \
			const DataCharacteristics& dchar, \
			bool alsoifequal ) \
{ \
    type dummy; \
    const DataCharacteristics owntype( dummy ); \
    return !alsoifequal && dchar==owntype \
	? 0 \
	: new DataInterpreter<type>( dchar ); \
} \
 \
 \
template <> DataInterpreter<type>* \
DataInterpreter<type>::create( const char* str, bool alsoifequal ) \
{ \
    DataCharacteristics writtentype; \
    writtentype.set( str ); \
 \
    return create( writtentype, alsoifequal ); \
} \
 \
 \
template <> DataInterpreter<type>* \
DataInterpreter<type>::create( const IOPar& par,  const char* key, \
			       bool alsoifequal ) \
{ \
    const char* dc = par.find( key ); \
    return dc ? create( dc, alsoifequal ) : 0; \
}


mImpl( int )
mImpl( od_int64 )
mImpl( float )
mImpl( double )


