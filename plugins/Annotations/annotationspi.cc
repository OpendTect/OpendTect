/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2004
________________________________________________________________________

-*/
static const char* rcsID = "$Id: annotationspi.cc,v 1.9 2009-04-06 07:16:35 cvsranojay Exp $";

#include "measuretoolman.h"
#include "plugins.h"
#include "measuretoolman.h"
#include "treeitem.h"
#include "uiodscenemgr.h"

#include "visannotimage.h"
#include "visarrow.h"
#include "viscallout.h"


mExternC mGlobal int GetAnnotationsPluginType()
{
    return PI_AUTO_INIT_LATE;
}


mExternC mGlobal PluginInfo* GetAnnotationsPluginInfo()
{
    static PluginInfo retpii = {
	"Annotations",
	"dGB (Nanne Hemstra)",
	"=od",
	"Annotation display utilities" };
    return &retpii;
}


mExternC mGlobal const char* InitAnnotationsPlugin( int, char** )
{
    ODMainWin()->sceneMgr().treeItemFactorySet()->addFactory(
	    			new Annotations::TreeItemFactory, 10000 );

    Annotations::MeasureToolMan* mgr =
	new Annotations::MeasureToolMan( *ODMainWin() );

    Annotations::ImageDisplay::initClass();
    Annotations::Image::initClass();
    Annotations::ArrowDisplay::initClass();
    Annotations::CalloutDisplay::initClass();
    Annotations::Callout::initClass();

    return 0;
}
