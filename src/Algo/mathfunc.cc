/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/
 
static const char* rcsID mUsedVar = "$Id$";


#include "mathfunc.h"
#include "linear.h"
#include "interpol1d.h"

LineParameters<float>* SecondOrderPoly::createDerivative() const
{
    return new LinePars( b, a*2 );
}

#define mXT float
#define mYT float
#include "pointbasedmathfunc.inc"
#undef mXT
#undef mYT
#define mXT float
#define mYT double
#include "pointbasedmathfunc.inc"
#undef mXT
#undef mYT
#define mXT double
#define mYT float
#include "pointbasedmathfunc.inc"
#undef mXT
#undef mYT
#define mXT double
#define mYT double
#include "pointbasedmathfunc.inc"
