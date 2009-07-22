/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2008
-*/

static const char* rcsID = "$Id: initvelocity.cc,v 1.3 2009-07-22 16:01:35 cvsbert Exp $";

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
