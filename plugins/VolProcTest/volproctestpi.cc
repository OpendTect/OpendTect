/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : March 2007
-*/



#include "volumeprocessingattribute.h"
#include "volprocthresholder.h"
#include "volumereader.h"
#include "odplugin.h"


mDefODPluginEarlyLoad(VolProcTest)
{
    mDefineStaticLocalObject( PluginInfo, retpi, (
	"Volume Processing Reader (Base)",
	"OpendTect",
	"dGB Earth Sciences (Yuancheng)",
	"=od",
	"Volume Processing Reader test" ))
    return &retpi;
}


mDefODInitPlugin(VolProcTest)
{
    VolProc::VolumeReader::initClass();
    VolProc::ThresholdStep::initClass();

    VolProc::AttributeAdapter::initClass();
    return nullptr;
}
