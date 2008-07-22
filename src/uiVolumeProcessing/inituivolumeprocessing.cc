/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne and Kristofer
 Date:          December 2007
 RCS:           $Id: inituivolumeprocessing.cc,v 1.4 2008-07-22 17:39:21 cvskris Exp $
________________________________________________________________________

-*/

#include "inituivolumeprocessing.h"

#include "uivelocitygridder.h"
#include "uivolprochorinterfiller.h"
#include "uivolprocmarchingcubes.h"
#include "uivolprocsmoother.h"

void uiVolumeProcessing::initStdClasses()
{
    VolProc::uiHorInterFiller::initClass();
    VolProc::uiMarchingCubes::initClass();
    VolProc::uiSmoother::initClass();
    VolProc::uiVelocityGridder::initClass();
}


