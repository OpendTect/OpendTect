/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		March 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiexpattribspi.cc,v 1.5 2011-03-17 05:24:20 cvssatyaki Exp $";

#include "uimenu.h"
#include "uiodmain.h"
#include "plugins.h"

#include "uigrubbfilterattrib.h"
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
    uiSemblanceAttrib::initClass();
    uiGrubbFilterAttrib::initClass();
    return 0;
}
