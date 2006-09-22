/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2006
 RCS:		$Id: horattribpi.cc,v 1.1 2006-09-22 09:21:29 cvsnanne Exp $
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
	"HorizonAttribute",
	"dGB - Nanne Hemstra",
	"=od",
	"Horizon attribute plugin." };
    return &retpii;
}


extern "C" const char* InitHorizonAttribPlugin( int, char** )
{
    Attrib::Horizon::initClass();

    return 0; // All OK - no error messages
}
