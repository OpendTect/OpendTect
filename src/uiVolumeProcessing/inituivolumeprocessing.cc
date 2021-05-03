/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/


#include "moddepmgr.h"

#include "uivelocitygridder.h"
#include "uivolprocattrib.h"
#include "uivolproclateralsmoother.h"
#include "uivolprocbodyfiller.h"
#include "uivolprochorinterfiller.h"
#include "uivolprocsmoother.h"
#include "uivolprocsurfacelimitedfiller.h"
#include "uivolprocvolreader.h"
#include "uivoxelconnectivityfilter.h"
#include "uiwellloginterpolator.h"
#include "uivolprocbatchsetup.h"

using namespace VolProc;

mDefModInitFn(uiVolumeProcessing)
{
    mIfNotFirstTime( return );

    uiBodyFiller::initClass();
    uiLateralSmoother::initClass();
    uiSmoother::initClass();
    uiHorInterFiller::initClass();
    uiSurfaceLimitedFiller::initClass();
    uiVelocityGridder::initClass();
    uiVolumeReader::initClass();
    uiVoxelConnectivityFilter::initClass();
    uiWellLogInterpolator::initClass();

#ifdef __debug__
    uiVolProcAttrib::initClass();
#endif

    Batch::MMJobDispatcher::addDef( new Batch::VolMMProgDef );
    Batch::ClusterJobDispatcher::addDef( new Batch::VolClusterProgDef );
}
