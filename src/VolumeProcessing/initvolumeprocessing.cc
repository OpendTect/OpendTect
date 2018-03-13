/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          December 2007
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "velocitygridder.h"
#include "volprocattrib.h"
#include "volprocbodyfiller.h"
#include "volprochilbert.h"
#include "volprochorinterfiller.h"
#include "volproclateralsmoother.h"
#include "volprocregionfiller.h"
#include "volprocsmoother.h"
#include "volprocstatscomputer.h"
#include "volprocsurfacelimitedfiller.h"
#include "volproctrans.h"
#include "volprocudfreplacer.h"
#include "volprocvolreader.h"
#include "wellloginterpolator.h"


mDefModInitFn(VolumeProcessing)
{
    mIfNotFirstTime( return );

    VolProcessingTranslatorGroup::initClass();
    VolProcessing2DTranslatorGroup::initClass();
    dgbVolProcessingTranslator::initClass();
    dgbVolProcessing2DTranslator::initClass();

    VolProc::BodyFiller::initClass();
    VolProc::ExternalAttribCalculator::initClass();
    VolProc::HilbertCalculator::initClass();
    VolProc::HorInterFiller::initClass();
    VolProc::LateralSmoother::initClass();
    VolProc::RegionFiller::initClass();
    VolProc::Smoother::initClass();
    VolProc::StatsCalculator::initClass();
    VolProc::SurfaceLimitedFiller::initClass();
    VolProc::UdfReplacer::initClass();
    VolProc::VelocityGridder::initClass();
    VolProc::VolumeReader::initClass();
    VolProc::WellLogInterpolator::initClass();

#ifdef __debug__
    VolProcAttrib::initClass();
#endif
}
