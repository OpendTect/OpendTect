/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 21-6-1996
-*/


#include "position.h"
#include "binidvalue.h"
#include "posidxpair2coord.h"

#include "bufstring.h"
#include "math2.h"
#include "rowcol.h"
#include "string2.h"
#include "undefval.h"
#include "survgeom.h"
#include "trckeyvalue.h"
#include "survinfo.h" // fallback with pErrMsg only

#include <ctype.h>
#include <math.h>

#define mUdfIdx mUdf(IdxPair::IdxType)
static const IdxPair udfidxpair( mUdfIdx, mUdfIdx );
static const Pos::IdxPair udfposidxpair( mUdfIdx, mUdfIdx );
#define mUdfOrd mUdf(Pos::Ordinate_Type)
static const Coord udfcoord( mUdfOrd, mUdfOrd );
static const Coord3 udfcoord3( mUdfOrd, mUdfOrd, mUdfOrd );


const IdxPair& IdxPair::udf()
{
   return udfidxpair;
}


const char* IdxPair::getUsrStr( const char* prefx, const char* sep,
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


bool IdxPair::parseUsrStr( const char* str, const char* prefx,
				const char* sep, const char* postfx )
{
    if ( !str || !*str )
	return false;
    if ( *str == '<' )
	{ *this = udf(); return true; }

    BufferString bs( str );
    char* ptr1st = bs.getCStr(); mSkipBlanks( ptr1st );
    while ( *prefx && *ptr1st && *prefx == *ptr1st )
	{ prefx++; ptr1st++; }
    if ( !*ptr1st )
	return false;

    char* ptr2nd = firstOcc( ptr1st, sep );
    if ( !ptr2nd )
	ptr2nd = ptr1st;
    else
	*ptr2nd++ = '\0';

    char* ptrpost = *postfx ? firstOcc( ptr2nd, postfx ) : 0;
    if ( ptrpost )
	*ptrpost = 0;

    second = toInt( ptr2nd );
    if ( ptr1st != ptr2nd )
	first = toInt( ptr1st );
    return true;
}


const Pos::IdxPair& Pos::IdxPair::udf()
{
    return udfposidxpair;
}


od_int64 Pos::IdxPair::sqDistTo( const Pos::IdxPair& oth ) const
{
    od_int64 sqfrst = (first-oth.first); sqfrst *= sqfrst;
    od_int64 sqsec = (second-oth.second); sqsec *= sqsec;
    return sqfrst + sqsec;
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

TrcKey::TrcKey( TrcKey::SurvID id, const BinID& bid )
    : survid_( id )
    , pos_( bid )
{
}


TrcKey::TrcKey( Pos::GeomID id, Pos::TraceID tid )
    : survid_( std2DSurvID() )
    , pos_( id, tid )
{
}


TrcKey::TrcKey( const BinID& bid )
    : survid_( std3DSurvID() )
    , pos_( bid )
{
}


TrcKey TrcKey::getSynth( Pos::TraceID trcnr )
{
    const BinID bid( 0, trcnr );
    return TrcKey( stdSynthSurvID(), bid );
}


bool TrcKey::is2D( SurvID sid )
{ return sid==std2DSurvID(); }


#define mGetGeomID( sid, pos ) return is2D(sid) ? pos.lineNr() : sid;

Pos::GeomID TrcKey::geomID() const
{ mGetGeomID( survid_, pos_ ); }

Pos::GeomID TrcKey::geomID( SurvID survid, const BinID& bid )
{ mGetGeomID( survid, bid ); }

const TrcKey& TrcKey::udf()
{
    mDefineStaticLocalObject( const TrcKey, udfkey,
	    (mUdf(SurvID), BinID::udf() ));
    return udfkey;
}


bool TrcKey::operator==( const TrcKey& oth ) const
{ return oth.survid_==survid_ && oth.pos_==pos_; }


TrcKey::SurvID TrcKey::std2DSurvID()
{
    return Survey::GeometryManager::get2DSurvID();
}


TrcKey::SurvID TrcKey::std3DSurvID()
{
    return Survey::GM().default3DSurvID();
}


TrcKey::SurvID TrcKey::stdSynthSurvID()
{
    return Survey::GM().synthSurvID();
}


TrcKey::SurvID TrcKey::cUndefSurvID()
{
    return Survey::GeometryManager::cUndefGeomID();
}


float TrcKey::distTo( const TrcKey& trckey ) const
{
    const Coord from = Survey::GM().toCoord( *this );
    const Coord to = Survey::GM().toCoord( trckey );
    return from.isUdf() || to.isUdf() ? mUdf(float) : from.distTo<float>(to);
}


TrcKey& TrcKey::setGeomID( Pos::GeomID geomid )
{
    const Survey::Geometry* geom = Survey::GM().getGeometry( geomid );
    if ( !geom || !geom->is2D() )
	survid_ = geomid;
    else
    {
	survid_ = Survey::GeometryManager::get2DSurvID();
	pos_.inl() = geomid;
    }
    return *this;
}


TrcKey& TrcKey::setFrom( const Coord& crd )
{
    const Survey::Geometry* geom = Survey::GM().getGeometry( geomID() );
    if ( !geom )
    {
	geom = Survey::GM().getGeometry( std3DSurvID() );
	if ( !geom )
	{
	    pErrMsg( "No default Survey ID" );
	    pos_ = SI().transform( crd );
	    return *this;
	}
    }

    *this = geom->getTrace( crd, mUdf(float) );
    return *this;
}


Coord TrcKey::getCoord() const
{
    return Survey::Geometry::toCoord( *this );
}



// TrcKeyValue
TrcKeyValue::TrcKeyValue( const BinIDValue& bidv )
    : tk_( bidv )
    , val_( bidv.val() )
{}


const TrcKeyValue& TrcKeyValue::udf()
{
    mDefineStaticLocalObject( const TrcKeyValue, udfkey, );
    return udfkey;
}



// Pos::IdxPair2Coord
bool Pos::IdxPair2Coord::operator==( const Pos::IdxPair2Coord& oth ) const
{
    return mIsEqual(xtr.a,oth.xtr.a,1.) && mIsEqual(ytr.a,oth.ytr.a,1.)
	&& mIsEqual(xtr.b,oth.xtr.b,1e-3) && mIsEqual(ytr.b,oth.ytr.b,1e-3)
	&& mIsEqual(xtr.c,oth.xtr.c,1e-3) && mIsEqual(ytr.c,oth.ytr.c,1e-3);
}


bool Pos::IdxPair2Coord::isNodeOn( const Pos::IdxPair2Coord& oth,
				      int i0, int i1 ) const
{
    const Coord mycrd( transform(IdxPair(i0,i1)) );
    const Coord othcrd( oth.transform(oth.transformBack(mycrd)) );
    return mycrd.sqDistTo(othcrd) < 1.;
}


bool Pos::IdxPair2Coord::isSubsetOf( const Pos::IdxPair2Coord& oth ) const
{
    return isNodeOn(oth,0,0)
	&& isNodeOn(oth,1,1)
	&& isNodeOn(oth,2,2)
	&& isNodeOn(oth,3,3)
	&& isNodeOn(oth,5,5)
	&& isNodeOn(oth,7,7)
	&& isNodeOn(oth,11,11)
	&& isNodeOn(oth,0,1000)
	&& isNodeOn(oth,1000,0)
	&& isNodeOn(oth,1000,1000);
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
    nxtr.c = ( c0.x_ - c2.x_ ) / cold;
    nytr.c = ( c0.y_ - c2.y_ ) / cold;
    const od_int32 rowd = ip0.inl() - ip1.inl();
    cold = ip0.crl() - ip1.crl();
    nxtr.b = ( c0.x_ - c1.x_ ) / rowd - ( nxtr.c * cold / rowd );
    nytr.b = ( c0.y_ - c1.y_ ) / rowd - ( nytr.c * cold / rowd );
    nxtr.a = c0.x_ - nxtr.b * ip0.inl() - nxtr.c * ip0.crl();
    nytr.a = c0.y_ - nytr.b * ip0.inl() - nytr.c * ip0.crl();

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
    if ( mIsUdf(coord.x_) || mIsUdf(coord.y_) )
	return Pos::IdxPair::udf();
    const Coord fip = transformBackNoSnap( coord );
    if ( mIsUdf(fip.x_) || mIsUdf(fip.y_) )
	return Pos::IdxPair::udf();

    return IdxPair( mRounded(IdxType,fip.x_), mRounded(IdxType,fip.y_) );
}


Pos::IdxPair Pos::IdxPair2Coord::transformBack( const Coord& coord,
			const IdxPair& start, const IdxPairStep& step ) const
{
    if ( mIsUdf(coord.x_) || mIsUdf(coord.y_) )
	return Pos::IdxPair::udf();
    const Coord fip = transformBackNoSnap( coord );
    if ( mIsUdf(fip.x_) || mIsUdf(fip.y_) )
	return Pos::IdxPair::udf();

    Coord frelip( fip.x_ - start.first, fip.y_ - start.second );
    if ( step.first && step.second )
	{ frelip.x_ /= step.first; frelip.y_ /= step.second; }

    const IdxPair relip( mRounded(IdxType,frelip.x_),
			 mRounded(IdxType,frelip.y_) );
    return IdxPair( start.first + relip.first * step.first,
		    start.second + relip.second * step.second );
}


Coord Pos::IdxPair2Coord::transformBackNoSnap( const Coord& coord ) const
{
    if ( mIsUdf(coord.x_) || mIsUdf(coord.y_) )
	return Coord::udf();

    double det = xtr.det( ytr );
    if ( mIsZero(det,mDefEps) )
	return Coord::udf();

    const double x = coord.x_ - xtr.a;
    const double y = coord.y_ - ytr.a;
    return Coord( (x*ytr.c - y*xtr.c) / det, (y*xtr.b - x*ytr.b) / det );
}


Coord Pos::IdxPair2Coord::transform( const Coord& ip_coord ) const
{
    return Coord( xtr.a + xtr.b*ip_coord.x_ + xtr.c*ip_coord.y_,
		  ytr.a + ytr.b*ip_coord.x_ + ytr.c*ip_coord.y_ );
}
