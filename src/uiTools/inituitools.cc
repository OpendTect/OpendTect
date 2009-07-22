/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituitools.cc,v 1.4 2009-07-22 16:01:42 cvsbert Exp $";

#include "inituitools.h"
#include "uigridder2d.h"
#include "uiarray2dinterpol.h"

void uiTools::initStdClasses()
{
    uiInverseDistanceGridder2D::initClass();
    uiInverseDistanceArray2DInterpol::initClass();
}
