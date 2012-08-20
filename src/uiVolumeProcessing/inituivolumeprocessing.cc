/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: inituivolumeprocessing.cc,v 1.14 2012-08-20 21:05:52 cvsyuancheng Exp $";


#include "moddepmgr.h"
#include "uivolprocfaultangle.h"
#include "uivelocitygridder.h"
#include "uivolprochorinterfiller.h"
#include "uivolproclateralsmoother.h"
#include "uivolprocbodyfiller.h"
#include "uivolprocsmoother.h"
#include "uivolprocvolreader.h"


mDefModInitFn(uiVolumeProcessing)
{
    mIfNotFirstTime( return );

    VolProc::uiFaultAngle::initClass();
    VolProc::uiHorInterFiller::initClass();
    VolProc::uiBodyFiller::initClass();
    VolProc::uiLateralSmoother::initClass();
    VolProc::uiSmoother::initClass();
    VolProc::uiVelocityGridder::initClass();
    VolProc::uiVolumeReader::initClass();
}
