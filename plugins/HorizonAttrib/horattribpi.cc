/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: horattribpi.cc,v 1.6 2009-04-06 07:20:09 cvsranojay Exp $";


#include "horizonattrib.h"
#include "initearthmodel.h"
#include "plugins.h"


mExternC int GetHorizonAttribPluginType()
{
    return PI_AUTO_INIT_EARLY;
}


mExternC PluginInfo* GetHorizonAttribPluginInfo()
{
    static PluginInfo retpii = {
	"Horizon-Attribute Base",
	"dGB (Nanne)",
	"=od",
	"The 'Horizon' attribute plugin." };
    return &retpii;
}


mExternC const char* InitHorizonAttribPlugin( int, char** )
{
    EarthModel::initStdClasses();
    Attrib::Horizon::initClass();

    return 0; // All OK - no error messages
}
