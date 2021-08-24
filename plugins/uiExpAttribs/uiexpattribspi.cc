/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		March 2008
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
	"Experimental Attributes (UI)",
	"OpendTect",
	"dGB",
	"=od",
	"" ));
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

    return 0;
}
