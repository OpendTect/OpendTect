/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: inituitools.cc,v 1.10 2012-05-02 15:12:20 cvskris Exp $";

#include "moddepmgr.h"
#include "uigridder2d.h"
#include "uiarray2dinterpol.h"
#include "uiraytrace1d.h"

mDefModInitFn(uiTools)
{
    mIfNotFirstTime( return );

    uiInverseDistanceGridder2D::initClass();
    uiInverseDistanceArray2DInterpol::initClass();
    uiTriangulationArray2DInterpol::initClass();
    uiArray2DInterpolExtension::initClass();
    uiVrmsRayTracer1D::initClass();
}
