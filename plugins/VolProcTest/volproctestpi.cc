/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : March 2007
-*/

static const char* rcsID mUnusedVar = "$Id: volproctestpi.cc,v 1.6 2012-05-02 15:11:11 cvskris Exp $";


#include "volumeprocessingattribute.h"
#include "volprocthresholder.h"
#include "volumereader.h"
#include "odplugin.h"


mDefODPluginInfo(VolProcTest)
{
    static PluginInfo retpi = {
    "Volume Processing Reader Test",
    "Yuancheng",
    "1.1.1",
    "Hello, there! :)" };
    return &retpi;
}


mDefODInitPlugin(VolProcTest)
{
    VolProc::VolumeReader::initClass();
    VolProc::ThresholdStep::initClass();

    VolProc::AttributeAdapter::initClass();
    return 0; // All OK - no error messages
}
