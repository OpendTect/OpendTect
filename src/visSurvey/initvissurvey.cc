/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: initvissurvey.cc,v 1.1 2007-12-14 05:10:16 cvssatyaki Exp $";


#include "initvissurvey.h"

#include "vishingeline.h"
#include "vishorizon2ddisplay.h"
#include "vishorizondisplay.h"
#include "vismarchingcubessurfacedisplay.h"
#include "vismpe.h"
#include "vismpeeditor.h"
#include "vismpeseedcatcher.h"
#include "vispicksetdisplay.h"
#include "visplanedatadisplay.h"
#include "vispolylinedisplay.h"
#include "visrandomtrackdisplay.h"
#include "visseedstickeditor.h"
#include "visseis2ddisplay.h"
#include "vissticksetdisplay.h"
#include "vissurvscene.h"
#include "visvolumedisplay.h"
#include "viswelldisplay.h"


namespace visSurvey
{

void initStdClasses()
{
    visSurvey::EdgeLineSetDisplay::initClass();
    visSurvey::Horizon2DDisplay::initClass();
    visSurvey::HorizonDisplay::initClass();
    visSurvey::MarchingCubesDisplay::initClass();
    visSurvey::MPEDisplay::initClass();
    visSurvey::MPEEditor::initClass();
    visSurvey::MPEClickCatcher::initClass();
    visSurvey::PickSetDisplay::initClass();
    visSurvey::PlaneDataDisplay::initClass();
    visSurvey::PolyLineDisplay::initClass();
    visSurvey::RandomTrackDisplay::initClass();
    //visSurvey::SeedEditor::initClass();
    visSurvey::Seis2DDisplay::initClass();
    //visSurvey::StickSetDisplay::initClass();
    visSurvey::Scene::initClass();
    visSurvey::VolumeDisplay::initClass();
    visSurvey::WellDisplay::initClass();
}

}; // namespace visBase
