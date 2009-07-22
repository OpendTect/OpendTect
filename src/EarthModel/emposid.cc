/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emposid.cc,v 1.16 2009-07-22 16:01:31 cvsbert Exp $";

#include "emposid.h"
#include "iopar.h"

using namespace EM;

const char* PosID::emobjStr() { return "Object"; }
const char* PosID::sectionStr() { return  "Patch"; }
const char* PosID::subidStr() { return  "Sub ID"; }


const PosID& PosID::udf()
{
    static PosID undef( -1, -1, -1 );
    return undef; 
}


bool PosID::isUdf() const { return objectID()==-1; }


RowCol PosID::getRowCol() const
{
    return int642rc(subID());
}


void PosID::fillPar( IOPar& par ) const
{
    par.set( emobjStr(), emobj );
    par.set( sectionStr(), section );
    par.set( subidStr(), subid );
}


bool PosID::usePar( const IOPar& par )
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
