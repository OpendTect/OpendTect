/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2006
 RCS:		$Id: uihorattribpi.cc,v 1.2 2006-10-11 06:59:24 cvsbert Exp $
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
	"Coherency User Interface",
	"dGB - Nanne Hemstra",
	"=od",
	"User interface for Coherency plugin." };
    return &retpi;
}


extern "C" const char* InituiHorizonAttribPlugin( int, char** )
{
    uiHorizonAttrib::initClass();
    return 0;
}
