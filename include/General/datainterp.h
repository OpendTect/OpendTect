#ifndef datainterp_h
#define datainterp_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Nov 2000
 Contents:	Binary data interpretation
 RCS:		$Id: datainterp.h,v 1.10 2008-10-21 03:39:01 cvsnanne Exp $
________________________________________________________________________

*/


#include "datachar.h"
#include "general.h"
#include "ibmformat.h"


/*!\brief Byte-level data interpreter.

Efficient (one function call overhead) get and set of data, usually in a data
buffer. Facility to swap bytes in advance. The interpretation is into/from
the template parameter. At present, float, double, int and long long
are supported and instantiated.

*/

template<class T>
class DataInterpreter
{
public:
			DataInterpreter(const DataCharacteristics&,
					bool ignoreendianness=false);
			DataInterpreter(const DataInterpreter<T>&);
    void		set(const DataCharacteristics&,
			    bool ignoreendianness=false);
			//!< use ignoreendianness when you pre-byteswap the data
    DataInterpreter<T>&	operator=(const DataInterpreter<T>&);
    inline DataInterpreter<T>& operator=( const DataCharacteristics& dc )
			{ set( dc, false ); return *this; }

    bool		needSwap() const;
    void		swap( void* buf, od_int64 bufsz_in_elements ) const
			{ (this->*swpfn)( buf, bufsz_in_elements );
			  const_cast<DataInterpreter<T>*>(this)->swpSwap(); }

    inline T		get( const void* buf, od_int64 nr ) const
			{ return (this->*getfn)( buf, nr ); }
    inline void		put( void* buf, od_int64 nr, T t ) const
			{ (this->*putfn)( buf, nr, t ); }

    inline bool		operator ==( const DataInterpreter& di ) const
			{ return di.getfn == getfn; }
    inline bool		operator !=( const DataInterpreter& di ) const
			{ return di.getfn != getfn; }
    inline bool		isSUCompat() const
			{ return getfn == &DataInterpreter::getF; }
    int			nrBytes() const;
    DataCharacteristics	dataChar() const;

protected:

    inline void		swap2(void*,od_int64) const;
    inline void		swap4(void*,od_int64) const;
    inline void		swap8(void*,od_int64) const;

    T			getS1(const void*,od_int64) const;
    T			getS2(const void*,od_int64) const;
    T			getS4(const void*,od_int64) const;
    T			getS8(const void*,od_int64) const;
    T			getU1(const void*,od_int64) const;
    T			getU2(const void*,od_int64) const;
    T			getU4(const void*,od_int64) const;
    T			getF(const void*,od_int64) const;
    T			getD(const void*,od_int64) const;

    T			getS2Ibm(const void*,od_int64) const;
    T			getS4Ibm(const void*,od_int64) const;
    T			getFIbm(const void*,od_int64) const;

    T			getS2swp(const void*,od_int64) const;
    T			getS4swp(const void*,od_int64) const;
    T			getS8swp(const void*,od_int64) const;
    T			getU2swp(const void*,od_int64) const;
    T			getU4swp(const void*,od_int64) const;
    T			getFswp(const void*,od_int64) const;
    T			getDswp(const void*,od_int64) const;

    T			getS2Ibmswp(const void*,od_int64) const;
    T			getS4Ibmswp(const void*,od_int64) const;
    T			getFIbmswp(const void*,od_int64) const;

    void		putS1(void*,od_int64,T) const;
    void		putS2(void*,od_int64,T) const;
    void		putS4(void*,od_int64,T) const;
    void		putS8(void*,od_int64,T) const;
    void		putU1(void*,od_int64,T) const;
    void		putU2(void*,od_int64,T) const;
    void		putU4(void*,od_int64,T) const;
    void		putF(void*,od_int64,T) const;
    void		putD(void*,od_int64,T) const;

    void		putS2Ibm(void*,od_int64,T) const;
    void		putS4Ibm(void*,od_int64,T) const;
    void		putFIbm(void*,od_int64,T) const;

    void		putS2swp(void*,od_int64,T) const;
    void		putS4swp(void*,od_int64,T) const;
    void		putS8swp(void*,od_int64,T) const;
    void		putU2swp(void*,od_int64,T) const;
    void		putU4swp(void*,od_int64,T) const;
    void		putFswp(void*,od_int64,T) const;
    void		putDswp(void*,od_int64,T) const;

    void		putS2Ibmswp(void*,od_int64,T) const;
    void		putS4Ibmswp(void*,od_int64,T) const;
    void		putFIbmswp(void*,od_int64,T) const;

    typedef T (DataInterpreter<T>::*GetFn)(const void*,od_int64) const;
    typedef void (DataInterpreter<T>::*PutFn)(void*,od_int64,T) const;
    typedef void (DataInterpreter<T>::*SwapFn)(void*,od_int64) const;
    GetFn		getfn;
    PutFn		putfn;
    SwapFn		swpfn;

