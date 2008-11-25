/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: expattribspi.cc,v 1.2 2008-11-25 15:35:21 cvsbert Exp $";

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
