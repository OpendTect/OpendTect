/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2004
________________________________________________________________________

-*/
static const char* rcsID = "$Id: annotationspi.cc,v 1.12 2010-11-08 11:48:22 cvsbert Exp $";

#include "measuretoolman.h"
#include "plugins.h"
#include "measuretoolman.h"
#include "treeitem.h"
#include "uiodscenemgr.h"

#include "visannotimage.h"
#include "visarrow.h"
#include "viscallout.h"


mExternC int GetAnnotationsPluginType()
{
    return PI_AUTO_INIT_LATE;
}


mExternC PluginInfo* GetAnnotationsPluginInfo()
{
    static PluginInfo retpii = {
	"Annotations",
	"dGB (Nanne Hemstra)",
	"=od",
	"Annotation display utilities."
	    "\nThis delivers the 'Annotations' item in the tree." };
    return &retpii;
}


mExternC const char* InitAnnotationsPlugin( int, char** )
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
