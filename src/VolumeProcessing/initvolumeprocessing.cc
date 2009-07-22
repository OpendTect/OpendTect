/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initvolumeprocessing.cc,v 1.8 2009-07-22 16:01:36 cvsbert Exp $";

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


