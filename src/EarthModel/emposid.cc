/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emposid.cc,v 1.5 2003-07-10 09:55:00 kristofer Exp $";

#include "emposid.h"
#include "iopar.h"
#include "geomgridsurface.h"


const char* EM::PosID::emobjstr ="Object";
const char* EM::PosID::patchstr = "Patch";
const char* EM::PosID::subidstr = "Sub ID";


RowCol EM::PosID::getRowCol() const
{
    return long2rc(subID());
}


void EM::PosID::fillPar( IOPar& par ) const
{
    par.set( emobjstr, emobj );
    par.set( patchstr, patch );
    par.set( subidstr, (long long) subid );
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
