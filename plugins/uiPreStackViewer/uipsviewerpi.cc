/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Y.C. Liu
 * DATE     : March 2007
-*/

static const char* rcsID = "$Id: uipsviewerpi.cc,v 1.8 2009-04-06 07:33:03 cvsranojay Exp $";

#include "plugins.h"
#include "uipsviewermanager.h"
#include "visprestackviewer.h"


mExternC int GetuiPreStackViewerPluginType()
{
    return PI_AUTO_INIT_LATE;
}


mExternC PluginInfo* GetuiPreStackViewerPluginInfo()
{
    static PluginInfo retpi = {
    "Pre-Stack Viewer",
    "dGB - Kristofer/Yuancheng",
    "1.1.1",
    "This is the PreStack Viewer in the 3D scene."
	"\nIt can be activated by right-clicking on a plane in the scene." };
    return &retpi;
}


mExternC const char* InituiPreStackViewerPlugin( int, char** )
{
    PreStackView::Viewer3D::initClass();
    static PreStackView::uiViewer3DMgr* mgr=0;
    if ( mgr ) return 0;
    mgr = new PreStackView::uiViewer3DMgr();    
    
    return 0; 
}
