/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : March 2007
-*/

static const char* rcsID = "$Id: uipsviewerpi.cc,v 1.9 2009-07-22 16:01:28 cvsbert Exp $";

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
