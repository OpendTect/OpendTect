/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituibase.cc,v 1.3 2009-07-22 16:01:37 cvsbert Exp $";

#include "inituibase.h"
#include "uicursor.h"

void uiBase::initStdClasses()
{
    uiCursorManager::initClass();
}
