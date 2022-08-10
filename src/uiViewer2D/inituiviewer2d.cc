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

    View2D::FaultSS2D::initClass();
    View2D::FaultSS3D::initClass();
    View2D::Fault::initClass();
    View2D::Horizon2D::initClass();
    View2D::Horizon3D::initClass();
    View2D::PickSet::initClass();
}
