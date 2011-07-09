#ifndef varlenarray_h
#define varlenarray_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2007
 RCS:		$Id: varlenarray.h,v 1.9 2011-07-09 23:57:22 cvskris Exp $
________________________________________________________________________

-*/


#ifdef __msvc__
# include "ptrman.h"

# define mAllocVarLenArr( type, varnm, __size ) \
  ArrPtrMan<type> varnm; \
  if ( __size ) \
      mTryAllocPtrMan( varnm, type [__size] );

# define mVarLenArr(varnm)	varnm.ptr()

#else

# define mAllocVarLenArr( type, varnm, __size ) \
  type varnm[__size];
# define mVarLenArr(varnm)	varnm

#endif


#define mAllocVarLenIdxArr(tp,var,sz) \
    mAllocVarLenArr(tp,var,sz) \
    if ( var ) \
	for ( tp idx=sz-1; idx>=0; idx-- ) \
	    var[idx] = idx;
	


#endif
