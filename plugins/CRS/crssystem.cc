/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "crssystem.h"
#include "iopar.h"
#include "unitofmeasure.h"


Coords::ProjectionBasedSystem::ProjectionBasedSystem()
{
}


Coords::ProjectionBasedSystem::ProjectionBasedSystem( AuthorityCode code )
{
    setProjection( code );
}


Coords::ProjectionBasedSystem::~ProjectionBasedSystem()
{
    delete proj_;
}


Coords::CoordSystem* Coords::ProjectionBasedSystem::clone() const
{
    Coords::ProjectionBasedSystem* cp = new Coords::ProjectionBasedSystem;
    if ( proj_ )
	cp->setProjection( proj_->authcode_ );

    return cp;
}


Coords::CoordSystem* Coords::ProjectionBasedSystem::getGeodeticSystem() const
{
    if ( !isOrthogonal() )
	return clone();

    Coords::ProjectionBasedSystem* cp = new Coords::ProjectionBasedSystem;
    cp->setProjection( proj_ ? proj_->getGeodeticAuthCode()
			     : Coords::AuthorityCode::sWGS84AuthCode() );
    return cp;
}


Coords::CoordSystem* Coords::ProjectionBasedSystem::getWGS84LLSystem()
{
    return new Coords::ProjectionBasedSystem(
				Coords::AuthorityCode::sWGS84AuthCode() );
}


BufferString Coords::ProjectionBasedSystem::summary() const
{
    if ( !proj_ )
	return "No Projection Selected";

    BufferString ret( isOrthogonal() ? "Projection: " : "Geodetic: " );
    ret.add( proj_->getProjDispString() );
    return ret;
}


bool Coords::ProjectionBasedSystem::isOK() const
{
    return proj_;
}

bool Coords::ProjectionBasedSystem::geographicTransformOK() const
{
    return isOK();
}

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


Coord Coords::ProjectionBasedSystem::convertFrom(const Coord& incrd,
						const CoordSystem& from) const
{
    mDynamicCastGet(const Coords::ProjectionBasedSystem*,fromproj,&from)
    if ( !fromproj || !isOK() || !fromproj->isOK() )
	return Coord::udf();

    return Projection::convert( incrd, *fromproj->getProjection(),
				*this->getProjection() );
}


Coord Coords::ProjectionBasedSystem::convertTo(const Coord& incrd,
						const CoordSystem& to) const
{
    mDynamicCastGet(const Coords::ProjectionBasedSystem*,toproj,&to)
    if ( !toproj || !isOK() || !toproj->isOK() )
	return Coord::udf();

    return Projection::convert( incrd, *this->getProjection(),
				*toproj->getProjection() );
}


bool Coords::ProjectionBasedSystem::isOrthogonal() const
{
    return !proj_ || proj_->isOrthogonal();
}


bool Coords::ProjectionBasedSystem::isFeet() const
{
    return proj_ && proj_->isFeet();
}


bool Coords::ProjectionBasedSystem::isMeter() const
{
    return !proj_ || proj_->isMeter();
}


BufferString Coords::ProjectionBasedSystem::getUnitName() const
{
    return UnitOfMeasure::getUnitLbl( proj_->getUOM() );
}


bool Coords::ProjectionBasedSystem::isWGS84() const
{
    return !proj_ ||
	proj_->getGeodeticAuthCode() == Coords::AuthorityCode::sWGS84AuthCode();
}


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
    proj_ = Coords::Projection::getByAuthCode( code );
    return proj_;
}


const Coords::Projection* Coords::ProjectionBasedSystem::getProjection() const
{
    return proj_;
}


BufferString Coords::ProjectionBasedSystem::getURNString() const
{
    if ( !proj_ )
	return BufferString::empty();

    Coords::AuthorityCode code = proj_->authcode_;
    return code.toURNString();
}
