#ifndef varlenarray_h
#define varlenarray_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2007
 RCS:		$Id: varlenarray.h,v 1.6 2009-03-18 15:08:11 cvskris Exp $
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

#endif
