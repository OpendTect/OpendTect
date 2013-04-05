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

mDefModInitFn(uiBase)
{
    mIfNotFirstTime( return );

    uiCursorManager::initClass();

#ifdef mUseCrashDumper
    System::CrashDumper::getInstance().setSendAppl(
	    				System::CrashDumper::sUiSenderAppl() );
#endif
}
