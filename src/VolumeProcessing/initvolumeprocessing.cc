/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initvolumeprocessing.cc,v 1.7 2009-05-22 18:41:55 cvskris Exp $";

#include "initvolumeprocessing.h"

#include "velocitygridder.h"
#include "volprochorinterfiller.h"
#include "volprocattrib.h"
#include "volproclateralsmoother.h"
#include "volprocsmoother.h"
#include "volprocmarchingcubes.h"
#include "volprocvolreader.h"


void VolumeProcessing::initStdClasses()
{
    VolProc::HorInterFiller::initClass();
    VolProc::LateralSmoother::initClass();
    VolProc::Smoother::initClass();
    VolProc::ExternalAttribCalculator::initClass();
    VolProc::MarchingCubes::initClass();
    VolProc::VelGriddingStep::initClass();
    VolProc::VolumeReader::initClass();
}


