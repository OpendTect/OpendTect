/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "latlong.h"

#include "coordsystem.h"
#include "separstr.h"
#include "survinfo.h"

#include <math.h>
#include <string.h>


const double cAvgEarthRadius = 6367450;

// LatLong
LatLong::LatLong()
{
    setUdf();
}


LatLong::LatLong( double la, double lo )
    : lat_(la)
    , lng_(lo)
{}


LatLong::LatLong( const LatLong& ll )
    : lat_(ll.lat_)
    , lng_(ll.lng_)
{}


LatLong::~LatLong()
{}


LatLong& LatLong::operator=( const LatLong& oth )
{
    if ( this == &oth )
	return *this;

    lat_ = oth.lat_;
    lng_ = oth.lng_;
    return *this;
}


bool LatLong::operator==( const LatLong& oth ) const
{
    if ( &oth == this )
	return true;

    return mIsEqual(oth.lat_,lat_,mDefEps) &&
	   mIsEqual(oth.lng_,lng_,mDefEps);
}


bool LatLong::operator!=( const LatLong& oth ) const
{
    return !(*this == oth);
}


void LatLong::setFromCoord( const Coord& coord )
{
    lat_ = coord.x_;
    lng_ = coord.y_;
}


Coord LatLong::asCoord() const
{
    return Coord( lat_, lng_ );
}


LatLong LatLong::fromCoord( const Coord& coord )
{
    return LatLong( coord.x_, coord.y_ );
}


Coord LatLong::toCoord( const LatLong& ll )
{
    return ll.asCoord();
}


Coord LatLong::transform( const LatLong& ll, bool towgs84,
			  const Coords::CoordSystem* coordsys )
{
    if ( !ll.isDefined() )
	return Coord::udf();

    if ( !coordsys )
	coordsys = SI().getCoordSystem().ptr();

    return coordsys ? coordsys->fromGeographic( ll, towgs84 ) : Coord::udf();
}


LatLong LatLong::transform( const Coord& c, bool towgs84,
			    const Coords::CoordSystem* coordsys )
{
    if ( !coordsys )
	coordsys = SI().getCoordSystem().ptr();

    return coordsys ? coordsys->toGeographic( c, towgs84 ) : LatLong::udf();
}


const char* LatLong::toString() const
{
    mDeclStaticString( ret );
    ret.set( "[" ).add( toStringPrecise(lat_) )
       .add( "," ).add( toStringPrecise(lng_) ).add( "]" );
    return ret.buf();
}


bool LatLong::fromString( const char* s )
{
    if ( !s || !*s ) return false;

    BufferString str( s );
    char* ptrlat = str.getCStr(); mSkipBlanks( ptrlat );
    if ( *ptrlat == '[' ) ptrlat++;
    char* ptrlng = firstOcc( ptrlat, ',' );
    if ( !ptrlng ) return false;
    *ptrlng++ = '\0';
    if ( !*ptrlng ) return false;
    char* ptrend = firstOcc( ptrlng, ']' );
    if ( ptrend ) *ptrend = '\0';

    lat_ = toDouble( ptrlat );
    lng_ = toDouble( ptrlng );
    return true;
}


void LatLong::getDMS( bool lat, int& d, int& m, float& s ) const
{
    double v = lat ? lat_ : lng_;
    d = (int)v;
    v -= d; v *= 60;
    m = (int)v;
    v -= m; v *= 60;
    s = (float)v;
}


void LatLong::setDMS( bool lat, int d, int m, float s )
{
    double& v = lat ? lat_ : lng_;
    const double one60th = 1. / 60.;
    const double one3600th = one60th * one60th;
    v = d + one60th * m + one3600th * s;
}


bool LatLong::isDMSString( const BufferString& llstr )
{
    if ( llstr.isEmpty() )
	return false;

    const bool hasmin = llstr.contains( '\'' );
    const bool hassec = llstr.contains( '\"' ) || llstr.contains( "''" );
    const bool hasminsecsymbols = hasmin && hassec;
    if ( hasminsecsymbols )
	return true;

    if ( isDigit(llstr.firstChar()) && isAlpha(llstr.lastChar()) )
	return true;

    return false;
}


