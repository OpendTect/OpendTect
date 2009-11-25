/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituiprestackprocessing.cc,v 1.5 2009-11-25 22:22:54 cvskris Exp $";

#include "inituiprestackprocessing.h"

#include "uiprestackagc.h"
#include "uiprestackmute.h"
#include "uiprestacklateralstack.h"

void uiPreStackProcessing::initStdClasses()
{
    PreStack::uiAGC::initClass();
    PreStack::uiMute::initClass();
    PreStack::uiLateralStack::initClass();
}
