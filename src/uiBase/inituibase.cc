/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituibase.cc,v 1.4 2011-08-23 06:54:12 cvsbert Exp $";

#include "inituibase.h"
#include "uicursor.h"

void uiBase::initStdClasses()
{
    mIfNotFirstTime( return );

    uiCursorManager::initClass();
}
