/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituitools.cc,v 1.3 2009-04-23 18:08:50 cvskris Exp $";

#include "inituitools.h"
#include "uigridder2d.h"
#include "uiarray2dinterpol.h"

void uiTools::initStdClasses()
{
    uiInverseDistanceGridder2D::initClass();
    uiInverseDistanceArray2DInterpol::initClass();
}
