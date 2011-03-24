/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		March 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiexpattribspi.cc,v 1.6 2011-03-24 03:00:29 cvsnanne Exp $";

#include "uimenu.h"
#include "uiodmain.h"
#include "plugins.h"

#include "uigrubbsfilterattrib.h"
#include "uisemblanceattrib.h"

extern "C" int GetuiExpAttribsPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetuiExpAttribsPluginInfo()
{
    static PluginInfo retpi = {
	"Experimental Attributes (UI)",
	"dGB (Nanne)",
	"=od",
   	"" };
    return &retpi;
}


extern "C" const char* InituiExpAttribsPlugin( int, char** )
{
    uiGrubbsFilterAttrib::initClass();
    uiSemblanceAttrib::initClass();
    return 0;
}
