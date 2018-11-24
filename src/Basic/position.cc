/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
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
#include "survgeom2d.h"
#include "survgeom3d.h"
#include "trckeyvalue.h"
#include "iopar.h"

#include <ctype.h>
#include <math.h>

#define mUdfIdx mUdf(IdxPair::pos_type)
static const IdxPair udfidxpair( mUdfIdx, mUdfIdx );
static const Pos::IdxPair udfposidxpair( mUdfIdx, mUdfIdx );
#define mUdfOrd mUdf(Pos::Ordinate_Type)
static const Coord udfcoord( mUdfOrd, mUdfOrd );
static const Coord3 udfcoord3( mUdfOrd, mUdfOrd, mUdfOrd );

static const char* sKeyXTransf = "Coord-X-BinID";
static const char* sKeyYTransf = "Coord-Y-BinID";


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

    const pos_type res = (pos_type)(diff.row()>0) + (pos_type)(diff.col()>0);
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


TrcKey::TrcKey( const BinID& bid )
    : geomsystem_(OD::VolBasedGeom)
    , pos_(bid)
{
}


TrcKey::TrcKey( GeomID id, tracenr_type tnr )
    : geomsystem_(id.is2D() ? OD::LineBasedGeom : OD::VolBasedGeom)
    , pos_(id.getI(),tnr)
{
}


TrcKey::TrcKey( GeomSystem gs, const BinID& bid )
    : geomsystem_(gs)
    , pos_(bid)
{
}


TrcKey::TrcKey( const BinID& bid, bool is2d )
    : geomsystem_(is2d ? OD::LineBasedGeom : OD::VolBasedGeom)
    , pos_(bid)
{
}


TrcKey TrcKey::getSynth( tracenr_type trcnr )
{
    TrcKey ret;
    ret.geomsystem_ = OD::SynthGeom;
    ret.pos_.inl() = -1;
    ret.pos_.crl() = trcnr;
    return ret;
}


Pos::GeomID TrcKey::geomID( GeomSystem gs, const BinID& bid )
{
    return gs < OD::LineBasedGeom ? GeomID( (GeomID::IDType)gs )
				  : GeomID( bid.lineNr() );
}


TrcKey& TrcKey::setGeomSystem( GeomSystem gs )
{
    geomsystem_ = gs;
    return *this;
}


TrcKey& TrcKey::setGeomID( GeomID geomid )
{
    geomsystem_ = geomSystemOf( geomid );
    if ( geomsystem_ == OD::LineBasedGeom )
	pos_.lineNr() = geomid.lineNr();
    return *this;
}


const TrcKey& TrcKey::udf()
{
    static const TrcKey udf( OD::SynthGeom, BinID::udf() );
    return udf;
}


bool TrcKey::isUdf() const
{
    return mIsUdf(pos_.col()) || mIsUdf(pos_.row());
}


bool TrcKey::exists() const
{
    if ( isUdf() )
	return false;
    else if ( is3D() )
	return Survey::Geometry::get3D().includes( pos_ );

    const auto gid( geomID() );
    const auto& geom2d = Survey::Geometry::get2D( gid );
    return geom2d.includes( gid.lineNr() );

}


TrcKey TrcKey::getFor( GeomID gid ) const
{
    const auto gs = geomSystemOf( gid );
    if ( gs == geomsystem_ )
	return *this;
    else if ( isUdf() )
	return TrcKey( gs, BinID::udf() );

    TrcKey tk( geomsystem_, BinID(gid.lineNr(),0) );
    tk.setFrom( getCoord() );
    return tk;
}


TrcKey::dist_type TrcKey::distTo( const TrcKey& oth ) const
{
    const Coord from = getCoord();
    const Coord to = oth.getCoord();
    return from.isUdf() || to.isUdf() ? mUdf(double)
				      : from.distTo<double>( to );
}


const Survey::Geometry& TrcKey::geometry() const
{
    return Survey::Geometry::get( geomID() );
}


TrcKey& TrcKey::setFrom( const Coord& crd )
{
    const auto& geom = geometry();
    if ( geom.is3D() )
	setBinID( geom.as3D()->transform(crd) );
    else if ( !geom.as2D()->isEmpty() )
	setTrcNr( geom.as2D()->nearestTracePosition(crd) );
    return *this;
}


Coord TrcKey::getCoord() const
{
    if ( geomsystem_ == OD::VolBasedGeom )
	return Survey::Geometry::get3D().transform( pos_ );

    const auto& geom2d = Survey::Geometry::get2D( geomID() );
    return geom2d.isEmpty() ? Coord::udf() : geom2d.getCoord( trcNr() );
}



// TrcKeyValue
TrcKeyValue::TrcKeyValue( const BinIDValue& bidv )
    : tk_(bidv)
    , val_(bidv.val())
{
}


const TrcKeyValue& TrcKeyValue::udf()
{
    static const TrcKeyValue udftkv;
    return udftkv;
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

    return IdxPair( mRounded(pos_type,fip.x_), mRounded(pos_type,fip.y_) );
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

    const IdxPair relip( mRounded(pos_type,frelip.x_),
			 mRounded(pos_type,frelip.y_) );
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


void Pos::IdxPair2Coord::fillPar( IOPar& iop ) const
{
    iop.set( sKeyXTransf, xtr.a, xtr.b, xtr.c );
    iop.set( sKeyYTransf, ytr.a, ytr.b, ytr.c );
}


void Pos::IdxPair2Coord::usePar( const IOPar& iop )
{
    iop.get( sKeyXTransf, xtr.a, xtr.b, xtr.c );
    iop.get( sKeyYTransf, ytr.a, ytr.b, ytr.c );
}


void Pos::IdxPair2Coord::fillBuf( void* buf ) const
{
    double* ptr = (double*)buf;
    *ptr++ = xtr.a; *ptr++ = xtr.b; *ptr++ = xtr.c;
    *ptr++ = ytr.a; *ptr++ = ytr.b; *ptr = ytr.c;
}


void Pos::IdxPair2Coord::useBuf( const void* buf )
{
    const double* ptr = (const double*)buf;
    xtr.a = *ptr++; xtr.b = *ptr++; xtr.c = *ptr++;
    ytr.a = *ptr++; ytr.b = *ptr++; ytr.c = *ptr;
}
