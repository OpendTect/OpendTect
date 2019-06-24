/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/

#include "odplugin.h"

#include "expattribsmod.h"

#include "eventfreqattrib.h"
#include "grubbsfilterattrib.h"
#include "similaritybyaw.h"
#include "integratedtrace.h"

mDefODPluginEarlyLoad(ExpAttribs)
mDefODPluginInfo(ExpAttribs)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Experimental Attributes (Base)",
	mODPluginExtraAttribsPackage,
	mODPluginCreator, mODPluginVersion, mODPluginSeeMainModDesc ) );
    return &retpi;
}


mDefODInitPlugin(ExpAttribs)
{
    Attrib::EventFreq::initClass();
    Attrib::GrubbsFilter::initClass();

#ifdef __debug__
    Attrib::SimilaritybyAW::initClass();
    Attrib::IntegratedTrace::initClass();
#endif

    return 0;
}