bool LatLong::parseDMSString( const BufferString& llstrin, bool lat )
{
    BufferString llstr = llstrin;
    llstr.trimBlanks();
    char degreesymbol = -80;
    const bool hasdegreesymbol = llstr.contains( degreesymbol );
    if ( hasdegreesymbol )
    {
	// Lat-Long string as in: DDD°MM'SS.SS"X
	llstr.replace( degreesymbol, ' ' );
	const int minpos = llstr.indexOf( '\'' );
	const bool validpos = minpos>0 && minpos<(llstr.size()-1);
	if ( validpos && llstr[minpos+1] != ' ' )
	    llstr.insertAt( minpos+1, " " );
    }

    llstr.replace( "  ", " " );

    // Lat-Long string as in: DDD MM' SS.SSSS'' (or SS.SSSS")
    const SeparString ss( llstr.buf(), ' ' );
    if ( ss.size() < 4 )
	return false;

    int degs = toInt( ss[0] );

    BufferString minstr = ss[1];
    minstr.remove( '\'' );
    int mins = toInt( minstr );

    BufferString secstr = ss[2];
    secstr.remove( '\"' );
    secstr.remove( '\'' );
    float secs = toFloat( secstr );

    const StringView NESW = ss[3];
    if ( NESW.size()==1 && degs>0 && (NESW[0]=='S' || NESW[0]=='W') )
	degs *= -1;

    if ( degs < 0 )
    {
	mins *= -1;
	secs *= -1;
    }

    setDMS( lat, degs, mins, secs );
    return true;
}


bool LatLong::setFromString( const char* str, bool lat )
{
// Supports strings formatted as dddmmssssh or ggg.gggggh
// h is not mandatory
    BufferString llstr = str;
    if ( llstr.isEmpty() )
	return false;

    if ( isDMSString(llstr) && parseDMSString(llstr,lat) )
	return true;

    char& lastchar = llstr[llstr.size()-1];
    const bool hasSW = lastchar=='S' || lastchar=='W';
    if ( lastchar=='N' || lastchar=='E' || hasSW )
	lastchar = '\0';

    const BufferString nrstr = llstr.find( '.' );
    const int trailsz = nrstr.size();
    const int sz = llstr.size() - trailsz; // size of string before .
    if ( sz < 5 ) // Size never larger than 4 when in degrees
    {
	double& val = lat ? lat_ : lng_;
	val = toDouble( llstr );
	if ( hasSW && val>0 )
	    val *= -1;
    }
    else
    {
	const int len = llstr.size();

	// parse seconds
	BufferString buf( 128, false );
	int parsesz = 4 + trailsz;
	int offset = parsesz;
#ifdef __win__
	strncpy_s(buf.getCStr(),buf.bufSize(),llstr.buf()+(len-offset),parsesz);
#else
	strncpy(buf.getCStr(),llstr.buf()+(len-offset),parsesz);
#endif
	buf[parsesz] = '\0';
	float secs = toFloat(buf) / 100.f;

	// parse minutes
	buf.setEmpty();
	parsesz = 2;
	offset += parsesz;
	if ( parsesz <= 0 )
	    return false;
#ifdef __win__
	strncpy_s(buf.getCStr(),buf.bufSize(),llstr.buf()+(len-offset),parsesz);
#else
	strncpy(buf.getCStr(),llstr.buf()+(len-offset),parsesz);
#endif
	buf[parsesz] = '\0';
	int mins = toInt( buf );

	// parse degrees
	buf.setEmpty();
	parsesz = len - offset;
	if ( parsesz <= 0 )
	    return false;
#ifdef __win__
	strncpy_s(buf.getCStr(),buf.bufSize(),llstr.buf(),parsesz);
#else
	strncpy(buf.getCStr(),llstr.buf(),parsesz);
#endif
	buf[parsesz] = '\0';
	int degs = toInt( buf );
	if ( hasSW && degs>0 )
	    degs *= -1;

	if ( degs < 0 )
	{
	    mins *= -1;
	    secs *= -1;
	}

	setDMS( lat, degs, mins, secs );
    }

    return true;
}


