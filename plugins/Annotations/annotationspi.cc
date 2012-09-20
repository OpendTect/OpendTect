/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2004
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "measuretoolman.h"
#include "odplugin.h"
#include "measuretoolman.h"
#include "treeitem.h"
#include "uiodscenemgr.h"

#include "visannotimage.h"
#include "visarrow.h"
#include "viscallout.h"
#include "visscalebar.h"

#include "annotationsmod.h"


mDefODPluginInfo(Annotations)
{
    static PluginInfo retpii = {
	"Annotations",
	"dGB (Nanne Hemstra)",
	"=od",
	"Annotation display utilities."
	    "\nThis delivers the 'Annotations' item in the tree." };
    return &retpii;
}


mDefODInitPlugin(Annotations)
{
    ODMainWin()->sceneMgr().treeItemFactorySet()->addFactory(
	    			new Annotations::TreeItemFactory, 10000 );

    new Annotations::MeasureToolMan( *ODMainWin() );

    Annotations::ImageDisplay::initClass();
    Annotations::Image::initClass();
    Annotations::ArrowDisplay::initClass();
    Annotations::CalloutDisplay::initClass();
    Annotations::Callout::initClass();
    Annotations::ScaleBar::initClass();
    Annotations::ScaleBarDisplay::initClass();

    return 0;
}
