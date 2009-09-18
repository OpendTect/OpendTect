/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituivolumeprocessing.cc,v 1.9 2009-09-18 18:13:43 cvskris Exp $";

#include "inituivolumeprocessing.h"

#include "uivelocitygridder.h"
#include "uivolprochorinterfiller.h"
#include "uivolproclateralsmoother.h"
#include "uivolprocbodyfiller.h"
#include "uivolprocsmoother.h"
#include "uivolprocvolreader.h"

void uiVolumeProcessing::initStdClasses()
{
    VolProc::uiHorInterFiller::initClass();
    VolProc::uiBodyFiller::initClass();
    VolProc::uiLateralSmoother::initClass();
    VolProc::uiSmoother::initClass();
    VolProc::uiVelocityGridder::initClass();
    VolProc::uiVolumeReader::initClass();
}


