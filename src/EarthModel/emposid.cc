/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emposid.cc,v 1.12 2004-12-17 13:31:02 nanne Exp $";

#include "emposid.h"
#include "iopar.h"
#include "geomgridsurface.h"


const char* EM::PosID::emobjStr() { return "Object"; }
const char* EM::PosID::sectionStr() { return  "Patch"; }
const char* EM::PosID::subidStr() { return  "Sub ID"; }


RowCol EM::PosID::getRowCol() const
{
    return int642rc(subID());
}


void EM::PosID::fillPar( IOPar& par ) const
{
    par.set( emobjStr(), emobj );
    par.set( sectionStr(), section );
    par.set( subidStr(), subid );
}


bool EM::PosID::usePar( const IOPar& par )
{
    int tmpsection;
    SubID tmpsubid;
    const bool res = par.get( emobjStr(), emobj ) &&
		     par.get( sectionStr(), tmpsection ) &&
		     par.get( subidStr(), tmpsubid );
    if ( res )
    {
	section = tmpsection;
	subid = tmpsubid;
    }

    return res;
}
