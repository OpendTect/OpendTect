/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initprestackprocessing.cc,v 1.5 2009-11-25 22:22:54 cvskris Exp $";

#include "initprestackprocessing.h"

#include "prestackagc.h"
#include "prestackmute.h"
#include "prestacklateralstack.h"

void PreStackProcessing::initStdClasses()
{
    PreStack::Mute::initClass();
    PreStack::AGC::initClass();
    PreStack::LateralStack::initClass();
}
