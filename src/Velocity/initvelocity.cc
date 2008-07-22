/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2008
-*/

static const char* rcsID = "$Id: initvelocity.cc,v 1.1 2008-07-22 17:39:21 cvskris Exp $";

#include "initvelocity.h"

#include "velocityfunctionvolume.h"

namespace Velocity
{

void initStdClasses()
{
    Vel::VolumeFunctionSource::initClass();
}

}; //namespace
