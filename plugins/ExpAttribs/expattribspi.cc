/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		January 2008
 RCS:		$Id: expattribspi.cc,v 1.1 2008-10-17 05:42:10 cvsnanne Exp $
________________________________________________________________________

-*/

#include "plugins.h"

#include "semblanceattrib.h"


extern "C" int GetExpAttribsPluginType()
{
    return PI_AUTO_INIT_EARLY;
}


extern "C" PluginInfo* GetExpAttribsPluginInfo()
{
    static PluginInfo retpi = {
	"Experimental Attributes (Non-UI)",
	"dGB (Nanne)",
	"=od",
    	"" };
    return &retpi;
}


extern "C" const char* InitExpAttribsPlugin( int, char** )
{
    Attrib::Semblance::initClass();
    return 0;
}
