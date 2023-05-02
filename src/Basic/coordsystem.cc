/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "coordsystem.h"

#include "iopar.h"
#include "separstr.h"
#include "keystrs.h"

static const BufferString coordsysfactorynm_( "System name" );
static const BufferString coordsysusrnm_( "Description" );
const char* Coords::CoordSystem::sKeyURN() { return "urnstring"; }
const char* Coords::CoordSystem::sKeyWKT() { return "wktstring"; }
const char* Coords::CoordSystem::sKeyJSON() { return "jsonstring"; }
const char* Coords::CoordSystem::sKeyURL() { return "urlstring"; }
const char* Coords::CoordSystem::sKeyFactoryName() { return coordsysfactorynm_;}
const char* Coords::CoordSystem::sKeyUiName() { return coordsysusrnm_; }

static const double cAvgEarthRadius = 6367450;
static const double latdist = cAvgEarthRadius*mDeg2RadD;

mImplFactory( Coords::CoordSystem, Coords::CoordSystem::factory );

using namespace Coords;

static Threads::Lock systemreposlock;
static ManagedObjectSet<IOPar> systemrepos;


static void reloadRepository( CallBacker* )
{
    Threads::Locker lock( systemreposlock );
    systemrepos.erase();

    //Todo
}

CoordSystem::~CoordSystem()
{}

bool CoordSystem::operator==( const CoordSystem& oth ) const
{
    if ( &oth == this )
	return true;

    IOPar myiop; fillPar( myiop );
    IOPar othiop; oth.fillPar( othiop );
    return myiop == othiop;
}


bool CoordSystem::operator!=( const CoordSystem& oth ) const
{
    return !(*this == oth);
}


void CoordSystem::initRepository( NotifierAccess* na )
{
    reloadRepository( nullptr );

    if ( na )
	na->notify( mSCB( reloadRepository ) );
    //Don't do remove, as we assume we will be destroyed
    //after NotifierAccess's last call
}


void CoordSystem::getSystemNames( bool orthogonalonly, bool projectiononly,
				uiStringSet& strings, ObjectSet<IOPar>& pars )
{
    deepErase( pars );
    strings.setEmpty();

    //Add all factory entries
    const BufferStringSet factorynames = factory().getNames();
    const uiStringSet factoryuinames = factory().getUserNames();

    for ( int idx=0; idx<factorynames.size(); idx++ )
    {
	mDeclareAndTryAlloc( PtrMan<IOPar>, systempar, IOPar );
	if ( !systempar ) //out of memory
	    continue;

	systempar->set( sKeyFactoryName(), factorynames.get(idx) );
	systempar->set( sKeyUiName(), factoryuinames[idx] );

	if ( orthogonalonly || projectiononly )
	{
	    RefMan<CoordSystem> system = createSystem( *systempar );
	    if ( !system || ( orthogonalonly && !system->isOrthogonal() )
		    || ( projectiononly && !system->isProjection() ) )
		continue;
	}

	pars += systempar.release();
	strings += factoryuinames[idx];
    }

    //Add all repository entries
    Threads::Locker lock( systemreposlock );

    for ( int idx=0; idx<systemrepos.size(); idx++ )
    {
	PtrMan<IOPar> systempar = new IOPar( *systemrepos[idx] );
	RefMan<CoordSystem> system = createSystem( *systempar );
	if ( !system )
	    continue;

	if ( orthogonalonly && !system->isOrthogonal() )
	    continue;

	uiString uiname;
	BufferString factoryname;
	if ( !systempar->get( sKeyUiName(), uiname ) ||
	     !systempar->get( sKeyFactoryName(), factoryname ) )
	    continue;

	pars += systempar.release();
	strings += uiname;
    }
}


RefMan<CoordSystem> CoordSystem::createSystem( const IOPar& par )
{
    BufferString factorykey;
    if ( !par.get(sKeyFactoryName(),factorykey) )
	return nullptr;

    RefMan<CoordSystem> res = factory().create( factorykey );
    if ( !res )
	return nullptr;

    if ( !res->usePar(par) )
	return nullptr;

    return res;
}


