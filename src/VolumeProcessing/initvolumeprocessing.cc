/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "moddepmgr.h"
#include "velocitygridder.h"
#include "volprocattrib.h"
#include "volprochorinterfiller.h"
#include "volproclateralsmoother.h"
#include "volprocsmoother.h"
#include "volprocbodyfiller.h"
#include "volprocstatscomputer.h"
#include "volprocsurfacelimitedfiller.h"
#include "volprocvolreader.h"
#include "volproctrans.h"
#include "wellloginterpolator.h"


mDefModInitFn(VolumeProcessing)
{
    mIfNotFirstTime( return );

    VolProcessingTranslatorGroup::initClass();
    dgbVolProcessingTranslator::initClass();

    VolProc::LateralSmoother::initClass();
    VolProc::Smoother::initClass();
    VolProc::ExternalAttribCalculator::initClass();
    VolProc::BodyFiller::initClass();
    VolProc::HorInterFiller::initClass();
    VolProc::StatsCalculator::initClass();
    VolProc::SurfaceLimitedFiller::initClass();
    VolProc::VelocityGridder::initClass();
    VolProc::VolumeReader::initClass();
    VolProc::WellLogInterpolator::initClass();

#ifdef __debug__
    VolProcAttrib::initClass();
#endif
}
