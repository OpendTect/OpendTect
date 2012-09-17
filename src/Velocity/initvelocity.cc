/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2008
-*/

static const char* rcsID = "$Id: initvelocity.cc,v 1.5 2011/08/23 14:51:33 cvsbert Exp $";


#include "moddepmgr.h"
#include "velocityfunctionvolume.h"
#include "velocityfunctionstored.h"


mDefModInitFn(Velocity)
{
    mIfNotFirstTime( return );

    Vel::VolumeFunctionSource::initClass();
    Vel::StoredFunctionSource::initClass();
}
