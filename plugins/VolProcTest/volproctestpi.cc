/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Y.C. Liu
 * DATE     : March 2007
-*/

static const char* rcsID = "$Id: volproctestpi.cc,v 1.1 2007-03-30 21:00:56 cvsyuancheng Exp $";


#include "volprocmgr.h"
#include "volumeprocessingattribute.h"
#include "volprocthresholder.h"
#include "volumereader.h"
#include "uiodmain.h"
#include "uivolumeprocessingattrib.h"
#include "uivolumereader.h"
#include "uivolthresholder.h"
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

    VolProc::uiReader::initClass();
    VolProc::uiVolumeThresholder::initClass();

    VolProc::Manager::get( ODMainWin() );
    VolProc::AttributeAdapter::initClass();
    VolProc::uiVolumeProcessingAttrib::initClass();
    //!<Initalizes toolbar

    return 0; // All OK - no error messages
}
