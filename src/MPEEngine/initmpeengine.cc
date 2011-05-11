/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initmpeengine.cc,v 1.11 2011-05-11 07:17:26 cvsumesh Exp $";

#include "initmpeengine.h"
#include "faulteditor.h"
#include "faultstickseteditor.h"
#include "faulttracker.h"
#include "horizoneditor.h"
#include "horizon2dtracker.h"
#include "horizon3dtracker.h"
#include "horizon2dextender.h"
#include "horizon3dextender.h"
#include "polygonsurfeditor.h"

void MPEEngine::initStdClasses()
{
    MPE::FaultEditor::initClass();
    MPE::FaultStickSetEditor::initClass();
    //MPE::FaultTracker::initClass();
    MPE::HorizonEditor::initClass();
    MPE::Horizon2DEditor::initClass();
    MPE::Horizon2DTracker::initClass();
    MPE::Horizon3DTracker::initClass();
    MPE::Horizon2DExtender::initClass();
    MPE::Horizon3DExtender::initClass();
    MPE::PolygonBodyEditor::initClass();
}
