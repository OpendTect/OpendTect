/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituiprestackprocessing.cc,v 1.6 2011-01-27 22:48:47 cvsyuancheng Exp $";

#include "inituiprestackprocessing.h"

#include "uiprestackagc.h"
#include "uiprestackmute.h"
#include "uiprestacklateralstack.h"
#include "uiprestackanglemute.h"

void uiPreStackProcessing::initStdClasses()
{
    PreStack::uiAGC::initClass();
    PreStack::uiMute::initClass();
    PreStack::uiLateralStack::initClass();
    PreStack::uiAngleMute::initClass();
}
