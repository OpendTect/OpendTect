/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bruno
 * DATE     : June 2011
-*/




#include "moddepmgr.h"
#include "view2dfault.h"
#include "view2dfaultss2d.h"
#include "view2dfaultss3d.h"
#include "view2dhorizon3d.h"
#include "view2dhorizon2d.h"
#include "view2dpickset.h"
#include "view2dseismic.h"


mDefModInitFn(uiViewer2D)
{
    mIfNotFirstTime( return );

    VW2DFaultSS2D::initClass();
    VW2DFaultSS3D::initClass();
    VW2DFault::initClass();
    Vw2DHorizon2D::initClass();
    Vw2DHorizon3D::initClass();
    VW2DPickSet::initClass();
}
