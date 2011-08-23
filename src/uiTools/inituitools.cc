/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituitools.cc,v 1.6 2011-08-23 06:54:12 cvsbert Exp $";

#include "inituitools.h"
#include "uigridder2d.h"
#include "uiarray2dinterpol.h"

void uiTools::initStdClasses()
{
    mIfNotFirstTime( return );

    uiInverseDistanceGridder2D::initClass();
    uiInverseDistanceArray2DInterpol::initClass();
    uiTriangulationArray2DInterpol::initClass();
    uiArray2DInterpolExtension::initClass();
}
