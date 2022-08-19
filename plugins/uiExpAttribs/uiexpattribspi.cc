/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimenu.h"
#include "uiodmain.h"
#include "odplugin.h"

#include "uieventfreqattrib.h"
#include "uigrubbsfilterattrib.h"
#include "uisimilaritybyaw.h"
#include "uiintegratedtrace.h"
#include "uicorrmultiattrib.h"

mDefODPluginInfo(uiExpAttribs)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Experimental Attributes (GUI)",
	"OpendTect",
	"dGB Earth Sciences",
	"=od",
	"User Interface for Experimental Attributes plugin" ));
    return &retpi;
}


mDefODInitPlugin(uiExpAttribs)
{
    uiEventFreqAttrib::initClass();
    uiGrubbsFilterAttrib::initClass();
    uiCorrMultiAttrib::initClass();
#ifdef __debug__
    uiSimilaritybyAW::initClass();
    uiIntegratedTrace::initClass();
#endif

    return nullptr;
}
