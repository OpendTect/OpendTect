/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituiprestackprocessing.cc,v 1.4 2009-07-22 16:01:41 cvsbert Exp $";

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
