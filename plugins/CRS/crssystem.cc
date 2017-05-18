/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : April 2017
-*/


#include "crssystem.h"
#include "iopar.h"

static const char* sKeyProjectionID = "Projection.ID";

Coords::ProjectionBasedSystem::ProjectionBasedSystem()
    : proj_(0)
{}


Coords::ProjectionBasedSystem::ProjectionBasedSystem( ProjectionID projid )
    : proj_(0)
{
    setProjection( projid );
}


Coords::PositionSystem* Coords::ProjectionBasedSystem::clone() const
{
    Coords::ProjectionBasedSystem* cp = new Coords::ProjectionBasedSystem;
    cp->proj_ = proj_;
    return cp;
}


BufferString Coords::ProjectionBasedSystem::summary() const
{
    if ( !proj_ )
	return "No Projection Selected";

    BufferString ret( "P: [" );
    ret.add( proj_->id().getI() ).add( "] " ).add( proj_->userName() );
    return ret;
}


bool Coords::ProjectionBasedSystem::isOK() const
{ return proj_ && proj_->isOK(); }

bool Coords::ProjectionBasedSystem::geographicTransformOK() const
{ return isOK(); }

LatLong Coords::ProjectionBasedSystem::toGeographic(
					const Coord& crd, bool wgs84 ) const
{
    if ( !isOK() )
	return LatLong::udf();

    return proj_->toGeographic( crd, wgs84 );
}


Coord Coords::ProjectionBasedSystem::fromGeographic(
					const LatLong& ll, bool wgs84 ) const
{
    if ( !isOK() )
	return Coord::udf();

    return proj_->fromGeographic( ll, wgs84 );
}


bool Coords::ProjectionBasedSystem::isOrthogonal() const
{ return !proj_ || proj_->isOrthogonal(); }

bool Coords::ProjectionBasedSystem::isFeet() const
{ return proj_ && proj_->isFeet(); }

bool Coords::ProjectionBasedSystem::isMeter() const
{ return !proj_ || proj_->isMeter(); }

bool Coords::ProjectionBasedSystem::usePar( const IOPar& par )
{
    if ( !PositionSystem::usePar(par) )
	return false;

    Coords::ProjectionID projid;
    if ( par.get(sKeyProjectionID,projid) )
	setProjection( projid );

    return true;
}


void Coords::ProjectionBasedSystem::fillPar( IOPar& par ) const
{
    PositionSystem::fillPar( par );
    if ( proj_ )
	par.set( sKeyProjectionID, proj_->id() );
}


bool Coords::ProjectionBasedSystem::setProjection( Coords::ProjectionID pid )
{
    const Coords::Projection* proj = Coords::Projection::getByID( pid );
    if ( !proj )
	return false;

    proj_ = proj;
    return true;
}


const Coords::Projection* Coords::ProjectionBasedSystem::getProjection() const
{ return proj_; }

