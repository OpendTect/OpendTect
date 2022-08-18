#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ptrman.h"

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
    const std::size_t __allocsize = __size; \
    if ( __allocsize ) \
    { \
	mTryAllocPtrMan( varnm, type [__allocsize] ); \
    } \
    varnm.setSize( __allocsize ); \
}



#ifdef __varlenwithptr__ 

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
