/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 21-6-1996
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "position.h"
#include "coordvalue.h"
#include "binidvalue.h"

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


od_int64 Pos::IdxPair::sqDistTo( const Pos::IdxPair& oth ) const
{
    od_int64 sqfrst = (first-oth.first); sqfrst *= sqfrst;
    od_int64 sqsec = (second-oth.second); sqsec *= sqsec;
    return sqfrst + sqsec;
}


const Pos::IdxPair& Pos::IdxPair::udf()
{
   static Pos::IdxPair udfpair( mUdf(IdxType), mUdf(IdxType) );
   return udfpair;
}


const char* Pos::IdxPair::getUsrStr( const char* prefx, const char* sep,
				     const char* postfx, bool only2nd ) const
{
    mDeclStaticString( ret );
    if ( isUdf() )
	ret.set( "<undef>" );
    else
    {
	ret.set( prefx );
	if ( only2nd )
	    ret.add( second );
	else
	    ret.add( first ).add( sep ).add( second );
	ret.add( postfx );
    }
    return ret.buf();
}


bool Pos::IdxPair::parseUsrStr( const char* str, const char* prefx,
       				const char* sep, const char* postfx )
{
    if ( !str || !*str )
	return false;
    if ( *str == '<' )
	{ *this = udf(); return true; }

    BufferString bs( str );
    char* ptr1st = bs.buf(); mSkipBlanks( ptr1st );
    while ( *prefx && *ptr1st && *prefx == *ptr1st )
	{ prefx++; ptr1st++; }
    if ( !*ptr1st )
	return false;

    char* ptr2nd = strstr( ptr1st, sep );
    if ( !ptr2nd )
	ptr2nd = ptr1st;
    else
	*ptr2nd++ = '\0';

    char* ptrpost = *postfx ? strstr( ptr2nd, postfx ) : 0;
    if ( ptrpost )
	*ptrpost = 0;

    second = toInt( ptr2nd );
    if ( ptr1st != ptr2nd )
	first = toInt( ptr1st );
    return true;
}


bool Pos::IdxPair::isNeighborTo( const Pos::IdxPair& oth,
			const Pos::IdxPairStep& step, bool conn8 ) const
{
    const IdxPairDelta diff( abs(row()-oth.row()), abs(col()-oth.col()) );
    const bool are8 = diff.row()<=step.row() && diff.col()<=step.col()
		   && !(!diff.row() && !diff.col());
    if ( conn8 )
	return are8;

    const IdxType res = (IdxType)(diff.row()>0) + (IdxType)(diff.col()>0);
    return are8 && res < 2;
}



Coord Coord::normalize() const
{
    const DistType sqabsval = sqAbs();
    if ( sqabsval < 1e-16 )
	return *this;

    return *this / Math::Sqrt(sqabsval); 
}


Coord::DistType Coord::dot( const Coord& b ) const
{
    return x*b.x + y*b.y;
}


const Coord& Coord::udf()
{
   static Coord udfcoord( mUdf(OrdType), mUdf(OrdType) );
   return udfcoord;
}


Coord::DistType Coord::cosAngle( const Coord& from, const Coord& to ) const
{
    DistType rsq = sqDistTo( from );
    DistType lsq = sqDistTo( to );
    if ( !rsq || !lsq ) return 1;

    DistType osq = from.sqDistTo( to );
    return (rsq +  lsq - osq) / (2 * Math::Sqrt(rsq) * Math::Sqrt(lsq));
}


#include <iostream>


Coord::DistType  Coord::angle( const Coord& from, const Coord& to ) const
{
    const DistType cosang = cosAngle( from, to );
    if ( cosang >=  1 ) return 0;
    if ( cosang <= -1 ) return M_PI;

    const Coord vec1 = from - *this;
    const Coord vec2 =  to  - *this;
    const DistType det = vec1.x * vec2.y - vec1.y * vec2.x;

    const DistType ang = Math::ACos( cosang );
    return det<0 ? 2*M_PI - ang : ang;
} 


const char* Coord::getUsrStr() const
{
    mDeclStaticString( ret );
    if ( isUdf() )
	ret.set( "<undef>" );
    else
	ret.set( "(" ).add( x ).add( "," ).add( y ).add( ")" );
    return ret.buf();
}


bool Coord::parseUsrStr( const char* s )
{
    if ( !s || !*s ) return false;
    if ( *s == '<' )
	{ *this = udf(); return true; }

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


Coord::DistType Coord3::abs() const
{
    return Math::Sqrt( x*x + y*y + z*z );
}


Coord::DistType Coord3::sqAbs() const { return x*x + y*y + z*z; }


const char* Coord3::getUsrStr() const
{
    mDeclStaticString( ret );
    if ( isUdf() )
	ret.set( "<undef>" );
    else
	ret.set("(").add(x).add(",").add(y).add(",").add(z).add(")");
    return ret.buf();
}


bool Coord3::parseUsrStr( const char* str )
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


Coord::DistType Coord3::distTo( const Coord3& b ) const
{
    return Math::Sqrt( Coord3::sqDistTo( b ) );
}


Coord::DistType Coord3::sqDistTo( const Coord3& b ) const
{
    const DistType dx = x-b.x, dy = y-b.y, dz = z-b.z;
    return dx*dx + dy*dy + dz*dz;
}


const Coord3& Coord3::udf()
{
   static Coord3 udfc3( mUdf(OrdType), mUdf(OrdType), mUdf(OrdType) );
   return udfc3;
}


Coord3Value::Coord3Value( double x, double y, double z, float v )
    : coord(x,y,z), value(v) 	
{
}


Coord3Value::Coord3Value( const Coord3& c, float v )
    : coord(c), value(v)
{
}


BinIDValue::BinIDValue( const BinIDValues& bvs, BinID::IdxType nr )
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


TrcKey::TrcKey( TrcKey::SurvID id, const BinID& bid )
    : survid_( id )
    , pos_( bid )
{
}


TrcKey::TrcKey( const BinID& bid )
    : survid_( std3DSurvID() )
    , pos_( bid )
{
}


TrcKey::TrcKey( TrcKey::SurvID id, int linenr, int trcnr )
    : survid_(id)
    , pos_(linenr,trcnr)
{
}


const TrcKey& TrcKey::udf()
{
    static const TrcKey udfkey( mUdf(SurvID),
	    	BinID::udf().lineNr(), BinID::udf().trcNr() );
    return udfkey;
}


TrcKey::SurvID TrcKey::std2DSurvID()
{
    return Survey::GeometryManager::get2DSurvID();
}


TrcKey::SurvID TrcKey::std3DSurvID()
{
    return Survey::GM().default3DSurvID();
}


TrcKey::SurvID TrcKey::cUndefSurvID()
{
    return Survey::GeometryManager::cUndefGeomID();
}
