/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initprestackprocessing.cc,v 1.4 2009-07-22 16:01:34 cvsbert Exp $";

#include "initprestackprocessing.h"

#include "prestackagc.h"
#include "prestackmute.h"
#include "prestackverticalstack.h"

void PreStackProcessing::initStdClasses()
{
    PreStack::Mute::initClass();
    PreStack::AGC::initClass();
    PreStack::VerticalStack::initClass();
}
