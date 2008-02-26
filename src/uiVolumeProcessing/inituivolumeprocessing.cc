/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne and Kristofer
 Date:          December 2007
 RCS:           $Id: inituivolumeprocessing.cc,v 1.3 2008-02-26 23:02:59 cvskris Exp $
________________________________________________________________________

-*/

#include "inituivolumeprocessing.h"

#include "uivolprochorinterfiller.h"
#include "uivolprocmarchingcubes.h"
#include "uivolprocsmoother.h"

void uiVolumeProcessing::initStdClasses()
{
    VolProc::uiHorInterFiller::initClass();
    VolProc::uiMarchingCubes::initClass();
    VolProc::uiSmoother::initClass();
}


