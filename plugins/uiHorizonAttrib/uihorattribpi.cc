/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2006
 RCS:		$Id: uihorattribpi.cc,v 1.4 2007-03-26 13:45:53 cvsbert Exp $
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
	"Horizon-Attribute",
	"dGB - Nanne Hemstra",
	"=od",
	"The 'Horizon' Attribute allows getting values from horizons.\n"
    	"Not to be confused with calculating attributes on horizons.\n"
        "It can even be useful to apply the 'Horizon' attribute on horizons." };
    return &retpi;
}


extern "C" const char* InituiHorizonAttribPlugin( int, char** )
{
    uiHorizonAttrib::initClass();
    return 0;
}
