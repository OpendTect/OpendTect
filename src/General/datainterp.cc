/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Nov 2000
 * FUNCTION : Interpret data buffers
-*/

static const char* rcsID = "$Id: datainterp.cc,v 1.1 2001-02-13 17:48:39 bert Exp $";

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
    unsigned char c;
    struct bits {
	unsigned char	bytepow:3;	// nrbytes_per_sample == 2^bytepow
	unsigned char	type:1;		// integer == 1, floating point == 0
	unsigned char	issigned:1;	// signed data == 1
	unsigned char	little:1;	// little endian == 1
	unsigned char	fmt:2;		// 0 == IEEE, 1 == IBM mainframe (SEG-Y)
					// 2 == SGI
   } b;
};


void DataCharacteristics::set( unsigned char c )
{
    _DC_union dc; dc.c = c;

    nrbytes = 1;
    while( dc.b.bytepow ) { dc.b.bytepow--; nrbytes *= 2; }
    type = dc.b.type ? DataCharacteristics::Integer: DataCharacteristics::Float;
    issigned = dc.b.issigned;
    littleendian = dc.b.little;
    fmt = dc.b.fmt == 0 ? DataCharacteristics::Ieee : DataCharacteristics::Ibm;
};


void DataCharacteristics::set( const char* s )
{
    FileMultiString fms( s );
    const int sz = fms.size();
    if ( sz > 0 )
	type = *fms[0] == 'F' ? DataCharacteristics::Float
			      : DataCharacteristics::Integer;
    if ( sz > 1 )
	fmt = matchStringCI( "ibm", fms[1] ) ? DataCharacteristics::Ibm
					     : DataCharacteristics::Ieee;
    if ( sz > 2 )
    {
	int nrb = atoi( fms[2] );
	if ( nrb > 0 && nrb < 9 ) nrbytes = snappedSize( type, nrb );
    }
    if ( sz > 3 )
	issigned = yesNoFromString( fms[3] );
    if ( sz > 4 )
	littleendian = yesNoFromString( fms[4] );
}


unsigned char DataCharacteristics::dump() const
{
    _DC_union dc;
    dc.b.bytepow = 0; int nb = nrbytes;
    while ( nb > 1 ) { dc.b.bytepow++; nb /= 2; }
    dc.b.type = isInt() ? 1 : 0;
    dc.b.issigned = issigned ? 1 : 0;
    dc.b.fmt = isIeee() ? 0 : 1;

    return dc.c;
}


