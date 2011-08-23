/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2008
-*/

static const char* rcsID = "$Id: initvelocity.cc,v 1.4 2011-08-23 06:54:11 cvsbert Exp $";

#include "initvelocity.h"

#include "velocityfunctionvolume.h"
#include "velocityfunctionstored.h"

namespace Velocity
{

void initStdClasses()
{
    mIfNotFirstTime( return );

    Vel::VolumeFunctionSource::initClass();
    Vel::StoredFunctionSource::initClass();
}

}; //namespace
