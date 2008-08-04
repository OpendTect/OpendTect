/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2004
 RCS:           $Id: annotationspi.cc,v 1.6 2008-08-04 11:11:31 cvsnanne Exp $
________________________________________________________________________

-*/

#include "measuretoolman.h"
#include "plugins.h"
#include "measuretoolman.h"
#include "treeitem.h"
#include "uiodscenemgr.h"

#include "visannotimage.h"
#include "visarrow.h"
#include "viscallout.h"


extern "C" int GetAnnotationsPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetAnnotationsPluginInfo()
{
    static PluginInfo retpii = {
	"Annotations",
	"dGB (Nanne Hemstra)",
	"=dgb",
	"Annotation display utilities" };
    return &retpii;
}


extern "C" const char* InitAnnotationsPlugin( int, char** )
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
