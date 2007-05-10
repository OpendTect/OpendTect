/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          13/01/2005
 RCS:           $Id: undefarray.cc,v 1.4 2007-05-10 15:49:53 cvshelene Exp $
________________________________________________________________________

-*/

#include "undefarray.h"

#include "bindatadesc.h"
#include "undefval.h"

#include <limits.h>


UndefArrayHandler::UndefArrayHandler( const BinDataDesc& desc )
   : isudf_( 0 )
   , setudf_( 0 )
   , limitrange_( 0 )
{
    set( desc );
}

#define mSetFunc( isint, issigned, sz, postfix ) \
if ( desc==BinDataDesc( isint, issigned, BinDataDesc::sz ) ) \
{ \
    isudf_ = UndefArrayHandler::isUdf##postfix; \
    setudf_ = UndefArrayHandler::setUdf##postfix; \
    limitrange_ = UndefArrayHandler::unsetUdf##postfix; \
}


bool UndefArrayHandler::set(const BinDataDesc& desc)
{
    isudf_ = 0;
    setudf_ = 0;
    limitrange_ = 0;

    mSetFunc( true, true, N1, UChar )
    else mSetFunc( true, true, N1, Char )
    else mSetFunc( true, false, N2, UShort )
    else mSetFunc( true, true, N2, Short )
    else mSetFunc( true, false, N4, UInt32 )
    else mSetFunc( true, true, N4, Int32 )
    else mSetFunc( true, false, N8, UInt64 )
    else mSetFunc( true, true, N8, Int64 )
    else mSetFunc( false, true, N4, Float )
    else mSetFunc( false, true, N8, Double )

    return isOK();
}


bool UndefArrayHandler::isOK() const
{ return isudf_ && setudf_ && limitrange_; }


bool UndefArrayHandler::isUdf(const void* ptr, int64 idx) const
{ return isudf_(ptr,idx); }


void UndefArrayHandler::setUdf(void* ptr, int64 idx) const
{ setudf_(ptr,idx); }


void UndefArrayHandler::unSetUdf(void* ptr, int64 idx) const
{ limitrange_(ptr,idx); }


#define mImplFuncs( type, udfval, udfreplace, postfix ) \
bool UndefArrayHandler::isUdf##postfix(const void* ptr,int64 idx) \
{ \
    const type* myptr = (const type*)(ptr); \
    return myptr[idx]==udfval; \
} \
 \
 \
void UndefArrayHandler::setUdf##postfix( void* ptr, int64 idx ) \
{ \
    type* myptr = (type*)(ptr); \
    myptr[idx] = udfval; \
} \
 \
 \
void UndefArrayHandler::unsetUdf##postfix( void* ptr, int64 idx ) \
{ \
    if ( !isUdf##postfix( ptr, idx ) ) \
	return; \
	 \
    type* myptr = (type*)(ptr); \
    myptr[idx] = udfreplace; \
}


#define mImplFloatFuncs( type, udfval, udfreplace, postfix ) \
 \
bool UndefArrayHandler::isUdf##postfix(const void* ptr,int64 idx) \
{ \
    const type* myptr = (const type*)(ptr); \
    return mIsUdf(myptr[idx]); \
} \
 \
 \
void UndefArrayHandler::setUdf##postfix( void* ptr, int64 idx ) \
{ \
    type* myptr = (type*)(ptr); \
    myptr[idx] = udfval; \
} \
 \
 \
void UndefArrayHandler::unsetUdf##postfix( void* ptr, int64 idx ) \
{ \
    if ( !isUdf##postfix( ptr, idx ) ) \
	return; \
	 \
    type* myptr = (type*)(ptr); \
    myptr[idx] = udfreplace; \
}


mImplFuncs( unsigned char, 255, 254, UChar )
mImplFuncs( char, 127, 126, Char )
mImplFuncs( unsigned short, 65535, 65534, UShort )
mImplFuncs( short, 32767, 32766, Short )
mImplFuncs( uint32, mUdf(uint32), INT_MAX, UInt32 )
mImplFuncs( int32, mUdf(int32), INT_MAX, Int32 )
mImplFuncs( uint64, mUdf(uint64), INT_MAX, UInt64 )
mImplFuncs( int64, mUdf(int64), mUdf(int64)+1, Int64 )
mImplFloatFuncs( float, mUdf(float), 1e29, Float )
mImplFloatFuncs( double, mUdf(double), 1e29, Double )