RefMan<CoordSystem> CoordSystem::createSystem( const char* str,
					       BufferString& /* msg */ )
{
    RefMan<CoordSystem> res = factory().create( sKey::ProjSystem() );
    if ( !res )
	return nullptr;

    BufferString descstr( str );
    descstr.trimBlanks();
    const StringView type( descstr.startsWith("PROJCRS") ? sKeyWKT()
							 : sKeyURN() );

    IOPar iop;
    iop.set( sKeyFactoryName(), res->factoryKeyword() );
    iop.set( sKey::Type(), type.buf() );
    iop.set( sKey::Desc(), str );
    if ( !res->usePar(iop) )
	return nullptr;

    return res;
}


BufferString CoordSystem::getDescString( StringType strtype,
					 bool withsystem ) const
{
    BufferString type;
    if ( strtype == Default )
	type = sKey::Default();
    else if ( strtype == URN )
	type = sKeyURN();
    else if ( strtype == WKT )
	type = sKeyWKT();
    else if ( strtype == JSON )
	type = sKeyJSON();
    else if ( strtype == URL )
	type = sKeyURL();

    IOPar iop;
    iop.set( sKey::Type(), type.buf() );
    fillPar( iop );
    if ( !iop.isPresent(sKey::Desc()) )
	return BufferString::empty();

    BufferString ret;
    if ( withsystem )
	ret.add( factoryKeyword() ).addSpace();

    ret.add( iop.find(sKey::Desc()) );

    return ret;
}


Coord CoordSystem::convert( const Coord& in, const CoordSystem& from,
			       const CoordSystem& to )
{
    return from.convertTo( in, to );
}


Coord CoordSystem::convertFrom( const Coord& in,
				   const CoordSystem& from ) const
{
    const LatLong wgs84ll = LatLong::transform( in, true, &from );
    return LatLong::transform( wgs84ll, true, this );
}


Coord CoordSystem::convertTo( const Coord& in,
				   const CoordSystem& to ) const
{
    const LatLong wgs84ll = LatLong::transform( in, true, this );
    return LatLong::transform( wgs84ll, true, &to );
}


bool CoordSystem::usePar( const IOPar& par )
{
    BufferString nm;
    if ( !par.get(sKeyFactoryName(),nm) || nm != factoryKeyword() )
	return false;

    return doUsePar( par );
}


void CoordSystem::fillPar( IOPar& par ) const
{
    par.set( sKeyFactoryName(), factoryKeyword() );
    doFillPar( par );
}


uiString CoordSystem::toUiString( const Coord& crd ) const
{
    return ::toUiString( toString(crd,false) );
}


BufferString CoordSystem::toString( const Coord& crd, bool withsystem ) const
{
    BufferString res;
    const char* space = " ";
    if ( withsystem )
	res.add( factoryKeyword() ).add( space );

    res.add( ::toString(crd.x) ).add( space );
    res.add( ::toString(crd.y));

    return res;
}


Coord CoordSystem::fromString( const char* str ) const
{
    const SeparString sepstr( str, ' ' );
    const int nrparts = sepstr.size();

    if ( nrparts==3 ) //With coord system name first
    {
	const BufferString system = sepstr[0];
	if ( system != factoryKeyword() )
	    return Coord::udf();
    }
    else if ( nrparts<2 )
	return Coord::udf();

    const BufferString xstr = sepstr[nrparts-2];
    const BufferString ystr = sepstr[nrparts-1];

    return Coord( toDouble(xstr,mUdf(double)),
		  toDouble(ystr,mUdf(double)) );
}


namespace Coords
{
    BufferString wgs84dispstr_;
    CoordSystem* wgs84crsproj_ = nullptr;

    extern "C" { mGlobal(Basic) void SetWGS84(const char*,CoordSystem*); }
    mExternC(Basic) void SetWGS84( const char* dispstr, CoordSystem* crs )
    {
	wgs84dispstr_.set( dispstr );
	wgs84crsproj_ = crs;
    }
}


CoordSystem* CoordSystem::getWGS84LLSystem()
{
    return wgs84crsproj_;
}


BufferString CoordSystem::sWGS84ProjDispString()
{
    mDeclStaticString(ret);
    if ( ret.isEmpty() )
	ret.set( wgs84dispstr_ );
    return ret;
}



