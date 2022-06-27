/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : March 2007
-*/


#include "odplugin.h"
#include "uiodmain.h"
#include "uipsviewermanager.h"
#include "uiprestackviewermod.h"


mDefODPluginInfo(uiPreStackViewer)
{
    mDefineStaticLocalObject( PluginInfo, retpi, (
	"Prestack Viewer (GUI)",
	"OpendTect",
	"dGB Earth Sciences (Kristofer/Yuancheng)",
	"=od",
	"This is the PreStack Viewer in the 3D scene."
	    "\nIt can be activated by right-clicking on a plane in the scene."))
    return &retpi;
}


mDefODInitPlugin(uiPreStackViewer)
{
    mDefineStaticLocalObject( PtrMan<PreStackView::uiViewer3DMgr>, theinst_,
		    = new PreStackView::uiViewer3DMgr() );

    if ( !theinst_ )
	return "Cannot instantiate the PreStackViewer plugin";

    return nullptr;
}
