/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "coordsystem.h"

#include "iopar.h"
#include "separstr.h"


const char* Coords::CoordSystem::sKeyFactoryName()
{
    return "System name";
}

const char* Coords::CoordSystem::sKeyUiName()
{
    return "Description";
}


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


void CoordSystem::getSystemNames( bool orthogonalonly,
				  bool projectiononly,
				  uiStringSet& strings, ObjectSet<IOPar>& pars )
{
    //Add all factory entries
    const BufferStringSet& factorynames = factory().getNames();
    const uiStringSet& factoryuinames = factory().getUserNames();

    for ( int idx=0; idx<factorynames.size(); idx++ )
    {
	PtrMan<IOPar> systempar = new IOPar;
	if ( !systempar ) //out of memory
	    continue;

	const char* factorykey = factorynames.get( idx ).buf();
	systempar->set( sKeyFactoryName(), factorykey );
	systempar->set( sKeyUiName(), factoryuinames[idx], true );
	if ( projectiononly )
	{
	    ConstRefMan<CoordSystem> projsystem = factory().create( factorykey);
	    if ( !projsystem || !projsystem->isProjection() )
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
	if ( !systempar->get(sKeyUiName(),uiname,true) ||
	     !systempar->get(sKeyFactoryName(),factoryname) )
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
    if ( !res || !res->usePar(par) || !res->isOK() )
	return nullptr;

    return res;
}


RefMan<CoordSystem> CoordSystem::createSystem( const char* str,
					       BufferString& msgs )
{
    RefMan<CoordSystem> res;
    RefMan<CoordSystem> cs;
    const StringView textstr( str );
    const BufferStringSet& factnms = factory().getNames();
    for ( int idx=factnms.size()-1; idx>=0; idx-- )
    {
	if ( !textstr.startsWith(factnms.get(idx).str()) )
	    continue;

	cs = factory().create( factnms.get(idx).str() );
	res = cs->fromString( str, &msgs );
	return res;
    }

    for ( int idx=factnms.size()-1; idx>=0; idx-- )
    {
	cs = factory().create( factnms.get(idx).str() );
	if ( !cs )
	    continue;

	res = cs->fromString( str, &msgs );
	if ( res )
	    return res;
    }

    //return empty UnlocatedXY ?
    return nullptr;
}


Coord CoordSystem::convert( const Coord& in, const CoordSystem& from,
			       const CoordSystem& to )
{
    return from.convertTo( in, to );
}


Coord CoordSystem::convertFrom( const Coord& in, const CoordSystem& from ) const
{
    const LatLong wgs84ll = LatLong::transform( in, true, &from );
    return LatLong::transform( wgs84ll, true, this );
}


Coord CoordSystem::convertTo( const Coord& in, const CoordSystem& to ) const
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


namespace Coords
{
    BufferString wgs84dispstr_;
    ConstRefMan<CoordSystem> wgs84crsproj_;

    extern "C" { mGlobal(Basic) void SetWGS84(const char*,CoordSystem*); }
    mExternC(Basic) void SetWGS84( const char* dispstr, CoordSystem* crs )
    {
	wgs84dispstr_.set( dispstr );
	wgs84crsproj_ = crs;
    }
}


const CoordSystem* CoordSystem::getWGS84LLSystem()
{
    return wgs84crsproj_.ptr();
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


// AnchorBasedXY

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
    auto* cp = new AnchorBasedXY( reflatlng_, refcoord_ );
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

    const Coord coorddist( (c.x_ - refcoord_.x_) * scalefac,
                           (c.y_ - refcoord_.y_) * scalefac );
    LatLong ll( reflatlng_.lat_ + coorddist.y_ / latdist,
                reflatlng_.lng_ + coorddist.x_ / lngdist_ );

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
    return Coord( refcoord_.x_ + latlongdist.lng_ * lngdist_ / scalefac,
                  refcoord_.y_ + latlongdist.lat_ * latdist / scalefac );
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


BufferString AnchorBasedXY::toString( StringType /* typ */,
				      bool withsystem ) const
{
    BufferString res;
    if ( withsystem )
	res.set( factoryKeyword() ).addSpace();

    FileMultiString fms;
    fms.add( reflatlng_.toString() ).add( refcoord_.toString() )
       .add( isfeet_ ? "F" : "M" );
    res.add( fms.str() );

    return res;
}


RefMan<CoordSystem> AnchorBasedXY::fromString( const char* str,
					       BufferString* msg ) const
{
    BufferString defstr( str );
    if ( defstr.startsWith(factoryKeyword()) )
	defstr.remove( factoryKeyword() ).trimBlanks();

    const FileMultiString fms( defstr.buf() );
    if ( fms.size() != 3 )
	return nullptr;

    const BufferString llstr = fms[0];
    const BufferString crdstr = fms[1];
    LatLong ll;
    Coord crd;
    ll.fromString( llstr.buf() );
    crd.fromString( crdstr.buf() );
    if ( ll.isUdf() || crd.isUdf() )
	return nullptr;

    RefMan<AnchorBasedXY> res = new AnchorBasedXY( ll, crd );
    res->setIsFeet( fms[2] == "F" );

    return res;
}
