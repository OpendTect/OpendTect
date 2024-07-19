/*+
 * (C) JOANNEUM RESEARCH; https://www.joanneum.at
 * AUTHOR   : Christoph Eichkitz
 * DATE     : November 2013
-*/


#include "GLCM_attrib.h"
#include "odplugin.h"


mDefODPluginEarlyLoad(GLCM)
mDefODPluginInfo(GLCM)
{
    mDefineStaticLocalObject( PluginInfo, retpi, (
	"GLCM (Base)",
	"OpendTect",
	"Joanneum Research (Christoph Eichkitz)",
	"1.0",
	"Plugin for the calculation of Grey level co-occurrence"
	" matrix-based attributes" ))
    return &retpi;
}


mDefODInitPlugin(GLCM)
{
    Attrib::GLCM_attrib::initClass();
    return nullptr;
}
