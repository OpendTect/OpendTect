#ifndef varlenarray_h
#define varlenarray_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2007
 RCS:		$Id: varlenarray.h,v 1.1 2007-10-05 10:38:34 cvsnanne Exp $
________________________________________________________________________

-*/


#ifdef __msvc__
# include "ptrman.h"

# define mVariableLengthArr( type, varnm, __size ) \
  ArrPtrMan<type> varnm = new type [__size]

#else

# define mVariableLengthArr( type, varnm, __size ) \
  type varnm[__size]

#endif

#endif
