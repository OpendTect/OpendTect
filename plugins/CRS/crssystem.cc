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


Coords::ProjectionBasedSystem::ProjectionBasedSystem( const AuthorityCode& code)
{
    setProjection( code );
}


Coords::ProjectionBasedSystem::~ProjectionBasedSystem()
{
    delete proj_;
}


Coords::CoordSystem* Coords::ProjectionBasedSystem::clone() const
{
    auto* cp = new ProjectionBasedSystem;
    if ( proj_ )
	cp->setProjection( proj_->authcode_ );

    return cp;
}


Coords::CoordSystem* Coords::ProjectionBasedSystem::getGeodeticSystem() const
{
    if ( isLatLong() )
	return clone();

    auto* cp = new ProjectionBasedSystem;
    cp->setProjection( proj_ ? proj_->getGeodeticAuthCode()
			     : AuthorityCode::sWGS84AuthCode() );
    return cp;
}


Coords::CoordSystem* Coords::ProjectionBasedSystem::getWGS84LLSystem()
{
    return new ProjectionBasedSystem( AuthorityCode::sWGS84AuthCode() );
}


BufferString Coords::ProjectionBasedSystem::summary() const
{
    if ( !proj_ )
	return "No Projection Selected";

    BufferString ret;
    if ( isOrthogonal() )
	ret.set( "Projection" );
    else if ( isLatLong() )
	ret.set( "Geodetic" );
    else
	ret.set( "Other projection" );

    ret.add( ": " ).add( proj_->getProjDispString() );
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


Coord Coords::ProjectionBasedSystem::convertFrom( const Coord& incrd,
						  const CoordSystem& from) const
{
    mDynamicCastGet(const ProjectionBasedSystem*,fromproj,&from)
    if ( !fromproj || !isOK() || !fromproj->isOK() )
	return Coord::udf();

    return Projection::convert( incrd, *fromproj->getProjection(),
				*this->getProjection() );
}


Coord Coords::ProjectionBasedSystem::convertTo(const Coord& incrd,
						const CoordSystem& to) const
{
    mDynamicCastGet(const ProjectionBasedSystem*,toproj,&to)
    if ( !toproj || !isOK() || !toproj->isOK() )
	return Coord::udf();

    return Projection::convert( incrd, *this->getProjection(),
				*toproj->getProjection() );
}


bool Coords::ProjectionBasedSystem::isOrthogonal() const
{
    return proj_ ? proj_->isOrthogonal() : false;
}


bool Coords::ProjectionBasedSystem::isLatLong() const
{
    return proj_ ? proj_->isLatLong() : false;
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
	proj_->getGeodeticAuthCode() == AuthorityCode::sWGS84AuthCode();
}


bool Coords::ProjectionBasedSystem::doUsePar( const IOPar& par )
{
    BufferString authcodestr;
    if ( par.get(IOPar::compKey(sKey::Projection(),sKey::ID()),authcodestr) )
	setProjection( AuthorityCode::fromString(authcodestr) );

    return isOK();
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


bool Coords::ProjectionBasedSystem::setProjection( const AuthorityCode& code )
{
    proj_ = Projection::getByAuthCode( code );
    return proj_;
}


const Coords::Projection* Coords::ProjectionBasedSystem::getProjection() const
{
    return proj_;
}


BufferString Coords::ProjectionBasedSystem::toString( StringType strtype,
						      bool withsystem ) const
{
    BufferString res;
    if ( withsystem )
	res.set( factoryKeyword() ).addSpace();

    if ( strtype == Default || strtype == URN )
	res.add( getURNString().buf() );
    else if ( strtype == WKT )
	res.add( getWKTString().buf() );
    else if ( strtype == JSON )
	res.add( getJSONString().buf() );
    else if ( strtype == URL )
	res.add( getUrl().buf() );

    return res;
}


RefMan<Coords::CoordSystem> Coords::ProjectionBasedSystem::fromString(
				    const char* str, BufferString* msg ) const
{
    BufferString defstr( str );
    if ( defstr.startsWith(factoryKeyword()) )
	defstr.remove( factoryKeyword() ).trimBlanks();

    BufferString emsg;
    Projection* proj = Projection::fromString( defstr.buf(), emsg );
    if ( !proj || !proj->isOK() )
    {
	delete proj;
	if ( msg )
	    msg->set( emsg.buf() );

	return nullptr;
    }

    RefMan<CoordSystem> res = new ProjectionBasedSystem( proj->authCode() );
    return res;
}


BufferString Coords::ProjectionBasedSystem::getURNString() const
{
    return proj_ ? proj_->authCode().toURNString() : BufferString::empty();
}


BufferString Coords::ProjectionBasedSystem::getWKTString() const
{
    return proj_ ? proj_->getWKTString() : BufferString::empty();
}


BufferString Coords::ProjectionBasedSystem::getJSONString() const
{
    return proj_ ? proj_->getJSONString() : BufferString::empty();
}


BufferString Coords::ProjectionBasedSystem::getUrl() const
{
    return proj_ ? proj_->getURL() : BufferString::empty();
}
