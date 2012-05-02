/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2008
-*/

static const char* mUnusedVar rcsID = "$Id: initvelocity.cc,v 1.6 2012-05-02 11:53:29 cvskris Exp $";


#include "moddepmgr.h"
#include "velocityfunctionvolume.h"
#include "velocityfunctionstored.h"


mDefModInitFn(Velocity)
{
    mIfNotFirstTime( return );

    Vel::VolumeFunctionSource::initClass();
    Vel::StoredFunctionSource::initClass();
}
