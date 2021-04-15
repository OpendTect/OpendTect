/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer
 Date:          Mar 2008
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "uimain.h"
#include "uiosgfont.h"
#include "uiglinfo.h"

mDefModInitFn(uiOSG)
{
    mIfNotFirstTime( return );

    uiOsgFontCreator::initClass();
    const int screendpi = uiMain::getMinDPI();
    visBase::DataObject::setDefaultPixelDensity( screendpi );

    uiGLI().createAndShowMessage( true, "dTect.Last GL info" );
}
