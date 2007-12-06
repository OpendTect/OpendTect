/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          December 2007
 RCS:           $Id: initvolumeprocessing.cc,v 1.1 2007-12-06 20:05:44 cvskris Exp $
________________________________________________________________________

-*/

#include "initvolumeprocessing.h"

#include "horinterfiller.h"
#include "volumeprocessingattribute.h"
#include "marchingcubesprocessing.h"

void VolumeProcessing::initStdClasses()
{
    VolProc::HorInterFiller::initClass();
    VolProc::ExternalAttribCalculator::initClass();
    VolProc::MarchingCubesProcessing::initClass();
}


