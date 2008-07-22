/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2008
-*/

static const char* rcsID = "$Id: inituivelocity.cc,v 1.1 2008-07-22 17:39:21 cvskris Exp $";

#include "initvelocity.h"

#include "uivelocityfunctionvolume.h"

namespace uiVelocity
{

void initStdClasses()
{
    Vel::uiVolumeFunction::initClass();
}

}; //namespace
