/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emposid.cc,v 1.2 2003-05-05 12:06:31 kristofer Exp $";

#include "emposid.h"
#include "iopar.h"


const char* EarthModel::PosID::emobjstr ="Object";
const char* EarthModel::PosID::patchstr = "Patch";
const char* EarthModel::PosID::subidstr = "Sub ID";


void EarthModel::PosID::fillPar( IOPar& par ) const
{
    par.set( emobjstr, emobj );
    par.set( patchstr, patch );
    par.set( subidstr, (long long) subid );
}


bool  EarthModel::PosID::usePar( const IOPar& par )
{
    int tmppatch;
    long long tmpsubid;
    const bool res =par.get( emobjstr, emobj ) &&
		    par.get( patchstr, tmppatch ) &&
		    par.get( subidstr, tmpsubid );
    if ( res )
    {
	patch = tmppatch;
	subid = tmpsubid;
    }

    return res;
}
