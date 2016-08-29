/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer
 Date:          Mar 2008
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "uiosgfont.h"
#include "uiglinfo.h"

mDefModInitFn(uiOSG)
{
    mIfNotFirstTime( return );

    uiOsgFontCreator::initClass();

    uiGLI().createAndShowMessage( true, "dTect.Last GL info" );
}
