#ifndef varlenarray_h
#define varlenarray_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2007
 RCS:		$Id$
________________________________________________________________________

-*/

#ifdef __msvc__
# define __varlenwithptr__
#endif

#ifdef __mac__
# define __varlenwithptr__
#endif

#ifdef __lux__
# ifdef __debug__
#  define __varlenwithptr__
# endif
#endif

#define mAllocLargeVarLenArr( type, varnm, __size ) \
ArrPtrMan<type> varnm; \
{ \
    const std::size_t __allocsize = mCast(std::size_t,__size); \
    if ( __allocsize ) \
    { \
	mTryAllocPtrMan( varnm, type [__allocsize] ); \
    } \
    varnm.setSize( __allocsize ); \
}


#ifdef __varlenwithptr__ 
# include "ptrman.h"

# define mAllocVarLenArr( type, varnm, __size ) \
    mAllocLargeVarLenArr( type, varnm, __size )

# define mVarLenArr(varnm)	varnm.ptr()
# define mIsVarLenArrOK(varnm)	((bool)varnm.ptr())

#else

# define mAllocVarLenArr( type, varnm, __size ) \
  type varnm[__size];
# define mVarLenArr(varnm)	varnm
# define mIsVarLenArrOK(varnm)	(true)

#endif


#define mAllocVarLenIdxArr(tp,var,sz) \
    mAllocVarLenArr(tp,var,sz) \
    if ( mIsVarLenArrOK(var) ) \
	for ( tp idx=sz-1; idx>=0; idx-- ) \
	    var[idx] = idx;
	

#undef __varlenwithptr__

#endif
