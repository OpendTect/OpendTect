/*+
________________________________________________________________________

Copyright:	(C) 1995-2022 dGB Beheer B.V.
License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "locationbase.h"

#include "survinfo.h"
#include "trigonometry.h"
#include "posimpexppars.h"

#define mInitTrcKey \
    setGeomID( geomid ); \
    trckey_.setFrom( pos_ );

LocationBase::LocationBase( double x, double y, double zval,
						    const Pos::GeomID& geomid )
    : pos_(x,y,zval)
{
    mInitTrcKey
}


LocationBase::LocationBase( const Coord& c, double zval,
						const Pos::GeomID& geomid )
    : pos_(c,zval)
{
    mInitTrcKey
}


LocationBase::LocationBase( const Coord3& c, const Pos::GeomID& geomid )
    : pos_(c)
{
    mInitTrcKey
}


LocationBase::LocationBase( const LocationBase& oth )
{
    *this = oth;
}


LocationBase::~LocationBase()
{
}


bool LocationBase::operator ==( const LocationBase& oth ) const
{
    return pos_ == oth.pos_;
}


bool LocationBase::operator !=( const LocationBase& oth ) const
{
    return !(*this == oth);
}


void LocationBase::operator =( const LocationBase& oth )
{
    if ( this != &oth )
    {
	pos_ = oth.pos_;
	trckey_ = oth.trckey_;
    }
}

bool LocationBase::hasPos() const
{
    return pos_.isDefined();
}


bool LocationBase::hasTrcKey() const
{
    return !mIsUdf(trckey_.inl()) && !mIsUdf(trckey_.crl())
	&& !trckey_.isSynthetic();
}


bool LocationBase::is2D() const
{
    return trcKey().is2D();
}


const Coord3& LocationBase::pos() const
{
    return pos_;
}


float LocationBase::z() const
{
    return (float)pos_.z;
}


void LocationBase::setPos( const Coord3& crd )
{
    pos_ = crd;
}


void LocationBase::setPos( const Coord& crd )
{
    pos_.x = crd.x;
    pos_.y = crd.y;
}

void LocationBase::setPos( double x,double y,double zval )
{
    setPos( Coord3(x,y,zval) );
}


void LocationBase::setPos( const Coord& c,float zval )
{
    setPos( c.x, c.y, zval );
}


const TrcKey& LocationBase::trcKey() const
{
    if ( hasTrcKey() || !hasPos() )
	return trckey_;

    mDefineStaticLocalObject( TrcKey, rettk, );
    rettk.setPosition( SI().transform(pos_) );
    return rettk;
}


OD::GeomSystem LocationBase::geomSystem() const
{
    return trcKey().geomSystem();
}


Pos::GeomID LocationBase::geomID() const
{
    return trcKey().geomID();
}


Pos::TraceID LocationBase::lineNr() const
{
    return trcKey().lineNr();
}


Pos::TraceID LocationBase::trcNr() const
{
    return trcKey().trcNr();
}


const BinID& LocationBase::binID() const
{
    return trcKey().position();
}


void LocationBase::setTrcKey( const TrcKey& tk )
{
    trckey_ = tk;
}


void LocationBase::setLineNr( Pos::LineID lnr )
{
    trckey_.setLineNr( lnr );
}


void LocationBase::setTrcNr( Pos::TraceID tnr )
{
    trckey_.setTrcNr( tnr );
}


void LocationBase::setGeomID( const Pos::GeomID& geomid )
{
    trckey_.setGeomID( geomid );
}


void LocationBase::setBinID( const BinID& bid, bool updcoord )
{
    trckey_.setPosition( bid );
    if ( updcoord )
	setPos( trckey_.getCoord() );
}


void LocationBase::setGeomSystem( OD::GeomSystem gs, bool updpos )
{
    trckey_.setGeomSystem( gs );
    if ( updpos )
	trckey_.setFrom( pos_ );
}
