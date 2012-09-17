#ifndef rcol_h
#define rcol_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		12-8-1997
 RCS:		$Id: rcol.h,v 1.23 2011/09/08 15:05:17 cvsjaap Exp $
________________________________________________________________________

-*/

#include "gendefs.h"


/*Macros that implement functions on RowCol and BinID. */


#define mImplInlineRowColFunctions(clss, row, col) \
inline clss::clss( int r, int c ) : row(r), col(c) {}       \
inline clss::clss( const clss& rc ) : row(rc.row), col(rc.col) {}       \
inline clss::clss( const od_int64& ser )   { fromInt64(ser); } \
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
inline void clss::fromInt32(int ll) \
{ row = ll>>16; col = ((short)(ll&0xFFFF)); } \
inline od_int64 clss::toInt64() const \
{ \
    return (((od_uint64) row )<<32)+ \
	    ((od_uint64) col &  0xFFFFFFFF); \
} \
inline void clss::fromInt64( od_int64 serialized ) \
{ \
    row = (od_int32) (serialized>>32); \
    col = (od_int32) (serialized & 0xFFFFFFFF); \
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
void	clss::fill(char* str) const \
{ \
    if ( !str ) return; \
    sprintf( str, "%d/%d", row, col ); \
} \
 \
bool	clss::use(const char* str) \
{ \
    if ( !str || !*str ) return false; \
 \
    static BufferString buf; buf = str; \
    char* ptr = strchr( buf.buf(), '/' ); \
    if ( !ptr ) return false; \
    *ptr++ = '\0'; \
    row = toInt( buf.buf() ); col = toInt( ptr ); \
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
} \
 \
 \
od_int64 clss::getSerialized() const \
{ \
    static bool didwarn = false; \
    if ( !didwarn ) \
    { \
	pErrMsg("Legacy, use toInt64 instead" ); \
	didwarn = true; \
    } \
 \
    return toInt64(); \
} \
 \
 \
void clss::setSerialized(od_int64 ll) \
{ \
    static bool didwarn = false; \
    if ( !didwarn ) \
    { \
	pErrMsg("Legacy, use fromInt64 instead" ); \
	didwarn = true; \
    } \
 \
    fromInt64( ll ); \
}


#endif
