/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emposid.cc,v 1.13 2005-01-10 15:36:47 kristofer Exp $";

#include "emposid.h"
#include "iopar.h"
#include "geomgridsurface.h"


const char* EM::PosID::emobjStr() { return "Object"; }
const char* EM::PosID::sectionStr() { return  "Patch"; }
const char* EM::PosID::subidStr() { return  "Sub ID"; }


const EM::PosID& EM::PosID::udf()
{
    static EM::PosID undef( -1, -1, -1 );
    return undef; 
}


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
