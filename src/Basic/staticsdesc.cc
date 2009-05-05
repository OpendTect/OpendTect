/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Sep 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: staticsdesc.cc,v 1.1 2009-05-05 21:00:00 cvskris Exp $";


#include "staticsdesc.h"

#include "iopar.h"
#include "separstr.h"
#include "fixedstring.h"

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

    if ( !horizon_.isEmpty() )
    {
	par.set( sKeyVelocity(), vel_ );
	par.set( sKeyVelocityAttrib(), velattrib_ );
    }
}


void StaticsDesc::removePars( IOPar& par )
{
    par.remove( sKeyHorizon() );
    par.remove( sKeyVelocity() );
    par.remove( sKeyVelocityAttrib() );
}


bool StaticsDesc::usePar( const IOPar& par )
{
    if  ( !par.get( sKeyHorizon(), horizon_ ) )
	return false;

    par.get( sKeyVelocityAttrib(), velattrib_ );
    par.get( sKeyVelocity(), vel_ );

    return true;
}
