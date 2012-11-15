/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUsedVar = "$Id$";

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
{ return RowCol::fromInt64( subID() ); }


void PosID::fillPar( IOPar& par ) const
{
    par.set( emobjStr(), emobjid_ );
    par.set( sectionStr(), sectionid_ );
    par.set( subidStr(), subid_ );
}


bool PosID::usePar( const IOPar& par )
{
    int tmpsection = mUdf(int);
    SubID tmpsubid = mUdf(od_int64);
    const bool res = par.get( emobjStr(), emobjid_ ) &&
		     par.get( sectionStr(), tmpsection ) &&
		     par.get( subidStr(), tmpsubid );
    if ( res )
    {
	sectionid_ = tmpsection;
	subid_ = tmpsubid;
    }

    return res;
}
