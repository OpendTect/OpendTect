/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "staticsdesc.h"

#include "iopar.h"
#include "separstr.h"
#include "stringview.h"

const char* StaticsDesc::sKeyHorizon()	{ return "Statics Horizon"; }
const char* StaticsDesc::sKeyVelocity()	{ return "Statics Velocity"; }
const char* StaticsDesc::sKeyVelocityAttrib()
{ return "Statics Velocity Attrib"; }


StaticsDesc::StaticsDesc()
    : vel_( mUdf(float) )
{}


void StaticsDesc::fillPar( IOPar& par ) const
{
    if ( horizon_.isUdf() )
	removePars( par );
    else
    {
	par.set( sKeyHorizon(), horizon_ );
	par.set( sKeyVelocity(), vel_ );
	par.set( sKeyVelocityAttrib(), velattrib_ );
    }
}


bool StaticsDesc::operator==( const StaticsDesc& oth ) const
{
    if ( &oth == this )
	return true;

    if ( horizon_!=oth.horizon_ )
	return false;

    if ( velattrib_!=oth.velattrib_ )
	return false;

    if ( velattrib_.isEmpty() )
	return mIsEqual(vel_,oth.vel_,1e-3);

    return true;
}


bool StaticsDesc::operator!=( const StaticsDesc& oth ) const
{ return !(*this==oth); }


void StaticsDesc::removePars( IOPar& par )
{
    par.removeWithKey( sKeyHorizon() );
    par.removeWithKey( sKeyVelocity() );
    par.removeWithKey( sKeyVelocityAttrib() );
}


bool StaticsDesc::usePar( const IOPar& par )
{
    horizon_.setUdf();
    vel_ = mUdf(float);
    par.get( sKeyHorizon(), horizon_ );
    if ( horizon_.isUdf() )
	return true;

    par.get( sKeyVelocityAttrib(), velattrib_ );
    par.get( sKeyVelocity(), vel_ );

    return true;
}
