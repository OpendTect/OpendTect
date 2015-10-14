/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "coord.h"
#include "moddepmgr.h"

#include "uivelocitygridder.h"
#include "uivolprocattrib.h"
#include "uivolprocbodyfiller.h"
#include "uivolprochorinterfiller.h"
#include "uivolproclateralsmoother.h"
#include "uivolprocregionfiller.h"
#include "uivolprocsmoother.h"
#include "uivolprocsurfacelimitedfiller.h"
#include "uivolprocvolreader.h"
#include "uivolprocbatchsetup.h"
#include "uiwellloginterpolator.h"



mDefModInitFn(uiVolumeProcessing)
{
    mIfNotFirstTime( return );

    VolProc::uiBodyFiller::initClass();
    VolProc::uiLateralSmoother::initClass();
    VolProc::uiSmoother::initClass();
    VolProc::uiHorInterFiller::initClass();
    VolProc::uiSurfaceLimitedFiller::initClass();
    VolProc::uiRegionFiller::initClass();
    VolProc::uiVelocityGridder::initClass();
    VolProc::uiVolumeReader::initClass();
    VolProc::uiWellLogInterpolator::initClass();

#ifdef __debug__
    uiVolProcAttrib::initClass();
#endif

    Batch::MMJobDispatcher::addDef( new Batch::VolMMProgDef );
}

