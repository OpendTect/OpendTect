/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: initvolumeprocessing.cc,v 1.14 2012-05-02 15:11:53 cvskris Exp $";


#include "moddepmgr.h"
#include "velocitygridder.h"
#include "volprochorinterfiller.h"
#include "volprocattrib.h"
#include "volproclateralsmoother.h"
#include "volprocsmoother.h"
#include "volprocbodyfiller.h"
#include "volprocvolreader.h"
#include "volproctrans.h"


mDefModInitFn(VolumeProcessing)
{
    mIfNotFirstTime( return );
    
    VolProcessingTranslatorGroup::initClass();
    dgbVolProcessingTranslator::initClass();

    VolProc::HorInterFiller::initClass();
    VolProc::LateralSmoother::initClass();
    VolProc::Smoother::initClass();
    VolProc::ExternalAttribCalculator::initClass();
    VolProc::BodyFiller::initClass();
    VolProc::VelGriddingStep::initClass();
    VolProc::VolumeReader::initClass();
}
