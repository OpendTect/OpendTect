/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          December 2007
 RCS:           $Id: inituiprestackprocessing.cc,v 1.1 2007-12-06 20:05:45 cvskris Exp $
________________________________________________________________________

-*/

#include "inituiprestackprocessing.h"

#include "uiprestackagc.h"
#include "uiprestackmute.h"

void uiPreStackProcessing::initStdClasses()
{
    PreStack::uiAGC::initClass();
    PreStack::uiMute::initClass();
}
