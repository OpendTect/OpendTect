/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Nov 2000
 * FUNCTION : Interpret data buffers
-*/

static const char* rcsID = "$Id: datainterp.cc,v 1.3 2001-02-22 08:22:05 bert Exp $";

#include "datainterp.h"
#include "datachar.h"
#include "ibmformat.h"
#include "separstr.h"

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
typedef signed long long TS8;
typedef unsigned long long TU8;

// But this is fundamental mathematics
const TS1 cMS1 = 127;
const TS2 cMS2 = 32767;
const TS4 cMS4 = 2147483647L;
const TS8 cMS8 = 9223372036854775807LL;
const TU1 cMU1 = 255;
const TU2 cMU2 = 65535;
const TU4 cMU4 = 4294967295UL;
const TU8 cMU8 = 18446744073709551615ULL;


union _DC_union
{
    unsigned short c;
    struct bits {

	unsigned char	bindesc:8;	// BinDataDesc part
	unsigned char	little:1;	// little endian == 1
	unsigned char	fmt:3;		// 0 == IEEE, 1 == IBM mainframe (SEG-Y)
					// 2 == SGI
	unsigned char	rest:4;		// Future? Subclasses?
   } b;
};


void DataCharacteristics::set( unsigned short c )
{
    _DC_union dc; dc.c = c;
    BinDataDesc::set( c );

    littleendian = dc.b.little;
    fmt = dc.b.fmt == 0 ? DataCharacteristics::Ieee : DataCharacteristics::Ibm;
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


unsigned short DataCharacteristics::dump() const
{
    _DC_union dc;

    dc.c = BinDataDesc::dump();
    dc.b.fmt = isIeee() ? 0 : 1;
    dc.b.little = littleendian;

    return dc.c;
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

void DataInterpreter<float>::swap2( void* buf, int bufsz ) const
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


#define mSwapChars2(b1,b2) \
	c = *( p + idx2 + b1 ); \
	*( p + idx2 + b1 ) = *( p + idx2 + b2 ); \
	*( p + idx2 + b2 ) = c

void DataInterpreter<float>::swap4( void* buf, int bufsz ) const
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


void DataInterpreter<float>::swap8( void* buf, int bufsz ) const
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


#define mDefDIG(rettyp,typ) \
rettyp DataInterpreter<rettyp>::get##typ( const void* buf, int nr ) const \
{ return (rettyp)( *(((T##typ*)buf) + nr) ); }

mDefDIG(float,S1)
mDefDIG(float,S2)
mDefDIG(float,S4)
mDefDIG(float,S8)
mDefDIG(float,U1)
mDefDIG(float,U2)
mDefDIG(float,U4)
mDefDIG(float,U8)
mDefDIG(float,F)
mDefDIG(float,D)

#define mDefDIGswp(rettyp,typ) \
rettyp DataInterpreter<rettyp>::get##typ##swp( const void* buf, int nr ) const \
{ \
    rettyp r = (rettyp)( *(((T##typ*)buf) + nr) ); \
    swap_bytes( &r, sizeof(rettyp) ); \
    return r; \
}

mDefDIGswp(float,S2)
mDefDIGswp(float,S4)
mDefDIGswp(float,S8)
mDefDIGswp(float,U2)
mDefDIGswp(float,U4)
mDefDIGswp(float,U8)
mDefDIGswp(float,F)
mDefDIGswp(float,D)


#define mDefDIPS(inptyp,typ) \
void DataInterpreter<inptyp>::put##typ( void* buf, int nr, inptyp f ) const \
{ \
    *(((T##typ*)buf)+nr) = f > cM##typ ? cM##typ \
		  : ( f < -cM##typ ? -cM##typ : (T##typ)(f + (f<0?-.5:.5)) ); \
}

mDefDIPS(float,S1)
mDefDIPS(float,S2)
mDefDIPS(float,S4)
mDefDIPS(float,S8)


#define mDefDIPU(inptyp,typ) \
void DataInterpreter<inptyp>::put##typ( void* buf, int nr, inptyp f ) const \
{ \
    *(((T##typ*)buf)+nr) = f > cM##typ ? cM##typ \
				       : (f < 0 ? 0 : (T##typ)(f + .5)); \
}

mDefDIPU(float,U1)
mDefDIPU(float,U2)
mDefDIPU(float,U4)
mDefDIPU(float,U8)


#define mDefDIPF(inptyp,typ) \
void DataInterpreter<inptyp>::put##typ( void* buf, int nr, inptyp f ) const \
{ *(((T##typ*)buf)+nr) = (T##typ)f; }

mDefDIPF(float,F)
mDefDIPF(float,D)


#define mDefDIPSswp(inptyp,typ) \
void DataInterpreter<inptyp>::put##typ##swp(void* buf,int nr,inptyp f) const \
{ \
    *(((T##typ*)buf)+nr) = f > cM##typ ? cM##typ \
		  : ( f < -cM##typ ? -cM##typ : (T##typ)(f + (f<0?-.5:.5)) ); \
    swap_bytes( ((T##typ*)buf)+nr, sizeof(T##typ) ); \
}

mDefDIPSswp(float,S2)
mDefDIPSswp(float,S4)
mDefDIPSswp(float,S8)


#define mDefDIPUswp(inptyp,typ) \
void DataInterpreter<inptyp>::put##typ##swp(void* buf,int nr,inptyp f) const \
{ \
    *(((T##typ*)buf)+nr) = f > cM##typ ? cM##typ \
		  : (f < 0 ? 0 : (T##typ)(f + .5)); \
    swap_bytes( ((T##typ*)buf)+nr, sizeof(T##typ) ); \
}

mDefDIPUswp(float,U2)
mDefDIPUswp(float,U4)
mDefDIPUswp(float,U8)


#define mDefDIPFswp(inptyp,typ) \
void DataInterpreter<inptyp>::put##typ##swp(void* buf,int nr,inptyp f) const \
{ \
    *(((T##typ*)buf)+nr) = (T##typ)f; \
    swap_bytes( ((T##typ*)buf)+nr, sizeof(T##typ) ); \
}

mDefDIPFswp(float,F)
mDefDIPFswp(float,D)


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


#define mDefDIPSIbm(inptyp,typ,fntyp) \
void DataInterpreter<inptyp>::put##typ##Ibm(void* buf,int nr,inptyp f) const \
{ \
    IbmFormat::put##fntyp( f > cM##typ ? cM##typ : ( f < -cM##typ ? -cM##typ \
		: (T##typ)(f + (f<0?-.5:.5)) ), ((T##typ*)buf)+nr ); \
}

mDefDIPSIbm(float,S2,Short)
mDefDIPSIbm(float,S4,Int)


#define mDefDIPFIbm(inptyp,typ,fntyp) \
void DataInterpreter<inptyp>::put##typ##Ibm(void* buf,int nr,inptyp f) const \
{ IbmFormat::put##fntyp( f, ((T##typ*)buf)+nr ); }

mDefDIPFIbm(float,F,Float)


#define mDefDIPSIbmswp(inptyp,typ,fntyp) \
void DataInterpreter<inptyp>::put##typ##Ibm##swp(void* buf,int nr,inptyp f) \
const { \
    IbmFormat::put##fntyp( f > cM##typ ? cM##typ : ( f < -cM##typ ? -cM##typ \
		: (T##typ)(f + (f<0?-.5:.5)) ), ((T##typ*)buf)+nr ); \
    swap_bytes( ((T##typ*)buf)+nr, sizeof(T##typ) ); \
}

mDefDIPSIbmswp(float,S2,Short)
mDefDIPSIbmswp(float,S4,Int)


#define mDefDIPFIbmswp(inptyp,typ,fntyp) \
void DataInterpreter<inptyp>::put##typ##Ibm##swp(void* buf,int nr,inptyp f) \
const { \
    IbmFormat::put##fntyp( f, ((T##typ*)buf)+nr ); \
    swap_bytes( ((T##typ*)buf)+nr, sizeof(T##typ) ); \
}

mDefDIPFIbmswp(float,F,Float)


#define mDICB(fntyp,fn) \
((DataInterpreter<float>::fntyp)(&DataInterpreter<float>::fn))

#define mDefGetPut(fnnm) \
{ \
    getfn = needswap ? mDICB(GetFn,get##fnnm##swp) : mDICB(GetFn,get##fnnm); \
    putfn = needswap ? mDICB(PutFn,put##fnnm##swp) : mDICB(PutFn,put##fnnm); \
}

#define mDefGetPutNoSwap(fnnm) \
{ getfn = mDICB(GetFn,get##fnnm); putfn = mDICB(PutFn,put##fnnm); }


void DataInterpreter<float>::set( const DataCharacteristics& dc, bool ignend )
{
    swpfn = dc.nrBytes() == BinDataDesc::N1 ? mDICB(SwapFn,swap0)
	: ( dc.nrBytes() == BinDataDesc::N8 ? mDICB(SwapFn,swap8)
	: ( dc.nrBytes() == BinDataDesc::N4 ? mDICB(SwapFn,swap4)
					    : mDICB(SwapFn,swap2) ) );

    getfn = mDICB(GetFn,get0);
    putfn = mDICB(PutFn,put0);
    needswap = !ignend && dc.needSwap();

    if ( !dc.isInteger() )
    {
	if ( !dc.isIeee() )
	{
	    if ( dc.nrBytes() == BinDataDesc::N4 )
		mDefGetPut(FIbm)
	}
	else
	{
	    if ( dc.nrBytes() == BinDataDesc::N4 )
		mDefGetPut(F)
	    else if ( dc.nrBytes() == BinDataDesc::N8 )
		mDefGetPut(D)
	}
    }
    else
    {
	if ( dc.isSigned() )
	{
	    if ( !dc.isIeee() )
	    {
		switch ( dc.nrBytes() )
		{
		case BinDataDesc::N1: mDefGetPutNoSwap(S1)	break;
		case BinDataDesc::N2: mDefGetPut(S2Ibm)		break;
		case BinDataDesc::N4: mDefGetPut(S4Ibm)		break;
		}
	    }
	    else
	    {
		switch ( dc.nrBytes() )
		{
		case BinDataDesc::N1: mDefGetPutNoSwap(S1)	break;
		case BinDataDesc::N2: mDefGetPut(S2)		break;
		case BinDataDesc::N4: mDefGetPut(S4)		break;
		case BinDataDesc::N8: mDefGetPut(S8)		break;
		}
	    }
	}
	else if ( dc.isIeee() )
	{
	    switch ( dc.nrBytes() )
	    {
	    case BinDataDesc::N1: mDefGetPutNoSwap(U1)		break;
	    case BinDataDesc::N2: mDefGetPut(U2)		break;
	    case BinDataDesc::N4: mDefGetPut(U4)		break;
	    case BinDataDesc::N8: mDefGetPut(U8)		break;
	    }
	}
    }
}


DataInterpreter<float>::DataInterpreter<float>( const DataCharacteristics& dc,
						bool ignend )
{
    set( dc, ignend );
}


DataInterpreter<float>::DataInterpreter<float>( const DataInterpreter& di )
	: swpfn(di.swpfn)
	, getfn(di.getfn)
	, putfn(di.putfn)
	, needswap(di.needswap)
{
}


DataInterpreter<float>& DataInterpreter<float>::operator=(
			    const DataInterpreter<float>& di )
{
    if ( &di != this )
    {
	swpfn = di.swpfn;
	getfn = di.getfn;
	putfn = di.putfn;
	needswap = di.needswap;
    }
    return *this;
}


int DataInterpreter<float>::nrBytes() const
{
    return swpfn == &DataInterpreter<float>::swap2 ?	2
	: (swpfn == &DataInterpreter<float>::swap4 ?	4
	: (swpfn == &DataInterpreter<float>::swap8 ?	8
	:						1  ));
}


#define mSet(nb,isint,frmt,iss,swpd) \
	dc = DataCharacteristics( isint, iss, (BinDataDesc::ByteCount)nb, \
		DataCharacteristics::frmt, __islittle__ != swpd );

DataCharacteristics DataInterpreter<float>::dataChar() const
{
    DataCharacteristics dc;
    switch ( nrBytes() )
    {

    case 2: {
	if ( getfn == &DataInterpreter<float>::getS2 )
	    mSet(2,true,Ieee,true,false)
	else if ( getfn == &DataInterpreter<float>::getU2 )
	    mSet(2,true,Ieee,false,false)
	else if ( getfn == &DataInterpreter<float>::getS2Ibm )
	    mSet(2,true,Ibm,true,false)
	else if ( getfn == &DataInterpreter<float>::getS2swp )
	    mSet(2,true,Ieee,true,true)
	else if ( getfn == &DataInterpreter<float>::getU2swp )
	    mSet(2,true,Ieee,false,true)
	else if ( getfn == &DataInterpreter<float>::getS2Ibmswp )
	    mSet(2,true,Ibm,true,true)
    }

    case 4:
    {
	if ( getfn == &DataInterpreter<float>::getS4 )
	    mSet(4,true,Ieee,true,false)
	else if ( getfn == &DataInterpreter<float>::getU4 )
	    mSet(4,true,Ieee,false,false)
	else if ( getfn == &DataInterpreter<float>::getF )
	    mSet(4,false,Ieee,true,false)
	else if ( getfn == &DataInterpreter<float>::getS4Ibm )
	    mSet(4,true,Ibm,true,false)
	else if ( getfn == &DataInterpreter<float>::getFIbm )
	    mSet(4,false,Ibm,true,false)
	else if ( getfn == &DataInterpreter<float>::getS4swp )
	    mSet(4,true,Ieee,true,true)
	else if ( getfn == &DataInterpreter<float>::getU4swp )
	    mSet(4,true,Ieee,false,true)
	else if ( getfn == &DataInterpreter<float>::getFswp )
	    mSet(4,false,Ieee,true,true)
	else if ( getfn == &DataInterpreter<float>::getS4Ibmswp )
	    mSet(4,true,Ibm,true,true)
	else if ( getfn == &DataInterpreter<float>::getFIbmswp )
	    mSet(4,false,Ibm,true,true)
    }

    case 8:
    {
	if ( getfn == &DataInterpreter<float>::getS8 )
	    mSet(8,true,Ieee,true,false)
	else if ( getfn == &DataInterpreter<float>::getU8 )
	    mSet(8,true,Ieee,false,false)
	else if ( getfn == &DataInterpreter<float>::getD )
	    mSet(8,false,Ieee,true,false)
	else if ( getfn == &DataInterpreter<float>::getS8swp )
	    mSet(8,true,Ieee,true,true)
	else if ( getfn == &DataInterpreter<float>::getU8swp )
	    mSet(8,true,Ieee,false,true)
	else if ( getfn == &DataInterpreter<float>::getDswp )
	    mSet(8,false,Ieee,true,true)
    }

    default:
    {
	if ( getfn == &DataInterpreter<float>::getS1 )
	    mSet(1,true,Ieee,true,false)
	else if ( getfn == &DataInterpreter<float>::getU1 )
	    mSet(1,true,Ieee,false,false)
    }

    }

    return dc;
}
