/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initprestackprocessing.cc,v 1.7 2011-08-23 06:54:11 cvsbert Exp $";

#include "initprestackprocessing.h"

#include "prestackagc.h"
#include "prestackmute.h"
#include "prestacklateralstack.h"
#include "prestackanglemute.h"

void PreStackProcessing::initStdClasses()
{
    mIfNotFirstTime( return );

    PreStack::Mute::initClass();
    PreStack::AGC::initClass();
    PreStack::LateralStack::initClass();
    PreStack::AngleMute::initClass();
}
