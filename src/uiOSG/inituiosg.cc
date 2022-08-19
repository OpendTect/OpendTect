/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    if ( uiMain::reqOpenGL() )
	uiGLI().createAndShowMessage( true, "dTect.Last GL info" );
}
