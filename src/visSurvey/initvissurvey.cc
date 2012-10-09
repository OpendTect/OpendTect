/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id$";


#include "moddepmgr.h"
#include "visfaultdisplay.h"
#include "visfaultsticksetdisplay.h"
#include "vishingeline.h"
#include "vishorizon2ddisplay.h"
#include "vishorizondisplay.h"
#include "vismarchingcubessurfacedisplay.h"
#include "vismpe.h"
#include "vismpeeditor.h"
#include "vismpeseedcatcher.h"
#include "vispicksetdisplay.h"
#include "visplanedatadisplay.h"
#include "vispolygonbodydisplay.h"
#include "vispolylinedisplay.h"
#include "visrandomposbodydisplay.h"
#include "visrandomtrackdisplay.h"
#include "visseis2ddisplay.h"
#include "vissurvscene.h"
#include "visvolumedisplay.h"
#include "viswelldisplay.h"


mDefModInitFn(visSurvey)
{
    mIfNotFirstTime( return );

    visSurvey::EdgeLineSetDisplay::initClass();
    visSurvey::FaultDisplay::initClass();
    visSurvey::FaultStickSetDisplay::initClass();
    visSurvey::Horizon2DDisplay::initClass();
    visSurvey::HorizonDisplay::initClass();
    visSurvey::MarchingCubesDisplay::initClass();
    visSurvey::MPEDisplay::initClass();
    visSurvey::MPEEditor::initClass();
    visSurvey::MPEClickCatcher::initClass();
    visSurvey::PickSetDisplay::initClass();
    visSurvey::RandomPosBodyDisplay::initClass();
    visSurvey::PlaneDataDisplay::initClass();
    visSurvey::PolygonBodyDisplay::initClass();
    visSurvey::PolyLineDisplay::initClass();
    visSurvey::RandomTrackDisplay::initClass();
    visSurvey::Seis2DDisplay::initClass();
    visSurvey::Scene::initClass();
    visSurvey::VolumeDisplay::initClass();
    visSurvey::WellDisplay::initClass();
}
