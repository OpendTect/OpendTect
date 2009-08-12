/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Aug 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: visbeachball.cc,v 1.1 2009-08-12 09:59:19 cvskarthika Exp $";

#include "visbeachball.h"

#include "iopar.h"

#include "SoBeachBall.h"

mCreateFactoryEntry( visBase::BeachBall );

namespace visBase
{

const char* BeachBall::radiusstr()	{ return "Radius"; }


BeachBall::BeachBall()
    : VisualObjectImpl( false )
    , ball_(0)
{
}


BeachBall::~BeachBall()
{
}


void BeachBall::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    par.set( radiusstr(), radius() );
}


int BeachBall::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    float rd = radius();
    par.get( radiusstr(), rd );
    setRadius( rd );

    return 1;
}


float BeachBall::radius() const
{
    return 1;
}

void BeachBall::setRadius( float r )
{
}


Coord3 BeachBall::position() const
{
    return Coord3(0,0,0);
}

void BeachBall::setPosition( Coord3 c )
{
}

} // namespace visBase
