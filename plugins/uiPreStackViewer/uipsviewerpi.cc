/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Y.C. Liu
 * DATE     : March 2007
-*/

static const char* rcsID = "$Id: uipsviewerpi.cc,v 1.4 2007-10-12 19:14:34 cvskris Exp $";

#include "plugins.h"
#include "uipsviewermanager.h"
#include "visprestackviewer.h"


extern "C" int GetuiPreStackViewerPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetuiPreStackViewerPluginInfo()
{
    static PluginInfo retpi = {
    "PreStackViewer Plugin",
    "dGB - Kristofer/Yuancheng",
    "1.1.1",
    "PreStack Editor - User Interface." };
    return &retpi;
}


extern "C" const char* InituiPreStackViewerPlugin( int, char** )
{
    PreStackView::PreStackViewer::initClass();
    static PreStackView::uiPSViewerMgr* mgr=0;
    if ( mgr ) return 0;
    mgr = new PreStackView::uiPSViewerMgr();    
    
    return 0; 
}
