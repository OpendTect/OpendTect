/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituibase.cc,v 1.2 2008-11-25 15:35:24 cvsbert Exp $";

#include "inituibase.h"
#include "uicursor.h"

void uiBase::initStdClasses()
{
    uiCursorManager::initClass();
}
