/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emposid.cc,v 1.1 2003-04-22 11:00:00 kristofer Exp $";

#include "emposid.h"
#include "iopar.h"


const char* EarthModel::PosID::emobjstr ="Object";
const char* EarthModel::PosID::partstr = "Patch";
const char* EarthModel::PosID::subidstr = "Sub ID";


void EarthModel::PosID::fillPar( IOPar& par ) const
{
    par.set( emobjstr, emobj );
    par.set( partstr, part );
    par.set( subidstr, (long long) subid );
}


bool  EarthModel::PosID::usePar( const IOPar& par )
{
    int tmppart;
    long long tmpsubid;
    const bool res =par.get( emobjstr, emobj ) &&
		    par.get( partstr, tmppart ) &&
		    par.get( subidstr, tmpsubid );
    if ( res )
    {
	part = tmppart;
	subid = tmpsubid;
    }

    return res;
}
