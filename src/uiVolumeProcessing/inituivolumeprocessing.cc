/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "moddepmgr.h"
#include "coord.h"
#include "uivelocitygridder.h"
#include "uivolproclateralsmoother.h"
#include "uivolprocbodyfiller.h"
#include "uivolprocsmoother.h"
#include "uivolprocsurfacelimitedfiller.h"
#include "uivolprocvolreader.h"



mDefModInitFn(uiVolumeProcessing)
{
    mIfNotFirstTime( return );

    VolProc::uiBodyFiller::initClass();
    VolProc::uiLateralSmoother::initClass();
    VolProc::uiSmoother::initClass();
    VolProc::uiSurfaceLimitedFiller::initClass();
    VolProc::uiVelocityGridder::initClass();
    VolProc::uiVolumeReader::initClass();
}
