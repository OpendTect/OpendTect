/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          December 2007
 RCS:           $Id: initvolumeprocessing.cc,v 1.2 2008-02-22 23:17:48 cvskris Exp $
________________________________________________________________________

-*/

#include "initvolumeprocessing.h"

#include "horinterfiller.h"
#include "volumeprocessingattribute.h"
#include "volumeprocessingsmoother.h"
#include "marchingcubesprocessing.h"

void VolumeProcessing::initStdClasses()
{
    VolProc::HorInterFiller::initClass();
    VolProc::Smoother::initClass();
    VolProc::ExternalAttribCalculator::initClass();
    VolProc::MarchingCubesProcessing::initClass();
}


