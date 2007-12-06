/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          December 2007
 RCS:           $Id: initprestackprocessing.cc,v 1.1 2007-12-06 20:05:45 cvskris Exp $
________________________________________________________________________

-*/

#include "initprestackprocessing.h"

#include "prestackagc.h"
#include "prestackmute.h"

void PreStackProcessing::initStdClasses()
{
    PreStack::Mute::initClass();
    PreStack::AGC::initClass();
}
