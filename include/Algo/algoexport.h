#ifndef algoexport_h
#define algoexport_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		27-1-98
 RCS:		$Id$
________________________________________________________________________

-*/



#ifdef __cpp__
#  include "mathfunc.h"

# ifdef mDoWindowsImport
Extern_Algo template class mExp(Algo) MathFunction<float,float>;
# endif

#endif

#endif