BufferString DataCharacteristics::toString() const
{
    FileMultiString fms( isInt() ? "Integer" : "Float" );
    fms += isIeee() ? "IEEE" : "IBMmainframe";
    fms += nrbytes;
    fms += getYesNoString( issigned );
    fms += getYesNoString( littleendian );

    return BufferString( (const char*)fms );
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
    swpfn = dc.nrbytes < 2 ?  mDICB(SwapFn,swap0)
	: ( dc.nrbytes > 4 ?  mDICB(SwapFn,swap8)
	: ( dc.nrbytes == 4 ? mDICB(SwapFn,swap4)
			    : mDICB(SwapFn,swap2) ) );

    getfn = mDICB(GetFn,get0);
    putfn = mDICB(PutFn,put0);
    needswap = !ignend && dc.needSwap();

    if ( !dc.isInt() )
    {
	if ( !dc.isIeee() )
	{
	    if ( dc.nrbytes == 4 )
		mDefGetPut(FIbm)
	}
	else
	{
	    if ( dc.nrbytes == 4 )
		mDefGetPut(F)
	    else if ( dc.nrbytes == 8 )
		mDefGetPut(D)
	}
    }
    else
    {
	if ( dc.issigned )
	{
	    if ( !dc.isIeee() )
	    {
		switch ( dc.nrbytes )
		{
		case 1: mDefGetPutNoSwap(S1)	break;
		case 2: mDefGetPut(S2Ibm)	break;
		case 4: mDefGetPut(S4Ibm)	break;
		}
	    }
	    else
	    {
		switch ( dc.nrbytes )
		{
		case 1: mDefGetPutNoSwap(S1)	break;
		case 2: mDefGetPut(S2)		break;
		case 4: mDefGetPut(S4)		break;
		case 8: mDefGetPut(S8)		break;
		}
	    }
	}
	else if ( dc.isIeee() )
	{
	    switch ( dc.nrbytes )
	    {
	    case 1: mDefGetPutNoSwap(U1)	break;
	    case 2: mDefGetPut(U2)		break;
	    case 4: mDefGetPut(U4)		break;
	    case 8: mDefGetPut(U8)		break;
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


#define mSet(typ,frt,iss,swpd) { \
	dc.type = DataCharacteristics::typ; dc.fmt = DataCharacteristics::frt; \
	dc.issigned = iss; dc.littleendian = __islittle__ != swpd; }

DataCharacteristics DataInterpreter<float>::dataChar() const
{
    DataCharacteristics dc;
    dc.nrbytes = nrBytes();

    switch ( dc.nrbytes )
    {

    case 2: {
	if ( getfn == &DataInterpreter<float>::getS2 )
	    mSet(Integer,Ieee,true,false)
	else if ( getfn == &DataInterpreter<float>::getU2 )
	    mSet(Integer,Ieee,false,false)
	else if ( getfn == &DataInterpreter<float>::getS2Ibm )
	    mSet(Integer,Ibm,true,false)
	else if ( getfn == &DataInterpreter<float>::getS2swp )
	    mSet(Integer,Ieee,true,true)
	else if ( getfn == &DataInterpreter<float>::getU2swp )
	    mSet(Integer,Ieee,false,true)
	else if ( getfn == &DataInterpreter<float>::getS2Ibmswp )
	    mSet(Integer,Ibm,true,true)
    }

    case 4:
    {
	if ( getfn == &DataInterpreter<float>::getS4 )
	    mSet(Integer,Ieee,true,false)
	else if ( getfn == &DataInterpreter<float>::getU4 )
	    mSet(Integer,Ieee,false,false)
	else if ( getfn == &DataInterpreter<float>::getF )
	    mSet(Float,Ieee,true,false)
	else if ( getfn == &DataInterpreter<float>::getS4Ibm )
	    mSet(Integer,Ibm,true,false)
	else if ( getfn == &DataInterpreter<float>::getFIbm )
	    mSet(Float,Ibm,true,false)
	else if ( getfn == &DataInterpreter<float>::getS4swp )
	    mSet(Integer,Ieee,true,true)
	else if ( getfn == &DataInterpreter<float>::getU4swp )
	    mSet(Integer,Ieee,false,true)
	else if ( getfn == &DataInterpreter<float>::getFswp )
	    mSet(Float,Ieee,true,true)
	else if ( getfn == &DataInterpreter<float>::getS4Ibmswp )
	    mSet(Integer,Ibm,true,true)
	else if ( getfn == &DataInterpreter<float>::getFIbmswp )
	    mSet(Float,Ibm,true,true)
    }

    case 8:
    {
	if ( getfn == &DataInterpreter<float>::getS8 )
	    mSet(Integer,Ieee,true,false)
	else if ( getfn == &DataInterpreter<float>::getU8 )
	    mSet(Integer,Ieee,false,false)
	else if ( getfn == &DataInterpreter<float>::getD )
	    mSet(Float,Ieee,true,false)
	else if ( getfn == &DataInterpreter<float>::getS8swp )
	    mSet(Integer,Ieee,true,true)
	else if ( getfn == &DataInterpreter<float>::getU8swp )
	    mSet(Integer,Ieee,false,true)
	else if ( getfn == &DataInterpreter<float>::getDswp )
	    mSet(Float,Ieee,true,true)
    }

    default:
    {
	if ( getfn == &DataInterpreter<float>::getS1 )
	    mSet(Integer,Ieee,true,false)
	else if ( getfn == &DataInterpreter<float>::getU1 )
	    mSet(Integer,Ieee,false,false)
    }

    }

    return dc;
}
