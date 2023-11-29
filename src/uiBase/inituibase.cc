/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "uicursor.h"
#include "uibutton.h"
#include "settings.h"

#include "uifileseltool.h"
#include "uihelpview.h"
#include "uirgbarray.h"

void Init_uiLocalFileSelToolProvider_Class();

mDefModInitFn(uiBase)
{
    mIfNotFirstTime( return );

    uiButton::setHaveCommonPBIcons(
	    !Settings::common().isFalse("Ui.Icons.PushButtons") );

    uiCursorManager::initClass();
    FlareHelpProvider::initODHelp();
    VideoProvider::init();

    WebsiteHelp::initClass();
    ReleaseNotesProvider::initClass();
    uiRGBImageLoader::initClass();

    Init_uiLocalFileSelToolProvider_Class();
    uiFileSelToolProvider::addPluginFileSelProviders();

    System::CrashDumper::getInstance().setSendAppl( "od_uiReportIssue" );
}
