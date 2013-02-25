/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 21-6-1996
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "position.h"

#include "bufstring.h"
#include "math2.h"
#include "rowcol.h"
#include "string2.h"
#include "undefval.h"
#include "survgeom.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>


mImplRowColFunctions( BinID, inl, crl );


float BinIDValues::udf = mUdf(float);

BinID::BinID( const RowCol& rc )
    : inl( rc.row )
    , crl( rc.col )
{}


const BinID& BinID::udf()
{
   static BinID _udf( mUdf(int), mUdf(int) );
   return _udf;
}


Coord Coord::normalize() const
{ return *this / abs(); }


double Coord::dot( const Coord& b ) const
{ return x*b.x + y*b.y; }


const Coord& Coord::udf()
{
   static Coord _udf( mUdf(double), mUdf(double) );
   return _udf;
}


double Coord::cosAngle( const Coord& from, const Coord& to ) const
{
    double rsq = sqDistTo( from );
    double lsq = sqDistTo( to );
    if ( !rsq || !lsq ) return 1;

    double osq = from.sqDistTo( to );
    return (rsq +  lsq - osq) / (2 * Math::Sqrt(rsq) * Math::Sqrt(lsq));
}


#include <iostream>


double Coord::angle( const Coord& from, const Coord& to ) const
{
    const double cosang = cosAngle( from, to );
    if ( cosang >=  1 ) return 0;
    if ( cosang <= -1 ) return M_PI;

    const Coord vec1 = from - *this;
    const Coord vec2 =  to  - *this;
    const double det = vec1.x * vec2.y - vec1.y * vec2.x;

    const double ang = Math::ACos( cosang );
    return det<0 ? 2*M_PI - ang : ang;
} 


void Coord::fill( char* str ) const
{
    if ( !str ) return;
    strcpy( str, "(" ); 
    getStringFromDouble(0,x,str+1);
    strcat( str, "," );
    getStringFromDouble( 0, y, str+strlen(str) );
    strcat( str, ")" );
}


bool Coord::use( const char* s )
{
    if ( !s || !*s ) return false;

    BufferString str( s );
    char* ptrx = str.buf(); mSkipBlanks( ptrx );
    if ( *ptrx == '(' ) ptrx++;
    char* ptry = strchr( ptrx, ',' );
    if ( !ptry ) return false;
    *ptry++ = '\0';
    if ( !*ptry ) return false;
    char* ptrend = strchr( ptry, ')' );
    if ( ptrend ) *ptrend = '\0';

    x = toDouble( ptrx );
    y = toDouble( ptry );
    return true;
}


bool getDirectionStr( const Coord& coord, BufferString& res )
{
    if ( mIsZero(coord.x,mDefEps) && mIsZero(coord.y,mDefEps) )
	return false;

    const double len = Math::Sqrt(coord.x*coord.x+coord.y*coord.y);
    const double x = coord.x/len;
    const double y = coord.y/len;

    res = "";
    if ( y>0.5 )
	res += "north";
    else if ( y<-0.5 )
	res += "south";

    if ( x>0.5 )
	res += "east";
    else if ( x<-0.5 )
	res += "west";

    return true;
}


double Coord3::abs() const
{
    return Math::Sqrt( x*x + y*y + z*z );
}


double Coord3::sqAbs() const { return x*x + y*y + z*z; }


void Coord3::fill(char* str, const char* start,
		     const char* space, const char* end) const
{
    strcpy( str, start );
    getStringFromDouble( 0, x, str+strlen(str) ); strcat(str,space);
    getStringFromDouble( 0, y, str+strlen(str) ); strcat(str,space);
    getStringFromDouble( 0, z, str+strlen(str) ); strcat(str,space);
    strcat( str, end );
}


bool Coord3::use(const char* str)
{
    if ( !str ) return false;
    const char* endptr=str+strlen(str);

    while ( !isdigit(*str) && *str!='+' && *str!='-' && str!=endptr )
	str++;

    char* numendptr;
    x = strtod( str, &numendptr );
    if ( str==numendptr ) return false;

    str = numendptr;
    while ( !isdigit(*str) && *str!='+' && *str!='-' && str!=endptr )
	str++;
    y = strtod( str, &numendptr );
    if ( str==numendptr ) return false;

    str = numendptr;
    while ( !isdigit(*str) && *str!='+' && *str!='-' && str!=endptr )
	str++;
    z = strtod( str, &numendptr );
    if ( str==numendptr ) return false;

    return true;
}


double Coord3::distTo( const Coord3& b ) const
{
    return Math::Sqrt( Coord3::sqDistTo( b ) );
}


double Coord3::sqDistTo( const Coord3& b ) const
{
    const double dx = x-b.x, dy = y-b.y, dz = z-b.z;
    return dx*dx + dy*dy + dz*dz;
}


const Coord3& Coord3::udf()
{
   static Coord3 _udf( mUdf(double), mUdf(double), mUdf(double) );
   return _udf;
}


BinIDValue::BinIDValue( const BinIDValues& bvs, int nr )
    	: binid(bvs.binid)
    	, value(bvs.value(nr))
{
}


BinIDValues::~BinIDValues()
{
    delete [] vals;
}


BinIDValues& BinIDValues::operator =( const BinIDValues& bvs )
{
    if ( &bvs != this )
    {
	binid = bvs.binid;
	setSize( bvs.sz );
	if ( vals )
	    memcpy( vals, bvs.vals, sz * sizeof(float) );
    }
    return *this;
}


bool BinIDValues::operator ==( const BinIDValues& bvs ) const
{
    if ( binid != bvs.binid || sz != bvs.sz )
	return false;

    for ( int idx=0; idx<sz; idx++ )
	if ( !mIsEqual(vals[idx],bvs.vals[idx],BinIDValue::compareEpsilon()) )
	    return false;

    return true;
}


void BinIDValues::setSize( int newsz, bool kpvals )
{
    if ( newsz == sz ) return;

    if ( !kpvals || newsz < 1 )
    {
	sz = newsz < 1 ? 0 : newsz;
	delete [] vals; vals = sz ? new float [sz] : 0;
	for ( int idx=0; idx<sz; idx++ )
	    Values::setUdf( vals[idx] );
    }
    else
    {
	float* oldvals = vals;
	int oldsz = sz; sz = newsz;
	vals = new float [ sz ];
	int transfsz = oldsz > sz ? sz : oldsz;
	for ( int idx=0; idx<transfsz; idx++ )
	    vals[idx] = oldvals[idx];
	delete [] oldvals;
	for ( int idx=transfsz; idx<sz; idx++ )
	    Values::setUdf( vals[idx] );
    }
}


void BinIDValues::setVals( const float* vs )
{
    if ( sz ) memcpy( vals, vs, sz * sizeof(float) );
}


TraceID::TraceID( const BinID& bid )
    : geomid_( std3DGeomID() )
    , pos_( bid )
{}


TraceID::TraceID( GeomID geomid, int linenr, int trcnr )
    : geomid_( geomid )
    , pos_( linenr, trcnr )
{}


const TraceID& TraceID::udf()
{
    static TraceID trcid( -1, -1, -1 );
    return trcid;
}


TraceID::GeomID TraceID::std3DGeomID()
{ return Survey::GeometryManager::cDefault3DGeom(); }

