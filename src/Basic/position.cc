/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-6-1996
-*/

static const char* rcsID = "$Id: position.cc,v 1.38 2004-07-22 16:14:07 bert Exp $";

#include "position.h"
#include "bufstring.h"
#include <math.h>
#include <string.h>


float BinIDValue::compareepsilon = 1e-4;
float BinIDValues::udf = mUndefValue;


double Coord::distance( const Coord& coord ) const
{
    double diffx = coord.x - x;
    double diffy = coord.y - y;
    return diffx || diffy ? sqrt( diffx*diffx + diffy*diffy ) : 0;
}


void Coord::fill( char* str ) const
{
    if ( !str ) return;
    strcpy( str, "(" ); strcat( str, getStringFromDouble(0,x) );
    strcat( str, "," ); strcat( str, getStringFromDouble(0,y) );
    strcat( str, ")" );
}


bool Coord::use( const char* str )
{
    if ( !str || !*str ) return false;
    static char buf[80];

    strcpy( buf, *str == '(' ? str+1 : str );
    char* ptr = strchr( buf, ',' );
    if ( !ptr ) return false;

    *ptr++ = '\0';
    const int len = strlen(ptr);
    if ( len && ptr[len-1] == ')' ) ptr[len-1] = '\0';

    x = atof( buf );
    y = atof( ptr );
    return true;
}


bool getDirectionStr( const Coord& coord, BufferString& res )
{
    if ( mIsZero(coord.x,mDefEps) && mIsZero(coord.y,mDefEps) )
	return false;

    const double len = sqrt(coord.x*coord.x+coord.y*coord.y);
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
    return sqrt( x*x + y*y + z*z );
}


void Coord3::fill(char* str, const char* start,
		     const char* space, const char* end) const
{
    strcpy( str, start );
    strcat( str, getStringFromDouble(0,x) ); strcat(str,space);
    strcat( str, getStringFromDouble(0,y) ); strcat(str,space);
    strcat( str, getStringFromDouble(0,z) ); strcat(str,space);
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


double Coord3::distance( const Coord3& b ) const
{
    double dx = x-b.x, dy = y-b.y, dz = z-b.z;
    return sqrt( dx*dx + dy*dy + dz*dz );
}


void BinID::fill( char* str ) const
{
    if ( !str ) return;
    sprintf( str, "%d/%d", inl, crl );
}


bool BinID::use( const char* str )
{
    if ( !str || !*str ) return false;

    static char buf[80];
    strcpy( buf, str );
    char* ptr = strchr( buf, '/' );
    if ( !ptr ) return false;
    *ptr++ = '\0';
    inl = atoi( buf );
    crl = atoi( ptr );
    return true;
}


BinID BinID::operator+( const BinID& bi ) const
{
    BinID res = *this; 
    res.inl += bi.inl; res.crl += bi.crl;
    return res; 
}


BinID BinID::operator-( const BinID& bi ) const
{
    BinID res = *this; 
    res.inl -= bi.inl; res.crl -= bi.crl;
    return res; 
}


BinIDValue::BinIDValue( const BinIDValues& bvs, int nr )
    	: binid(bvs.binid)
    	, value(bvs.value(nr))
{
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
	if ( !mIsEqual(vals[idx],bvs.vals[idx],BinIDValue::compareepsilon) )
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
	    vals[idx] = mUndefValue;
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
	    vals[idx] = mUndefValue;
    }
}


void BinIDValues::setVals( const float* vs )
{
    if ( sz ) memcpy( vals, vs, sz * sizeof(float) );
}
