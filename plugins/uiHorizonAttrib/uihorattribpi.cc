/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2006
 RCS:		$Id: uihorattribpi.cc,v 1.3 2007-02-19 10:42:29 cvsdgb Exp $
________________________________________________________________________

-*/

#include "uihorizonattrib.h"
#include "plugins.h"

extern "C" int GetuiHorizonAttribPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetuiHorizonAttribPluginInfo()
{
    static PluginInfo retpi = {
	"'Horizon' attribute",
	"dGB - Nanne Hemstra",
	"=od",
	"The 'Horizon' Attribute allows getting values from horizons.\n"
    	"Not to be confused with calculating attributes on horizons."
        "You can even use the 'Horizon' attribute for horizons." };
    return &retpi;
}


extern "C" const char* InituiHorizonAttribPlugin( int, char** )
{
    uiHorizonAttrib::initClass();
    return 0;
}
