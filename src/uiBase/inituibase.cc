/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer
 Date:          Mar 2008
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "uicursor.h"

#include "uihelpview.h"
#include "uirgbarray.h"

void Init_uiLocalFileSelToolProvider_Class();


mDefModInitFn(uiBase)
{
    mIfNotFirstTime( return );

    uiCursorManager::initClass();
    FlareHelpProvider::initODHelp();

    DevDocHelp::initClass();
    WebsiteHelp::initClass();
    uiRGBImageLoader::initClass();

    Init_uiLocalFileSelToolProvider_Class();

#ifdef mUseCrashDumper
    System::CrashDumper::getInstance().setSendAppl(
					System::CrashDumper::sUiSenderAppl() );
#endif
}
