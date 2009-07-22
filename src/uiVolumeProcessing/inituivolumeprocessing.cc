/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituivolumeprocessing.cc,v 1.8 2009-07-22 16:01:43 cvsbert Exp $";

#include "inituivolumeprocessing.h"

#include "uivelocitygridder.h"
#include "uivolprochorinterfiller.h"
#include "uivolproclateralsmoother.h"
#include "uivolprocmarchingcubes.h"
#include "uivolprocsmoother.h"
#include "uivolprocvolreader.h"

void uiVolumeProcessing::initStdClasses()
{
    VolProc::uiHorInterFiller::initClass();
    VolProc::uiMarchingCubes::initClass();
    VolProc::uiLateralSmoother::initClass();
    VolProc::uiSmoother::initClass();
    VolProc::uiVelocityGridder::initClass();
    VolProc::uiVolumeReader::initClass();
}


