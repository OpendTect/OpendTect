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

# ifdef do_import_export

#  ifdef __cpp__
#   include "mathfunc.h"

mExportTemplClassInst(Algo) BendPointBasedMathFunction<float,float>;
mExportTemplClassInst(Algo) BendPointBasedMathFunction<double,float>;
mExportTemplClassInst(Algo) BendPointBasedMathFunction<float,double>;
mExportTemplClassInst(Algo) BendPointBasedMathFunction<double,double>;

mExportTemplClassInst(Algo) MathFunction<float,float>;

#  endif

# endif

#endif
