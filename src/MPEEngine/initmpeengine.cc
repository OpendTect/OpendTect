/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          December 2007
 RCS:           $Id: initmpeengine.cc,v 1.3 2008-09-09 17:22:03 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "initmpeengine.h"
#include "faulteditor.h"
#include "faulttracker.h"
#include "horizoneditor.h"
#include "horizon2dtracker.h"
#include "horizon3dtracker.h"
#include "polygonsurfeditor.h"
//#include "polygonsurftracker.h"

void MPEEngine::initStdClasses()
{
    MPE::FaultEditor::initClass();
    //MPE::FaultTracker::initClass();
    MPE::HorizonEditor::initClass();
    MPE::Horizon2DTracker::initClass();
    MPE::Horizon3DTracker::initClass();
    MPE::PolygonBodyEditor::initClass();
 //   MPE::PolygonSurfTracker::initClass();
}
