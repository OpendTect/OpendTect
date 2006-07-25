/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2004
 RCS:           $Id: annotationspi.cc,v 1.2 2006-07-25 22:24:27 cvskris Exp $
________________________________________________________________________

-*/

#include "plugins.h"
#include "treeitem.h"
#include "uiodscenemgr.h"


extern "C" int GetnewAnnotationsPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetnewAnnotationsPluginInfo()
{
    static PluginInfo retpii = {
	"Annotations",
	"dGB (Nanne Hemstra)",
	"=dgb",
	"Annotation display utilities" };
    return &retpii;
}


extern "C" const char* InitnewAnnotationsPlugin( int, char** )
{
    ODMainWin()->sceneMgr().treeItemFactorySet()->addFactory(
	    			new Annotations::TreeItemFactory, 10000 );
    return 0;
}
