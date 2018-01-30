/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/


#include "emposid.h"
#include "iopar.h"

namespace EM
{

const char* PosID::emobjStr() { return "Object"; }
const char* PosID::sectionStr() { return  "Patch"; }
const char* PosID::posidStr() { return  "Sub ID"; }


const PosID& PosID::getInvalid()
{
    mDefineStaticLocalObject( PosID, undef,
	    ( DBKey::getInvalid(), -1, PosID::getInvalid() ) );
    return undef;
}


bool PosID::isUdf() const { return objectID().isInvalid(); }


RowCol PosID::getRowCol() const
{ return PosID::getRowCol(); }


void PosID::fillPar( IOPar& par ) const
{
    par.set( emobjStr(), objid_ );
    par.set( sectionStr(), sectionid_ );
    par.set( posidStr(), getI() );
}


bool PosID::usePar( const IOPar& par )
{
    int tmpsection = mUdf(int);
    od_int64 tmpposid = mUdf(od_int64);
    const bool res = par.get( emobjStr(), objid_ ) &&
		     par.get( sectionStr(), tmpsection ) &&
		     par.get( posidStr(), tmpposid );
    if ( res )
    {
	sectionid_ = mCast( EM::SectionID, tmpsection );
	setI( tmpposid );
    }

    return res;
}

} // namespace EM
