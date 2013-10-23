/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "moddepmgr.h"
#include "uiosgfont.h"

mDefModInitFn(uiCoin)
{
    mIfNotFirstTime( return );

    uiOsgFontCreator::initClass();
}
