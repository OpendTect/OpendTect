/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : April 2017
-*/


#include "crssystem.h"
#include "iopar.h"


Coords::ProjectionBasedSystem::ProjectionBasedSystem()
    : proj_(0)
{}


Coords::ProjectionBasedSystem::ProjectionBasedSystem(
					    Coords::AuthorityCode code )
    : proj_(0)
{
    setProjection( code );
}


Coords::CoordSystem* Coords::ProjectionBasedSystem::clone() const
{
    Coords::ProjectionBasedSystem* cp = new Coords::ProjectionBasedSystem;
    cp->proj_ = proj_;
    return cp;
}


BufferString Coords::ProjectionBasedSystem::summary() const
{
    if ( !proj_ )
	return "No Projection Selected";

    BufferString ret( "Projection: [" );
    ret.add( proj_->authCode().toString() ).add( "] " ).add( proj_->userName());
    return ret;
}


bool Coords::ProjectionBasedSystem::isOK() const
{ return proj_; }

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


bool Coords::ProjectionBasedSystem::doUsePar( const IOPar& par )
{
    BufferString authcodestr;
    if ( par.get(IOPar::compKey(sKey::Projection(),sKey::ID()),authcodestr) )
	setProjection( Coords::AuthorityCode::fromString(authcodestr) );

    return true;
}


void Coords::ProjectionBasedSystem::doFillPar( IOPar& par ) const
{
    if ( !proj_ )
	return;

    par.set( IOPar::compKey(sKey::Projection(),sKey::ID()),
	     proj_->authCode().toString() );
    par.set( IOPar::compKey(sKey::Projection(),sKey::Name()),
	     proj_->userName() );
}


bool Coords::ProjectionBasedSystem::setProjection( Coords::AuthorityCode code )
{
    const Coords::Projection* proj = Coords::Projection::getByAuthCode( code );
    if ( !proj )
	return false;

    proj_ = proj;
    return true;
}


const Coords::Projection* Coords::ProjectionBasedSystem::getProjection() const
{ return proj_; }
