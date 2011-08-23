/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituivolumeprocessing.cc,v 1.10 2011-08-23 06:54:12 cvsbert Exp $";

#include "inituivolumeprocessing.h"

#include "uivelocitygridder.h"
#include "uivolprochorinterfiller.h"
#include "uivolproclateralsmoother.h"
#include "uivolprocbodyfiller.h"
#include "uivolprocsmoother.h"
#include "uivolprocvolreader.h"

void uiVolumeProcessing::initStdClasses()
{
    mIfNotFirstTime( return );

    VolProc::uiHorInterFiller::initClass();
    VolProc::uiBodyFiller::initClass();
    VolProc::uiLateralSmoother::initClass();
    VolProc::uiSmoother::initClass();
    VolProc::uiVelocityGridder::initClass();
    VolProc::uiVolumeReader::initClass();
}


