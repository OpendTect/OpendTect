
/*+
 * (C) JOANNEUM RESEARCH; http://www.joanneum.at
 * AUTHOR   : Christoph Eichkitz; http://www.joanneum.at/resources/gph/mitarbeiterinnen/mitarbeiter-detailansicht/person/0/3144/eichkitz.html
 * DATE     : November 2013
-*/


#include "uiglcmmod.h"
#include "uiGLCM_attrib.h"
#include "GLCM_attrib.h"

#include "odplugin.h"
#include "uiodmain.h"
#include "uimain.h"

#include <iostream>

mDefODPluginInfo(uiGLCM)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"GLCM (GUI)",
	"OpendTect",
	"Christoph Eichkitz",
	"1.0",
	"Plugin for the calculation of Grey level co-occurrence"
	" matrix-based attributes.\n"
	"(Joanneum Research)") )
    return &retpi;
}


mDefODInitPlugin(uiGLCM)
{
    uiGLCM_attrib::initClass();
    return nullptr;
}
