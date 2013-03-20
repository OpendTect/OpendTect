/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "moddepmgr.h"
#include "uigridder2d.h"
#include "uiarray2dinterpol.h"
#include "uiraytrace1d.h"
#include "filepath.h"

mDefModInitFn(uiTools)
{
    mIfNotFirstTime( return );

    uiInverseDistanceGridder2D::initClass();
    uiInverseDistanceArray2DInterpol::initClass();
    uiTriangulationArray2DInterpol::initClass();
    uiExtensionArray2DInterpol::initClass();
    uiVrmsRayTracer1D::initClass();
    
#ifdef mUseCrashDumper
    System::CrashDumper::getInstance().setSendAppl(
	    				System::CrashDumper::sUiSenderAppl() );
#endif
}
