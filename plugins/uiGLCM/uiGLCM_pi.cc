
/*+
 * (C) JOANNEUM RESEARCH; http://www.joanneum.at
 * AUTHOR   : Christoph Eichkitz;
 * DATE     : November 2013
 *

 More info on Christoph:
http://www.joanneum.at/resources/gph/mitarbeiterinnen/mitarbeiter-detailansicht/person/0/3144/eichkitz.html

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
	"GLCM: Gray Level Co-occurrence Matrix",
	mODGLCMPluginPackage,
	"Joanneum Research (Christoph Eichkitz)",
	"1.06",
	"Adds GLCM (Gray Level Co-occurrence Matrix) attributes" ) );
    return &retpi;
}


mDefODInitPlugin(uiGLCM)
{
    uiGLCM_attrib::initClass();
    return 0;
}
