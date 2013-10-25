/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "odplugin.h"

#include "expattribsmod.h"

#include "curvgrad.h"
#include "grubbsfilterattrib.h"
#include "similaritybyaw.h"


mDefODPluginEarlyLoad(ExpAttribs)
mDefODPluginInfo(ExpAttribs)
{
    mDefineStaticLocalObject( PluginInfo, retpi,
	( "Experimental Attributes (Base)",
	"dGB Earth Sciences (Nanne)",
	"=od",
    	"" ) );
    return &retpi;
}


mDefODInitPlugin(ExpAttribs)
{
    Attrib::CurvGrad::initClass();
    Attrib::GrubbsFilter::initClass();
    Attrib::SimilaritybyAW::initClass();

    return 0;
}
