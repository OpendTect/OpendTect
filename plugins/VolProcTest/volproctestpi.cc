/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : March 2007
-*/

static const char* rcsID = "$Id: volproctestpi.cc,v 1.3 2009-07-22 16:01:27 cvsbert Exp $";


#include "volumeprocessingattribute.h"
#include "volprocthresholder.h"
#include "volumereader.h"
#include "plugins.h"

extern "C" int GetVolProcTestPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetVolProcTestPluginInfo()
{
    static PluginInfo retpi = {
    "Volume Processing Reader Test",
    "Yuancheng",
    "1.1.1",
    "Hello, there! :)" };
    return &retpi;
}


extern "C" const char* InitVolProcTestPlugin( int, char** )
{
    VolProc::VolumeReader::initClass();
    VolProc::ThresholdStep::initClass();

    VolProc::AttributeAdapter::initClass();
    return 0; // All OK - no error messages
}
