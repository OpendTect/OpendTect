/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2006
 RCS:		$Id: horattribpi.cc,v 1.2 2007-02-19 10:42:29 cvsdgb Exp $
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
	"The 'Horizon' Attribute - calculation part",
	"dGB - Nanne Hemstra",
	"=od",
	"Enables calculation of the 'Horizon' attribute." };
    return &retpii;
}


extern "C" const char* InitHorizonAttribPlugin( int, char** )
{
    Attrib::Horizon::initClass();

    return 0; // All OK - no error messages
}
