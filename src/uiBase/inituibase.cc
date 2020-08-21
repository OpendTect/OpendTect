/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "moddepmgr.h"
#include "uicursor.h"
#include "uibutton.h"
#include "settings.h"

#include "uihelpview.h"
#include "uirgbarray.h"

mDefModInitFn(uiBase)
{
    mIfNotFirstTime( return );

    uiButton::setHaveCommonPBIcons(
	    !Settings::common().isFalse("Ui.Icons.PushButtons") );

    uiCursorManager::initClass();
    FlareHelpProvider::initODHelp();

    WebsiteHelp::initClass();
    uiRGBImageLoader::initClass();

#ifdef mUseCrashDumper
    System::CrashDumper::getInstance().setSendAppl(
					System::CrashDumper::sUiSenderAppl() );
#endif
}
