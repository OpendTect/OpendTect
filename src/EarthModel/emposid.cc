/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emposid.cc,v 1.9 2004-07-23 12:54:49 kristofer Exp $";

#include "emposid.h"
#include "iopar.h"
#include "geomgridsurface.h"


const char* EM::PosID::emobjstr ="Object";
const char* EM::PosID::sectionstr = "Patch";
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
