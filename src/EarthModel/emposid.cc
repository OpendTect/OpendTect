/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emposid.h"
#include "iopar.h"

namespace EM
{

const char* PosID::emobjStr() { return "Object"; }
const char* PosID::sectionStr() { return  "Patch"; }
const char* PosID::subidStr() { return  "Sub ID"; }


PosID::PosID( const ObjectID& id, const RowCol& rc )
    : emobjid_(id)
    , sectionid_(SectionID::def())
    , subid_(rc.toInt64())
{}


PosID::PosID( const ObjectID& id, SubID subid )
    : emobjid_(id)
    , sectionid_(SectionID::def())
    , subid_(subid)
{}


const PosID& PosID::udf()
{
    mDefineStaticLocalObject( PosID, undef,
	( ObjectID::udf(), SectionID::udf(), mUdf(SubID) ) );
    return undef;
}


void PosID::setUdf()
{
    *this = udf();
}


bool PosID::isUdf() const
{
    return objectID().isUdf();
}


bool PosID::isValid() const
{
    return emobjid_.isValid() && sectionid_.isValid() && !mIsUdf(subid_);
}


RowCol PosID::getRowCol() const
{
    return RowCol::fromInt64( subID() );
}


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
	sectionid_ = sCast( EM::SectionID, tmpsection );
	subid_ = tmpsubid;
    }

    return res;
}

} // namespace EM
