/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Aug 2016
-*/


#include "coordsystem.h"

#include "iopar.h"
#include "separstr.h"

static const double cAvgEarthRadius = 6367450;
static const double latdist = cAvgEarthRadius*mDeg2RadD;

mImplFactory( Coords::PositionSystem, Coords::PositionSystem::factory );

using namespace Coords;

static Threads::Lock systemreposlock;
static ManagedObjectSet<IOPar> systemrepos;


static void reloadRepository(CallBacker*)
{
    Threads::Locker lock( systemreposlock );
    systemrepos.erase();

    //Todo
}


bool PositionSystem::operator==( const PositionSystem& oth ) const
{
    IOPar myiop; fillPar( myiop );
    IOPar othiop; oth.fillPar( othiop );
    return myiop == othiop;
}


void PositionSystem::initRepository( NotifierAccess* na )
{
    reloadRepository( 0 );

    if ( na ) na->notify( mSCB( reloadRepository ) );
    //Don't do remove, as we assume we will be destroyed
    //after NotifierAccess's last call
}


void PositionSystem::getSystemNames( bool orthogonalonly, uiStringSet& strings,
			     ObjectSet<IOPar>& pars )
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

	if ( orthogonalonly )
	{
	    RefMan<PositionSystem> system = createSystem( *systempar );
	    if ( !system || !system->isOrthogonal() )
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
	RefMan<PositionSystem> system = createSystem( *systempar );
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


RefMan<PositionSystem> PositionSystem::createSystem( const IOPar& par )
{
    BufferString factorykey;
    if ( !par.get( sKeyFactoryName(), factorykey ) )
	return 0;

    RefMan<PositionSystem> res = factory().create( factorykey );
    if ( !res )
	return 0;

    if ( !res->usePar( par ) )
	return 0;

    return res;
}


Coord PositionSystem::convert( const Coord& in, const PositionSystem& from,
		       const PositionSystem& to )
{
    const Coord geomwgs84 = from.toGeographicWGS84( in );
    return to.fromGeographicWGS84( geomwgs84 );
}


Coord PositionSystem::convertFrom( const Coord& in,
				   const PositionSystem& from ) const
{ return convert( in, from, *this ); }


bool PositionSystem::usePar( const IOPar& par )
{
    BufferString nm;
    return par.get( sKeyFactoryName(), nm ) && nm == factoryKeyword();
}


void PositionSystem::fillPar( IOPar& par ) const
{
    par.set( sKeyFactoryName(), factoryKeyword() );
}


uiString PositionSystem::toUiString( const Coord& crd ) const
{
    return ::toUiString( toString(crd,false) );
}


BufferString PositionSystem::toString( const Coord& crd, bool withsystem ) const
{
    BufferString res;
    const char* space = " ";
    if ( withsystem )
	res.add( factoryKeyword() ).add( space );

    res.add( ::toString(crd.x_) ).add( space );
    res.add( ::toString(crd.y_));

    return res;
}


Coord PositionSystem::fromString( const char* str ) const
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


UnlocatedXY::UnlocatedXY()
    : lngdist_(mUdf(float))
{
}


UnlocatedXY::UnlocatedXY( const Coord& c, const LatLong& l )
{
    setLatLongEstimate( c, l );
}


bool UnlocatedXY::geographicTransformOK() const
{ return !mIsUdf(lngdist_); }


void UnlocatedXY::setLatLongEstimate( const LatLong& ll, const Coord& c )
{
    refcoord_ = c; reflatlng_ = ll;
    lngdist_ = mDeg2RadD * cos( ll.lat_ * mDeg2RadD ) * cAvgEarthRadius;
}


LatLong UnlocatedXY::toGeographicWGS84( const Coord& c ) const
{
    if ( !geographicTransformOK() ) return reflatlng_;

    const double scalefac = isfeet_ ? mFromFeetFactorD : 1;

    Coord coorddist( (c.x_ - refcoord_.x_) * scalefac,
		    (c.y_ - refcoord_.y_) * scalefac );
    LatLong ll( reflatlng_.lat_ + coorddist.y_ / latdist,
	       reflatlng_.lng_ + coorddist.x_ / lngdist_ );

    if ( ll.lat_ > 90 )		ll.lat_ = 180 - ll.lat_;
    else if ( ll.lat_ < -90 )	ll.lat_ = -180 - ll.lat_;
    if ( ll.lng_ < -180 )	ll.lng_ = ll.lng_ + 360;
    else if ( ll.lng_ > 180 )	ll.lng_ = ll.lng_ - 360;

    return ll;
}


Coord UnlocatedXY::fromGeographicWGS84( const LatLong& ll ) const
{
    if ( !geographicTransformOK() ) return Coord::udf();

    const double scalefac = isfeet_ ? mFromFeetFactorD : 1;

    const LatLong latlongdist( ll.lat_ - reflatlng_.lat_,
			      ll.lng_ - reflatlng_.lng_ );
    return Coord( refcoord_.x_ + latlongdist.lng_ * lngdist_ / scalefac,
		 refcoord_.y_ + latlongdist.lat_ * latdist / scalefac );
}


static const char* sKeyIsFeet = "XY in Feet";
static const char* sKeyRefLatLong = "Reference Lat/Long";
static const char* sKeyRefCoord = "Reference Coordinate";

bool UnlocatedXY::usePar( const IOPar& par )
{
    if ( !PositionSystem::usePar(par) )
	return false;

    Coord crd;
    LatLong latlong;
    par.getYN( sKeyIsFeet, isfeet_ );

    if ( par.get(sKeyRefLatLong,latlong.lat_,latlong.lng_) &&
	 par.get(sKeyRefCoord,crd) )
    {
	setLatLongEstimate( latlong, crd );
    }

    return true;
}


void UnlocatedXY::fillPar( IOPar& par ) const
{
    PositionSystem::fillPar( par );
    par.setYN( sKeyIsFeet, isfeet_ );
    par.set( sKeyRefLatLong, reflatlng_.lat_, reflatlng_.lng_);
    par.set( sKeyRefCoord, refcoord_ );
}
