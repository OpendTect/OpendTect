/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emposid.cc,v 1.10 2004-10-05 17:34:33 kristofer Exp $";

#include "emposid.h"
#include "iopar.h"
#include "geomgridsurface.h"


const char* EM::PosID::emobjstr ="Object";
const char* EM::PosID::sectionstr = "Patch";
const char* EM::PosID::subidstr = "Sub ID";


RowCol EM::PosID::getRowCol() const
{
    return longlong2rc(subID());
}


void EM::PosID::fillPar( IOPar& par ) const
{
    par.set( emobjstr, emobj );
    par.set( sectionstr, section );
    par.set( subidstr, subid );
}


bool EM::PosID::usePar( const IOPar& par )
{
    int tmpsection;
    long long tmpsubid;
    const bool res = par.get( emobjstr, emobj ) &&
		     par.get( sectionstr, tmpsection ) &&
		     par.get( subidstr, tmpsubid );
    if ( res )
    {
	section = tmpsection;
	subid = tmpsubid;
    }

    return res;
}
