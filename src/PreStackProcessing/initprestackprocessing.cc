/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          December 2007
 RCS:           $Id: initprestackprocessing.cc,v 1.2 2008-02-05 20:23:55 cvskris Exp $
________________________________________________________________________

-*/

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
