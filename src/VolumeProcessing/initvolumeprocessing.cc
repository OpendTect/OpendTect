/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initvolumeprocessing.cc,v 1.11 2011/08/23 14:51:33 cvsbert Exp $";


#include "moddepmgr.h"
#include "velocitygridder.h"
#include "volprochorinterfiller.h"
#include "volprocattrib.h"
#include "volproclateralsmoother.h"
#include "volprocsmoother.h"
#include "volprocbodyfiller.h"
#include "volprocvolreader.h"


mDefModInitFn(VolumeProcessing)
{
    mIfNotFirstTime( return );

    VolProc::HorInterFiller::initClass();
    VolProc::LateralSmoother::initClass();
    VolProc::Smoother::initClass();
    VolProc::ExternalAttribCalculator::initClass();
    VolProc::BodyFiller::initClass();
    VolProc::VelGriddingStep::initClass();
    VolProc::VolumeReader::initClass();
}
