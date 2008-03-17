/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer
 Date:          Mar 2008
 RCS:           $Id: inituibase.cc,v 1.1 2008-03-17 13:13:26 cvskris Exp $
________________________________________________________________________

-*/

#include "inituibase.h"
#include "uicursor.h"

void uiBase::initStdClasses()
{
    uiCursorManager::initClass();
}
