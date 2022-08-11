/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Sep 2007
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
    par.set( sKeyHorizon(), horizon_ );

    if ( !horizon_.isUdf() )
    {
	par.set( sKeyVelocity(), vel_ );
	par.set( sKeyVelocityAttrib(), velattrib_ );
    }
}


bool StaticsDesc::operator==( const StaticsDesc& b ) const
{
    if ( horizon_!=b.horizon_ )
	return false;

    if ( velattrib_!=b.velattrib_ )
	return false;

    if ( velattrib_.isEmpty() )
	return mIsEqual( vel_, b.vel_, 1e-3 );

    return true;
}


bool StaticsDesc::operator!=( const StaticsDesc& b ) const
{ return !(*this==b); }


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
