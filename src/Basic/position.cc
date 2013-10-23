/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 21-6-1996
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "position.h"
#include "binidvalue.h"
#include "coordvalue.h"
#include "posidxpair2coord.h"

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


#define mImplBinIDValueEqOpers(clss) \
bool clss::operator==( const BinID& bid ) const \
{ return Pos::IdxPair::operator==(bid); } \
bool clss::operator!=( const BinID& bid ) const \
{ return Pos::IdxPair::operator!=(bid); }

mImplBinIDValueEqOpers(BinIDValue)
mImplBinIDValueEqOpers(BinIDValues)

BinIDValue::BinIDValue( const BinIDValues& bvs, int nr )
{
    set( bvs, nr );
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

bool Coord3::isSameAs(const Coord3& pos, const Coord3& eps) const
{
    const Coord3 diff = *this - eps;
    return fabs(diff.x)<eps.x && fabs(diff.y)<eps.y && fabs(diff.z)<eps.z;
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


bool Pos::IdxPair2Coord::set3Pts( const Coord& c0, const Coord& c1,
			  const Coord& c2, const Pos::IdxPair& ip0,
			  const Pos::IdxPair& ip1, od_int32 crl2 )
{
    if ( ip1.inl() == ip0.inl() )
	return false;
    if ( ip0.crl() == crl2 )
        return false;

    DirTransform nxtr, nytr;
    od_int32 cold = ip0.crl() - crl2;
    nxtr.c = ( c0.x - c2.x ) / cold;
    nytr.c = ( c0.y - c2.y ) / cold;
    const od_int32 rowd = ip0.inl() - ip1.inl();
    cold = ip0.crl() - ip1.crl();
    nxtr.b = ( c0.x - c1.x ) / rowd - ( nxtr.c * cold / rowd );
    nytr.b = ( c0.y - c1.y ) / rowd - ( nytr.c * cold / rowd );
    nxtr.a = c0.x - nxtr.b * ip0.inl() - nxtr.c * ip0.crl();
    nytr.a = c0.y - nytr.b * ip0.inl() - nytr.c * ip0.crl();

    if ( mIsZero(nxtr.a,mDefEps) ) nxtr.a = 0;
    if ( mIsZero(nxtr.b,mDefEps) ) nxtr.b = 0;
    if ( mIsZero(nxtr.c,mDefEps) ) nxtr.c = 0;
    if ( mIsZero(nytr.a,mDefEps) ) nytr.a = 0;
    if ( mIsZero(nytr.b,mDefEps) ) nytr.b = 0;
    if ( mIsZero(nytr.c,mDefEps) ) nytr.c = 0;

    if ( !nxtr.valid(nytr) )
	return false;

    xtr = nxtr;
    ytr = nytr;
    return true;
}


Coord Pos::IdxPair2Coord::transform( const Pos::IdxPair& ip ) const
{
    return Coord( xtr.a + xtr.b*ip.row() + xtr.c*ip.col(),
		  ytr.a + ytr.b*ip.row() + ytr.c*ip.col() );
}


Pos::IdxPair Pos::IdxPair2Coord::transformBack( const Coord& coord ) const
{
    if ( mIsUdf(coord.x) || mIsUdf(coord.y) )
	return Pos::IdxPair::udf();
    const Coord fip = transformBackNoSnap( coord );
    if ( mIsUdf(fip.x) || mIsUdf(fip.y) )
	return Pos::IdxPair::udf();

    return IdxPair( mRounded(IdxType,fip.x), mRounded(IdxType,fip.y) );
}


Pos::IdxPair Pos::IdxPair2Coord::transformBack( const Coord& coord,
			const IdxPair& start, const IdxPairStep& step ) const
{
    if ( mIsUdf(coord.x) || mIsUdf(coord.y) )
	return Pos::IdxPair::udf();
    const Coord fip = transformBackNoSnap( coord );
    if ( mIsUdf(fip.x) || mIsUdf(fip.y) )
	return Pos::IdxPair::udf();

    Coord frelip( fip.x - start.first, fip.y - start.second );
    if ( step.first && step.second )
	{ frelip.x /= step.first; frelip.y /= step.second; }

    const IdxPair relip( mRounded(IdxType,frelip.x),
	    		 mRounded(IdxType,frelip.y) );
    return IdxPair( start.first + relip.first * step.first,
		    start.second + relip.second * step.second );
}


Coord Pos::IdxPair2Coord::transformBackNoSnap( const Coord& coord ) const
{
    if ( mIsUdf(coord.x) || mIsUdf(coord.y) )
	return Coord::udf();

    double det = xtr.det( ytr );
    if ( mIsZero(det,mDefEps) ) 
	return Coord::udf();

    const double x = coord.x - xtr.a;
    const double y = coord.y - ytr.a;
    return Coord( (x*ytr.c - y*xtr.c) / det, (y*xtr.b - x*ytr.b) / det );
}


Coord Pos::IdxPair2Coord::transform( const Coord& ip_coord ) const
{
    return Coord( xtr.a + xtr.b*ip_coord.x + xtr.c*ip_coord.y,
		  ytr.a + ytr.b*ip_coord.x + ytr.c*ip_coord.y );
}
