/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emposid.cc,v 1.8 2004-07-14 15:31:08 nanne Exp $";

#include "emposid.h"
#include "iopar.h"
#include "geomgridsurface.h"


const char* EM::PosID::emobjstr ="Object";
const char* EM::PosID::patchstr = "Patch";
const char* EM::PosID::subidstr = "Sub ID";


void EM::PosID::setSubID( const RowCol& rc )
{
    subid = rc2longlong(rc);
}


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
    const bool res = par.get( emobjstr, emobj ) &&
		     par.get( patchstr, tmppatch ) &&
		     par.get( subidstr, tmpsubid );
    if ( res )
    {
	patch = tmppatch;
	subid = tmpsubid;
    }

    return res;
}