    void		swap0(void*,od_int64) const		{}
    T			get0(const void*,od_int64) const	{ return 0; }
    void		put0(void*,od_int64,T) const		{}
    void		swpSwap();

};


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
template <> inline \
void DataInterpreter<typ>::swap##N( void* b, od_int64 s ) const \
{ doswap##N(b,s); }

mDefSwapFn(float,2) mDefSwapFn(float,4) mDefSwapFn(float,8)
mDefSwapFn(int,2) mDefSwapFn(int,4) mDefSwapFn(int,8)
mDefSwapFn(double,2) mDefSwapFn(double,4) mDefSwapFn(double,8)
mDefSwapFn(od_int64,2) mDefSwapFn(od_int64,4) mDefSwapFn(od_int64,8)


#define mDefDIG(rettyp,typ) \
template <> inline \
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
template <> inline \
rettyp DataInterpreter<rettyp>::get##typ( const void* buf, od_int64 nr ) const \
{ T##typ t = *(((T##typ*)buf) + nr); return (rettyp)(t > 0 ? t+.5:t-.5); }

mDefDIGF2I(int,F)
mDefDIGF2I(int,D)
mDefDIGF2I(od_int64,F)
mDefDIGF2I(od_int64,D)


#define mDefDIGswp(rettyp,typ) \
template <> inline \
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
template <> inline \
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
template <> inline \
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
template <> inline \
void DataInterpreter<inptyp>::put##typ( void* buf, od_int64 nr,inptyp f) const \
{ *(((T##typ*)buf)+nr) = f; }

#define mDefDIPISc(inptyp,typ) \
template <> inline \
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
template <> inline \
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
template <> inline \
void DataInterpreter<inptyp>::put##typ( void* buf, od_int64 nr,inptyp f) const \
{ *(((T##typ*)buf)+nr) = f < 0 ? 0 : (T##typ)f; }

#define mDefDIPIUc(inptyp,typ) \
template <> inline \
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
template <> inline \
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
template <> inline \
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
template <> inline \
void DataInterpreter<inptyp>::put##typ##swp(void* buf,od_int64 nr, \
					    inptyp f) const \
{ \
    *(((T##typ*)buf)+nr) = (T##typ)f; \
    SwapBytes( ((T##typ*)buf)+nr, sizeof(T##typ) ); \
}

#define mDefDIPIScswp(inptyp,typ) \
template <> inline \
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
template <> inline \
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
template <> inline \
void DataInterpreter<inptyp>::put##typ##swp(void* buf,od_int64 nr, \
					    inptyp f) const \
{ \
    *(((T##typ*)buf)+nr) = f < 0 ? 0 : (T##typ)f; \
    SwapBytes( ((T##typ*)buf)+nr, sizeof(T##typ) ); \
}

#define mDefDIPUIcswp(inptyp,typ) \
template <> inline \
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
template <> inline \
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
template <> inline \
rettyp DataInterpreter<rettyp>::get##typ##Ibmswp(const void* buf, \
					    od_int64 nr) const \
{ \
     T##typ x = *( ((T##typ*)buf)+nr ); \
     SwapBytes( &x, sizeof(T##typ) ); \
     return (rettyp)IbmFormat::as##fntyp( &x ); \
}

#define mDefDIGIbm(rettyp,typ,fntyp) \
template <> inline \
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
template <> inline \
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
template <> inline \
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


#define mDefDIPFIbm(inptyp,typ,fntyp) \
template <> inline \
void DataInterpreter<inptyp>::put##typ##Ibm(void* buf,od_int64 nr,\
						 inptyp f) const \
{ IbmFormat::put##fntyp( f, ((T##typ*)buf)+nr ); }

mDefDIPFIbm(float,F,Float)
mDefDIPFIbm(double,F,Float)
mDefDIPFIbm(int,F,Float)
mDefDIPFIbm(od_int64,F,Float)


#define mDefDIPSIbmswp(inptyp,typ,fntyp) \
template <> inline \
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


#define mDefDIPFIbmswp(inptyp,typ,fntyp) \
template <> inline \
void DataInterpreter<inptyp>::put##typ##Ibm##swp(void* buf,od_int64 nr,\
						 inptyp f) \
const { \
    IbmFormat::put##fntyp( f, ((T##typ*)buf)+nr ); \
    SwapBytes( ((T##typ*)buf)+nr, sizeof(T##typ) ); \
}

mDefDIPFIbmswp(float,F,Float)
mDefDIPFIbmswp(double,F,Float)
mDefDIPFIbmswp(int,F,Float)
mDefDIPFIbmswp(od_int64,F,Float)


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


#endif
