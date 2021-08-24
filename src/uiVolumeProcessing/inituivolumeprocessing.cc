/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/


#include "coord.h"
#include "moddepmgr.h"

#include "uivelocitygridder.h"
#include "uivolprocattrib.h"
#include "uivolproclateralsmoother.h"
#include "uivolprocbodyfiller.h"
#include "uivolprochorinterfiller.h"
#include "uivolprocsmoother.h"
#include "uivolprocsurfacelimitedfiller.h"
#include "uivolprocvolreader.h"
#include "uiwellloginterpolator.h"
#include "uivolprocbatchsetup.h"



mDefModInitFn(uiVolumeProcessing)
{
    mIfNotFirstTime( return );

    VolProc::uiBodyFiller::initClass();
    VolProc::uiLateralSmoother::initClass();
    VolProc::uiSmoother::initClass();
    VolProc::uiHorInterFiller::initClass();
    VolProc::uiSurfaceLimitedFiller::initClass();
    VolProc::uiVelocityGridder::initClass();
    VolProc::uiVolumeReader::initClass();
    VolProc::uiWellLogInterpolator::initClass();

#ifdef __debug__
    uiVolProcAttrib::initClass();
#endif

    Batch::MMJobDispatcher::addDef( new Batch::VolMMProgDef );
    Batch::ClusterJobDispatcher::addDef( new Batch::VolClusterProgDef );
}
