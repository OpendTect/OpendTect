/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		March 2008
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uiexpattribspi.cc,v 1.9 2012-05-02 15:11:12 cvskris Exp $";

#include "uimenu.h"
#include "uiodmain.h"
#include "odplugin.h"

#include "uigrubbsfilterattrib.h"
#include "uisemblanceattrib.h"


mDefODPluginInfo(uiExpAttribs)
{
    static PluginInfo retpi = {
	"Experimental Attributes (UI)",
	"dGB (Nanne)",
	"=od",
   	"" };
    return &retpi;
}


mDefODInitPlugin(uiExpAttribs)
{
    uiGrubbsFilterAttrib::initClass();
    uiSemblanceAttrib::initClass();
    return 0;
}
