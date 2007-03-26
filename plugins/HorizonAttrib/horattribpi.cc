/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2006
 RCS:		$Id: horattribpi.cc,v 1.3 2007-03-26 13:45:53 cvsbert Exp $
________________________________________________________________________

-*/


#include "horizonattrib.h"
#include "plugins.h"


extern "C" int GetHorizonAttribPluginType()
{
    return PI_AUTO_INIT_EARLY;
}


extern "C" PluginInfo* GetHorizonAttribPluginInfo()
{
    static PluginInfo retpii = {
	"Horizon-Attribute Base",
	"dGB (Nanne)",
	"=od",
	"The 'Horizon' attribute plugin." };
    return &retpii;
}


extern "C" const char* InitHorizonAttribPlugin( int, char** )
{
    Attrib::Horizon::initClass();

    return 0; // All OK - no error messages
}
