/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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