// LatLong2Coord
LatLong2Coord::LatLong2Coord()
    : latdist_(cAvgEarthRadius*mDeg2RadD)
    , lngdist_(mUdf(double))
    , scalefac_(-1)
{
}


LatLong2Coord::LatLong2Coord( const Coord& c, const LatLong& l )
    : latdist_(cAvgEarthRadius*mDeg2RadD)
    , scalefac_(-1)
{
    set( l, c );
}


LatLong2Coord::~LatLong2Coord()
{}


bool LatLong2Coord::operator==( const LatLong2Coord& oth ) const
{
    if ( &oth == this )
	return true;

    return reflatlng_ == oth.reflatlng_ &&
	   refcoord_ == oth.refcoord_ &&
	   mIsEqual(latdist_,oth.latdist_,1e-3) &&
	   mIsEqual(lngdist_,oth.lngdist_,1e-3) &&
	   mIsEqual(scalefac_,oth.scalefac_,mDefEps);
}


bool LatLong2Coord::operator!=( const LatLong2Coord& oth ) const
{
    return !(*this == oth);
}


void LatLong2Coord::set( const LatLong& ll, const Coord& c )
{
    refcoord_ = c; reflatlng_ = ll;
    lngdist_ = mDeg2RadD * cos( ll.lat_ * mDeg2RadD ) * cAvgEarthRadius;
}


// We cannot put this in the constructor: that leads to disaster at startup
#define mPrepScaleFac() \
    if ( scalefac_ < 0 ) \
	const_cast<LatLong2Coord*>(this)->scalefac_ \
		= SI().xyInFeet() ? mFromFeetFactorD : 1;


LatLong LatLong2Coord::transform( const Coord& c ) const
{
    if ( !isOK() )
	return reflatlng_;

    mPrepScaleFac();

    Coord coorddist( (c.x_ - refcoord_.x_) * scalefac_,
                     (c.y_ - refcoord_.y_) * scalefac_ );
    LatLong ll( reflatlng_.lat_ + coorddist.y_ / latdist_,
                reflatlng_.lng_ + coorddist.x_ / lngdist_ );

    if ( ll.lat_ > 90 )		ll.lat_ = 180 - ll.lat_;
    else if ( ll.lat_ < -90 )	ll.lat_ = -180 - ll.lat_;
    if ( ll.lng_ < -180 )	ll.lng_ = ll.lng_ + 360;
    else if ( ll.lng_ > 180 )	ll.lng_ = ll.lng_ - 360;

    return ll;
}


Coord LatLong2Coord::transform( const LatLong& ll ) const
{
    if ( !isOK() ) return SI().minCoord(true);
    mPrepScaleFac();

    const LatLong latlongdist( ll.lat_ - reflatlng_.lat_,
			       ll.lng_ - reflatlng_.lng_ );
    return Coord( refcoord_.x_ + latlongdist.lng_ * lngdist_ / scalefac_,
		  refcoord_.y_ + latlongdist.lat_ * latdist_ / scalefac_ );
}


const char* LatLong2Coord::toString() const
{
    mDeclStaticString( ret );
    ret.set( refcoord_.toString() ).add( "=" ).add( reflatlng_.toString() );
    return ret.buf();
}


bool LatLong2Coord::fromString( const char* s )
{
    lngdist_ = mUdf(float);
    if ( !s || !*s )
	return false;

    BufferString str( s );
    char* ptr = str.find( '=' );
    if ( !ptr )
	return false;

    *ptr++ = '\0';
    Coord c;
    LatLong l;
    if ( !c.fromString(str) || !l.fromString(ptr) )
	return false;
   if ( mIsZero(c.x_,1e-3) && mIsZero(c.y_,1e-3) )
	return false;

    set( l, c );
    return true;
}
