/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          December 2007
 RCS:           $Id: initmpeengine.cc,v 1.4 2008-09-09 19:05:05 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "initmpeengine.h"
#include "faulteditor.h"
#include "faulttracker.h"
#include "horizoneditor.h"
#include "horizon2dtracker.h"
#include "horizon3dtracker.h"
#include "polygonsurfeditor.h"

void MPEEngine::initStdClasses()
{
    MPE::FaultEditor::initClass();
    //MPE::FaultTracker::initClass();
    MPE::HorizonEditor::initClass();
    MPE::Horizon2DTracker::initClass();
    MPE::Horizon3DTracker::initClass();
    MPE::PolygonBodyEditor::initClass();
}
