/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: horattribpi.cc,v 1.8 2011-04-21 13:09:13 cvsbert Exp $";


#include "horizonattrib.h"
#include "initearthmodel.h"
#include "odplugin.h"


mDefODPluginEarlyLoad(HorizonAttrib)
mDefODPluginInfo(HorizonAttrib)
{
    static PluginInfo retpii = {
	"Horizon-Attribute Base",
	"dGB (Nanne)",
	"=od",
	"The 'Horizon' attribute plugin." };
    return &retpii;
}


mDefODInitPlugin(HorizonAttrib)
{
    EarthModel::initStdClasses();
    Attrib::Horizon::initClass();

    return 0; // All OK - no error messages
}
