/*+
 * (C) JOANNEUM RESEARCH; http://www.joanneum.at
 * AUTHOR   : Christoph Eichkitz; http://www.joanneum.at/resources/gph/mitarbeiterinnen/mitarbeiter-detailansicht/person/0/3144/eichkitz.html
 * DATE     : November 2013
-*/


#include "GLCM_attrib.h"
#include "odplugin.h"


mDefODPluginEarlyLoad(GLCM)
mDefODPluginInfo(GLCM)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"GLCM (Base)",
	"OpendTect",
	"Christoph Eichkitz",
	"1.0",
	"Plugin for the calculation of Grey level co-occurrence"
	" matrix-based attributes.\n"
	"(Joanneum Research)") )
    return &retpi;
}


mDefODInitPlugin(GLCM)
{
    Attrib::GLCM_attrib::initClass();
    return nullptr;
}
