/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emposid.cc,v 1.6 2003-08-25 06:56:17 kristofer Exp $";

#include "emposid.h"
#include "iopar.h"
#include "geomgridsurface.h"


const char* EM::PosID::emobjstr ="Object";
const char* EM::PosID::patchstr = "Patch";
const char* EM::PosID::subidstr = "Sub ID";


RowCol EM::PosID::getRowCol() const
{
    return longlong2rc(subID());
}


void EM::PosID::fillPar( IOPar& par ) const
{
    par.set( emobjstr, emobj );
    par.set( patchstr, patch );
    par.set( subidstr, subid );
}


bool EM::PosID::usePar( const IOPar& par )
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
