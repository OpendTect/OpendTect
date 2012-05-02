/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* mUnusedVar rcsID = "$Id: inituibase.cc,v 1.6 2012-05-02 11:53:35 cvskris Exp $";

#include "moddepmgr.h"
#include "uicursor.h"

mDefModInitFn(uiBase)
{
    mIfNotFirstTime( return );

    uiCursorManager::initClass();
}
