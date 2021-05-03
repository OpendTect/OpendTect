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
#include "volprochorinterfiller.h"
#include "volproclateralsmoother.h"
#include "volprocsmoother.h"
#include "volprocstatscomputer.h"
#include "volprocsurfacelimitedfiller.h"
#include "volproctrans.h"
#include "volprocvolreader.h"
#include "voxelconnectivityfilter.h"
#include "wellloginterpolator.h"

using namespace VolProc;

mDefModInitFn(VolumeProcessing)
{
    mIfNotFirstTime( return );

    VolProcessingTranslatorGroup::initClass();
    VolProcessing2DTranslatorGroup::initClass();
    dgbVolProcessingTranslator::initClass();
    dgbVolProcessing2DTranslator::initClass();

    VolumeReader::initClass();
    BodyFiller::initClass();
    HorInterFiller::initClass();
    SurfaceLimitedFiller::initClass();
    LateralSmoother::initClass();
    Smoother::initClass();
    StatsCalculator::initClass();
    ExternalAttribCalculator::initClass();
    VelocityGridder::initClass();
    VoxelConnectivityFilter::initClass();
    WellLogInterpolator::initClass();

#ifdef __debug__
    VolProcAttrib::initClass();
#endif
}
