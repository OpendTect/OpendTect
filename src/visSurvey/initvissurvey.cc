/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: initvissurvey.cc,v 1.3 2008-05-30 21:05:43 cvskris Exp $";


#include "initvissurvey.h"

#include "vishingeline.h"
#include "vishorizon2ddisplay.h"
#include "vishorizondisplay.h"
#include "visfaultdisplay.h"
#include "vismarchingcubessurfacedisplay.h"
#include "vismpe.h"
#include "vismpeeditor.h"
#include "vismpeseedcatcher.h"
#include "vispicksetdisplay.h"
#include "visplanedatadisplay.h"
#include "vispolylinedisplay.h"
#include "visrandomtrackdisplay.h"
#include "visseis2ddisplay.h"
#include "vissurvscene.h"
#include "visvolumedisplay.h"
#include "viswelldisplay.h"


namespace visSurvey
{

void initStdClasses()
{
    EdgeLineSetDisplay::initClass();
    Horizon2DDisplay::initClass();
    HorizonDisplay::initClass();
    MarchingCubesDisplay::initClass();
    MPEDisplay::initClass();
    MPEEditor::initClass();
    FaultDisplay::initClass();
    MPEClickCatcher::initClass();
    PickSetDisplay::initClass();
    PlaneDataDisplay::initClass();
    PolyLineDisplay::initClass();
    RandomTrackDisplay::initClass();
    Seis2DDisplay::initClass();
    Scene::initClass();
    VolumeDisplay::initClass();
    WellDisplay::initClass();
}

}; // namespace visBase
