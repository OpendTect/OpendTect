/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne and Kristofer
 Date:          December 2007
 RCS:           $Id: inituivolumeprocessing.cc,v 1.2 2008-02-25 19:14:55 cvskris Exp $
________________________________________________________________________

-*/

#include "inituivolumeprocessing.h"

#include "uivolprochorinterfiller.h"
#include "uivolprocmarchingcubes.h"

void uiVolumeProcessing::initStdClasses()
{
    VolProc::uiHorInterFiller::initClass();
    VolProc::uiMarchingCubes::initClass();
}


