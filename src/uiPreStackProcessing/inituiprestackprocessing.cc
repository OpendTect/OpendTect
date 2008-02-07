/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          December 2007
 RCS:           $Id: inituiprestackprocessing.cc,v 1.2 2008-02-07 20:16:08 cvskris Exp $
________________________________________________________________________

-*/

#include "inituiprestackprocessing.h"

#include "uiprestackagc.h"
#include "uiprestackmute.h"
#include "uiprestackverticalstack.h"

void uiPreStackProcessing::initStdClasses()
{
    PreStack::uiAGC::initClass();
    PreStack::uiMute::initClass();
    PreStack::uiVerticalStack::initClass();
}
