/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initmpeengine.cc,v 1.9 2010-06-07 16:00:41 cvsjaap Exp $";

#include "initmpeengine.h"
#include "faulteditor.h"
#include "faultstickseteditor.h"
#include "faulttracker.h"
#include "horizoneditor.h"
#include "horizon2dtracker.h"
#include "horizon3dtracker.h"
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
    MPE::PolygonBodyEditor::initClass();
}
