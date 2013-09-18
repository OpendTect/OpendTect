/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/
 
static const char* rcsID mUsedVar = "$Id$";


#include "mathfunc.h"
#include "linear.h"

LineParameters<float>* SecondOrderPoly::createDerivative() const
{
    return new LinePars( b, a*2 );
}
