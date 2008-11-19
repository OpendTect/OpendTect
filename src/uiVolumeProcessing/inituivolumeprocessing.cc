/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne and Kristofer
 Date:          December 2007
 RCS:           $Id: inituivolumeprocessing.cc,v 1.5 2008-11-19 15:01:57 cvskris Exp $
________________________________________________________________________

-*/

#include "inituivolumeprocessing.h"

#include "uivelocitygridder.h"
#include "uivolprochorinterfiller.h"
#include "uivolprocmarchingcubes.h"
#include "uivolprocsmoother.h"
#include "uivolprocvolreader.h"

void uiVolumeProcessing::initStdClasses()
{
    VolProc::uiHorInterFiller::initClass();
    VolProc::uiMarchingCubes::initClass();
    VolProc::uiSmoother::initClass();
    VolProc::uiVelocityGridder::initClass();
    VolProc::uiVolumeReader::initClass();
}


