/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituibase.cc,v 1.5 2011/08/23 14:51:33 cvsbert Exp $";

#include "moddepmgr.h"
#include "uicursor.h"

mDefModInitFn(uiBase)
{
    mIfNotFirstTime( return );

    uiCursorManager::initClass();
}
