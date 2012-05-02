/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2008
-*/

static const char* rcsID mUnusedVar = "$Id: initvelocity.cc,v 1.7 2012-05-02 15:11:53 cvskris Exp $";


#include "moddepmgr.h"
#include "velocityfunctionvolume.h"
#include "velocityfunctionstored.h"


mDefModInitFn(Velocity)
{
    mIfNotFirstTime( return );

    Vel::VolumeFunctionSource::initClass();
    Vel::StoredFunctionSource::initClass();
}