UnlocatedXY::UnlocatedXY()
{
}


CoordSystem* UnlocatedXY::clone() const
{
    UnlocatedXY* cp = new UnlocatedXY;
    cp->isfeet_ = isfeet_;
    return cp;
}


LatLong UnlocatedXY::toGeographic( const Coord&, bool ) const
{
    return LatLong::udf();
}


Coord UnlocatedXY::fromGeographic( const LatLong&, bool ) const
{
    return Coord::udf();
}


static const char* sKeyIsFeet = "XY in Feet";

bool UnlocatedXY::doUsePar( const IOPar& par )
{
    par.getYN( sKeyIsFeet, isfeet_ );
    return true;
}


void UnlocatedXY::doFillPar( IOPar& par ) const
{
    par.setYN( sKeyIsFeet, isfeet_ );
}


AnchorBasedXY::AnchorBasedXY()
    : lngdist_(mUdf(float))
{
}


AnchorBasedXY::AnchorBasedXY( const LatLong& l, const Coord& c )
{
    setLatLongEstimate( l, c );
}


CoordSystem* AnchorBasedXY::clone() const
{
    AnchorBasedXY* cp = new AnchorBasedXY( reflatlng_, refcoord_ );
    cp->isfeet_ = isfeet_;
    return cp;
}


bool AnchorBasedXY::geographicTransformOK() const
{ return !mIsUdf(lngdist_); }

BufferString AnchorBasedXY::summary() const
{
    BufferString ret( "Anchor: " );
    ret.add( reflatlng_.toString() ).add( refcoord_.toPrettyString() );
    return ret;
}


void AnchorBasedXY::setLatLongEstimate( const LatLong& ll, const Coord& c )
{
    refcoord_ = c; reflatlng_ = ll;
    lngdist_ = mDeg2RadD * cos( ll.lat_ * mDeg2RadD ) * cAvgEarthRadius;
}


LatLong AnchorBasedXY::toGeographic( const Coord& c, bool ) const
{
    if ( !geographicTransformOK() ) return reflatlng_;

    const double scalefac = isfeet_ ? mFromFeetFactorD : 1;

    Coord coorddist( (c.x - refcoord_.x) * scalefac,
		    (c.y - refcoord_.y) * scalefac );
    LatLong ll( reflatlng_.lat_ + coorddist.y / latdist,
	       reflatlng_.lng_ + coorddist.x / lngdist_ );

    if ( ll.lat_ > 90 )		ll.lat_ = 180 - ll.lat_;
    else if ( ll.lat_ < -90 )	ll.lat_ = -180 - ll.lat_;
    if ( ll.lng_ < -180 )	ll.lng_ = ll.lng_ + 360;
    else if ( ll.lng_ > 180 )	ll.lng_ = ll.lng_ - 360;

    return ll;
}


Coord AnchorBasedXY::fromGeographic( const LatLong& ll, bool ) const
{
    if ( !geographicTransformOK() ) return Coord::udf();

    const double scalefac = isfeet_ ? mFromFeetFactorD : 1;

    const LatLong latlongdist( ll.lat_ - reflatlng_.lat_,
			      ll.lng_ - reflatlng_.lng_ );
    return Coord( refcoord_.x + latlongdist.lng_ * lngdist_ / scalefac,
		 refcoord_.y + latlongdist.lat_ * latdist / scalefac );
}


static const char* sKeyRefLatLong = "Reference Lat/Long";
static const char* sKeyRefCoord = "Reference Coordinate";

bool AnchorBasedXY::doUsePar( const IOPar& par )
{
    Coord crd;
    LatLong latlong;
    par.getYN( sKeyIsFeet, isfeet_ );

    if ( par.get(sKeyRefLatLong,latlong.lat_,latlong.lng_) &&
	 par.get(sKeyRefCoord,crd) )
	setLatLongEstimate( latlong, crd );

    return true;
}


void AnchorBasedXY::doFillPar( IOPar& par ) const
{
    par.setYN( sKeyIsFeet, isfeet_ );
    par.set( sKeyRefLatLong, reflatlng_.lat_, reflatlng_.lng_ );
    par.set( sKeyRefCoord, refcoord_ );
}
