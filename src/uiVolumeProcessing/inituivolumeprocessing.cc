/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
