#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		27-1-98
________________________________________________________________________

-*/

# ifdef do_import_export

#  include "mathfunc.h"

mExportTemplClassInst(Algo) BendPointBasedMathFunction<float,float>;
mExportTemplClassInst(Algo) BendPointBasedMathFunction<double,float>;
mExportTemplClassInst(Algo) BendPointBasedMathFunction<float,double>;
mExportTemplClassInst(Algo) BendPointBasedMathFunction<double,double>;

mExportTemplClassInst(Algo) MathFunction<float,float>;


# endif
