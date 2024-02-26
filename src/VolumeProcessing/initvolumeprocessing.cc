/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "velocitygridder.h"
#include "volprocattrib.h"
#include "volprocbodyfiller.h"
#include "volprochorinterfiller.h"
#include "volproclateralsmoother.h"
#include "volprocmath.h"
#include "volprocsmoother.h"
#include "volprocstatscomputer.h"
#include "volprocsurfacelimitedfiller.h"
#include "volproctrans.h"
#include "volprocvolreader.h"
#include "voxelconnectivityfilter.h"
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
    VolProc::HorInterFiller::initClass();
    VolProc::LateralSmoother::initClass();
    VolProc::Math::initClass();
    VolProc::Smoother::initClass();
    VolProc::StatsCalculator::initClass();
    VolProc::SurfaceLimitedFiller::initClass();
    VolProc::VelocityGridder::initClass();
    VolProc::VolumeReader::initClass();
    VolProc::VoxelConnectivityFilter::initClass();
    VolProc::WellLogInterpolator::initClass();

#ifdef __debug__
    VolProcAttrib::initClass();
#endif
}
