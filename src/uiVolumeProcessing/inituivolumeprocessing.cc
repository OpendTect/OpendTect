/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne and Kristofer
 Date:          December 2007
 RCS:           $Id: inituivolumeprocessing.cc,v 1.1 2007-12-06 20:05:45 cvskris Exp $
________________________________________________________________________

-*/

#include "inituivolumeprocessing.h"

#include "uivolumeprocessingattrib.h"
#include "uihorinterfiller.h"
#include "uimarchingcubesprocessing.h"

void uiVolumeProcessing::initStdClasses()
{
    VolProc::uiHorInterFiller::initClass();
    VolProc::uiMarchingCubesProcessing::initClass();
}


