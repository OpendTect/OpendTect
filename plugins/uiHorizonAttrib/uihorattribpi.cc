/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2006
 RCS:		$Id: uihorattribpi.cc,v 1.1 2006-09-22 15:14:43 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uihorizonattrib.h"
#include "plugins.h"
#include "uiattrfact.h"

extern "C" int GetuiHorizonAttribPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetuiHorizonAttribPluginInfo()
{
    static PluginInfo retpi = {
	"Coherency User Interface",
	"dGB - Nanne Hemstra",
	"=od",
	"User interface for Coherency plugin." };
    return &retpi;
}


mDeclAttrDescEd(Horizon)

extern "C" const char* InituiHorizonAttribPlugin( int, char** )
{
    uiAttribFactory::add( "Horizon", "Horizon",
	                              new uiHorizonAttrDescEdCreater);

//  uiHorizonAttrib::initClass();
    return 0; // All OK - no error messages
}
