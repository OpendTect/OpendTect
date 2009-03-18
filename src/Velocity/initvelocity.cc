/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2008
-*/

static const char* rcsID = "$Id: initvelocity.cc,v 1.2 2009-03-18 18:45:26 cvskris Exp $";

#include "initvelocity.h"

#include "velocityfunctionvolume.h"
#include "velocityfunctionstored.h"

namespace Velocity
{

void initStdClasses()
{
    Vel::VolumeFunctionSource::initClass();
    Vel::StoredFunctionSource::initClass();
}

}; //namespace
