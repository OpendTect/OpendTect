#ifndef rcol_h
#define rcol_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		12-8-1997
 RCS:		$Id$
________________________________________________________________________

-*/

#include "gendefs.h"


/*Macros that implement functions on RowCol and BinID. */


#define mImplInlineRowColFunctions(clss, row, col) \
inline clss::clss( int r, int c ) : row(r), col(c) {}       \
inline clss::clss( const clss& rc ) : row(rc.row), col(rc.col) {}       \
inline clss::clss() : row( 0 ), col ( 0 )  {} \
inline bool clss::operator==(const clss& rc ) const \
	    { return row==rc.row && col==rc.col; }  \
inline bool clss::operator!=(const clss& rc ) const \
	    { return row!=rc.row || col!=rc.col; }  \
inline clss clss::operator+( const clss& rc ) const \
	    { return clss( row+rc.row, col+rc.col ); } \
inline clss clss::operator-( const clss& rc ) const \
	    { return clss( row-rc.row, col-rc.col ); } \
inline clss clss::operator+() const { return clss( +row, +col ); } \
inline clss clss::operator-() const { return clss( -row, -col ); } \
inline clss clss::operator*( const clss& rc ) const \
	    { return clss( row*rc.row, col*rc.col ); } \
inline clss clss::operator*( int factor ) const \
	    { return clss( row*factor, col*factor ); } \
inline clss clss::operator/( const clss& rc ) const \
	    { return clss( row/rc.row, col/rc.col ); } \
inline clss clss::operator/( int denominator ) const \
	    { return clss( row/denominator, col/denominator ); } \
inline const clss& clss::operator+=( const clss& rc ) \
	    { row += rc.row; col += rc.col; return *this; } \
inline const clss& clss::operator-=( const clss& rc ) \
	    { row -= rc.row; col -= rc.col; return *this; } \
inline const clss& clss::operator*=( const clss& rc ) \
	    { row *= rc.row; col *= rc.col; return *this; } \
inline const clss& clss::operator*=( int factor ) \
	    { row *= factor; col *= factor;  return *this; }  \
inline const clss& clss::operator/=( const clss& rc ) \
	    { row /= rc.row; col /= rc.col;  return *this; }  \
inline int& clss::operator[](int idx) { return idx==0 ? row : col; } \
inline int clss::operator[](int idx) const { return idx==0 ? row : col; } \
inline int clss::toInt32() const \
{ return (((unsigned int) row)<<16)+ ((unsigned int) col & 0xFFFF); } \
inline clss clss::fromInt32(int ll) \
{ return clss ( ll>>16, ((short)(ll&0xFFFF)) ); } \
inline od_int64 clss::toInt64() const \
{ \
    return (((od_uint64) row )<<32)+ \
	    ((od_uint64) col &  0xFFFFFFFF); \
} \
inline clss clss::fromInt64( od_int64 serialized ) \
{ \
    return clss( (od_int32) (serialized>>32), \
                 (od_int32) (serialized & 0xFFFFFFFF)); \
} \
 \
 \
inline int clss::sqDistTo( const clss& rc ) const \
{ \
    const int rdist = (row-rc.row); const int cdist = (col-rc.col); \
    return rdist*rdist+cdist*cdist; \
} \
 \
 \



#define mImplRowColFunctions(clss, row, col) \
const char* clss::getUsrStr( bool onlycol ) const \
{ \
    mDeclStaticString( ret ); \
    if ( onlycol ) \
	ret.set( col ); \
    else \
	ret.set( row ).add( "/" ).add( col ); \
    return ret.buf(); \
} \
 \
bool clss::parseUsrStr( const char* str ) \
{ \
    if ( !str || !*str ) return false; \
 \
    static BufferString bs; bs = str; \
    char* ptr = strchr( bs.buf(), '/' ); \
    if ( !ptr ) \
	{ row = 0; col = toInt( bs.buf() ); } \
    else \
	{ *ptr++ = '\0'; row = toInt( bs.buf() ); col = toInt( ptr ); } \
    return true; \
} \
 \
bool clss::isNeighborTo( const clss& rc, const clss& step, \
		         bool eightconnectivity ) const \
{ \
    const clss diff(abs(row-rc.row),abs(col-rc.col)); \
    const bool areeightconnected = diff.row<=step.row && diff.col<=step.col && \
	                                 !(!diff.row && !diff.col); \
    if ( eightconnectivity ) \
	return areeightconnected; \
 \
    const int res = int(diff.row>0) + int(diff.col>0); \
    return areeightconnected && res<2; \
}


#endif
