/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odplugin.h"

#include "expattribsmod.h"

#include "eventfreqattrib.h"
#include "grubbsfilterattrib.h"
#include "similaritybyaw.h"
#include "integratedtrace.h"
#include "corrmultiattrib.h"


mDefODPluginEarlyLoad(ExpAttribs)
mDefODPluginInfo(ExpAttribs)
{
    static PluginInfo retpi(
	"Experimental Attributes (Base)",
	"Experimental Attributes" );
    return &retpi;
}


mDefODInitPlugin(ExpAttribs)
{
    Attrib::EventFreq::initClass();
    Attrib::GrubbsFilter::initClass();
    Attrib::CorrMultiAttrib::initClass();

#ifdef __debug__
    Attrib::SimilaritybyAW::initClass();
    Attrib::IntegratedTrace::initClass();
#endif

    return nullptr;
}
