/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "visfaultdisplay.h"
#include "visfaultsetdisplay.h"
#include "visfaultsticksetdisplay.h"
#include "vishorizon2ddisplay.h"
#include "vishorizondisplay.h"
#include "vismarchingcubessurfacedisplay.h"
#include "vismpeeditor.h"
#include "vismpeseedcatcher.h"
#include "vispicksetdisplay.h"
#include "visplanedatadisplay.h"
#include "vispolygonbodydisplay.h"
#include "vispolylinedisplay.h"
#include "visprestackdisplay.h"
#include "vispseventdisplay.h"
#include "visrandomposbodydisplay.h"
#include "visrandomtrackdisplay.h"
#include "visseis2ddisplay.h"
#include "vissurvscene.h"
#include "visvolumedisplay.h"
#include "viswelldisplay.h"


mDefModInitFn(visSurvey)
{
    mIfNotFirstTime( return );

    visSurvey::FaultDisplay::initClass();
    visSurvey::FaultSetDisplay::initClass();
    visSurvey::FaultStickSetDisplay::initClass();
    visSurvey::Horizon2DDisplay::initClass();
    visSurvey::HorizonDisplay::initClass();
    visSurvey::MarchingCubesDisplay::initClass();
    visSurvey::MPEEditor::initClass();
    visSurvey::MPEClickCatcher::initClass();
    visSurvey::PickSetDisplay::initClass();
    visSurvey::RandomPosBodyDisplay::initClass();
    visSurvey::PlaneDataDisplay::initClass();
    visSurvey::PolygonBodyDisplay::initClass();
    visSurvey::PolyLineDisplay::initClass();
    visSurvey::PreStackDisplay::initClass();
    visSurvey::PSEventDisplay::initClass();
    visSurvey::RandomTrackDisplay::initClass();
    visSurvey::Seis2DDisplay::initClass();
    visSurvey::Scene::initClass();
    visSurvey::VolumeDisplay::initClass();
    visSurvey::WellDisplay::initClass();
}
