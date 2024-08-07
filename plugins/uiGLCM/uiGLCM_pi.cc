
/*+
 * (C) JOANNEUM RESEARCH; https://www.joanneum.at
 * AUTHOR   : Christoph Eichkitz
 * DATE     : November 2013
-*/


#include "uiGLCM_attrib.h"
#include "uiglcmmod.h"

#include "odplugin.h"
#include "uiodmain.h"
#include "uimain.h"

#include <iostream>

mDefODPluginInfo(uiGLCM)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"GLCM (GUI)",
	"OpendTect",
	"Joanneum Research (Christoph Eichkitz)",
	"1.0",
	"User Interface for for the calculation of Grey level co-occurrence"
	" matrix-based attributes" ))
    return &retpi;
}


mDefODInitPlugin(uiGLCM)
{
    uiGLCM_attrib::initClass();
    return nullptr;
}
