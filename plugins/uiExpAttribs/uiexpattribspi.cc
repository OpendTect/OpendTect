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
#include "uigrubbsfilterattrib.h"
#include "uisimilaritybyaw.h"


mDefODPluginInfo(uiExpAttribs)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Experimental Attributes (UI)",
	"dGB (Nanne)",
	"=od",
   	"" ));
    return &retpi;
}


mDefODInitPlugin(uiExpAttribs)
{
    uiCurvGrad::initClass();
    uiGrubbsFilterAttrib::initClass();
    uiSimilaritybyAW::initClass();

    return 0;
}
