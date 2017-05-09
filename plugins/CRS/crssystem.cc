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


bool Coords::ProjectionBasedSystem::isOK() const
{ return proj_ && proj_->isOK(); }

bool Coords::ProjectionBasedSystem::geographicTransformOK() const
{ return isOK(); }

LatLong Coords::ProjectionBasedSystem::toGeographicWGS84(
						const Coord& crd ) const
{
    if ( !isOK() )
	return LatLong::udf();

    return proj_->toGeographicWGS84( crd );
}


Coord Coords::ProjectionBasedSystem::fromGeographicWGS84(
						const LatLong& ll ) const
{
    if ( !isOK() )
	return Coord::udf();

    return proj_->fromGeographicWGS84( ll );
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
