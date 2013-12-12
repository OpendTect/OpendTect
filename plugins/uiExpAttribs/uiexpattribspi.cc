/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		March 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimenu.h"
#include "uiodmain.h"
#include "odplugin.h"

#include "uicurvgrad.h"
#include "uieventfreqattrib.h"
#include "uifkfilterattrib.h"
#include "uigrubbsfilterattrib.h"
#include "uisimilaritybyaw.h"


mDefODPluginInfo(uiExpAttribs)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Experimental Attributes (UI)",
	"dGB",
	"=od",
	"" ));
    return &retpi;
}


mDefODInitPlugin(uiExpAttribs)
{
    uiCurvGrad::initClass();
    uiEventFreqAttrib::initClass();
    uiFKFilterAttrib::initClass();
    uiGrubbsFilterAttrib::initClass();
#ifdef __debug__
    uiSimilaritybyAW::initClass();
#endif

    return 0;
}
