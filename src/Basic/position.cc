/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "position.h"
#include "binidvalue.h"

#include "bufstring.h"
#include "keystrs.h"
#include "math2.h"
#include "posidxpair2coord.h"
#include "string2.h"
#include "survgeom2d.h"
#include "survinfo.h" // fallback with pErrMsg only
#include "trckeyvalue.h"
#include "undefval.h"

#include <ctype.h>

#define mUdfIdx mUdf(IdxPair::IdxType)
static const IdxPair udfidxpair( mUdfIdx, mUdfIdx );
static const Pos::IdxPair udfposidxpair( mUdfIdx, mUdfIdx );
#define mUdfOrd mUdf(Coord::OrdType)
static const Coord udfcoord( mUdfOrd, mUdfOrd );
static const Coord3 udfcoord3( mUdfOrd, mUdfOrd, mUdfOrd );
static const char* sKeyXTransf = "Coord-X-BinID";
static const char* sKeyYTransf = "Coord-Y-BinID";


IdxPair::IdxPair()
    : std::pair<od_int32,od_int32>(0,0)
{}


IdxPair::IdxPair( od_int32 f, od_int32 s )
    : std::pair<od_int32,od_int32>(f,s)
{}


IdxPair::~IdxPair()
{}


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
    if ( !ptr1st || !*ptr1st )
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


// Pos::IdxPair
Pos::IdxPair::IdxPair()
    : ::IdxPair(0,0)
{}


Pos::IdxPair::IdxPair( IdxType f, IdxType s )
    : ::IdxPair(f,s)
{}


Pos::IdxPair::IdxPair( const Pos::IdxPair& oth )
    : ::IdxPair(oth.first,oth.second)
{
}


Pos::IdxPair::~IdxPair()
{}


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


// BinIDValue
BinIDValue::BinIDValue( BinID::IdxType i, BinID::IdxType c, float v )
    : BinIDValueIdxPair(i,c,v)
{}


BinIDValue::BinIDValue( const BinID& b, float v )
    : BinIDValueIdxPair(b,v)
{}


BinIDValue::BinIDValue( const BinIDValues& bvs, int nr )
{
    set( bvs, nr );
}


BinIDValue::~BinIDValue()
{}


// BinIDValues
BinIDValues::BinIDValues( BinID::IdxType i, BinID::IdxType c, int n )
    : BinIDIdxPairValues(i,c,n)
{}


BinIDValues::BinIDValues( const BinID& b, int n )
    : BinIDIdxPairValues(b,n)
{}


BinIDValues::BinIDValues( const BinIDValue& biv )
    : BinIDIdxPairValues(biv)
{}


BinIDValues::~BinIDValues()
{}


// Coord
Coord::Coord( const Geom::Point2D<OrdType>& p )
    :  Geom::Point2D<OrdType>( p )
{}


Coord::Coord()
    :  Geom::Point2D<OrdType>(0,0)
{}


Coord::Coord( OrdType cx, OrdType cy )
    :  Geom::Point2D<OrdType>( cx, cy )
{}


Coord::~Coord()
{}


Coord::DistType Coord::sqHorDistTo( const Coord& oth ) const
{
    const DistType dx = x_-oth.x_, dy = y_-oth.y_;
    return dx*dx + dy*dy;
}


Coord::DistType Coord::horDistTo( const Coord& oth ) const
{
    return Math::Sqrt( sqHorDistTo(oth) );
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
    return x_*b.x_ + y_*b.y_;
}


const Coord& Coord::udf()
{
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


Coord::DistType Coord::angle( const Coord& from, const Coord& to ) const
{
    const DistType cosang = cosAngle( from, to );
    if ( cosang >=  1 ) return 0;
    if ( cosang <= -1 ) return M_PI;

    const Coord vec1 = from - *this;
    const Coord vec2 =  to  - *this;
    const DistType det = vec1.x_ * vec2.y_ - vec1.y_ * vec2.x_;

    const DistType ang = Math::ACos( cosang );
    return det<0 ? 2*M_PI - ang : ang;
}


const char* Coord::toString() const
{
    mDeclStaticString( ret );
    if ( isUdf() )
	ret.set( "<undef>" );
    else
        ret.set( "(" ).add( toStringPrecise( x_ ) )
	   .add( "," ).add( toStringPrecise( y_ ) ).add( ")" );

    return ret.buf();
}


const char* Coord::toPrettyString( int nrdec ) const
{
    mDeclStaticString( ret );
    if ( isUdf() )
	ret.set( "<undef>" );
    else
    {
	BufferString xstr = ::toString( x_, 0, 'f', nrdec);
	BufferString ystr = ::toString( y_, 0, 'f', nrdec);
	ret.set( "(" ).add( xstr ).add( "," ).add( ystr ).add( ")" );
    }
    return ret.buf();
}


bool Coord::fromString( const char* s )
{
    if ( !s || !*s ) return false;
    if ( *s == '<' )
	{ *this = udf(); return true; }

    BufferString str( s );
    char* ptrx = str.getCStr(); mSkipBlanks( ptrx );
    if ( ptrx && *ptrx == '(' ) ptrx++;
    char* ptry = firstOcc( ptrx, ',' );
    if ( !ptry ) return false;
    *ptry++ = '\0';
    if ( !*ptry ) return false;
    char* ptrend = firstOcc( ptry, ')' );
    if ( ptrend ) *ptrend = '\0';

    x_ = toDouble( ptrx );
    y_ = toDouble( ptry );
    return true;
}


// Coord3

mStartAllowDeprecatedSection

Coord3::Coord3()
    : Coord()
    , z_(0)
    , z(z_)
{}


Coord3::Coord3( const Coord& a, OrdType _z )
    : Coord(a)
    , z_(_z)
    , z(z_)
{}


Coord3::Coord3(const Coord3& xyz )
    : Coord( xyz.x_, xyz.y_ )
    , z_( xyz.z_ )
    , z(z_)
{}


Coord3::Coord3( OrdType _x, OrdType _y, OrdType _z )
    : Coord(_x,_y)
    , z_(_z)
    , z(z_)
{}

mStopAllowDeprecatedSection


Coord3::~Coord3()
{}


Coord::DistType Coord3::abs() const
{
    return Math::Sqrt( x_*x_ + y_*y_ + z_*z_ );
}


Coord::DistType Coord3::sqAbs() const { return x_*x_ + y_*y_ + z_*z_; }


const char* Coord3::toString() const
{
    mDeclStaticString( ret );
    if ( isUdf() )
	ret.set( "<undef>" );
    else
        ret.set("(").add(x_).add(",").add(y_).add(",").add(z_).add(")");
    return ret.buf();
}


bool Coord3::fromString( const char* str )
{
    StringView fs( str );
    if ( fs.isEmpty() ) return false;

    const char* endptr = str + fs.size();

    while ( !iswdigit(*str) && *str!='+' && *str!='-' && str!=endptr )
	str++;

    char* numendptr;
    x_ = strtod( str, &numendptr );
    if ( str==numendptr ) return false;

    str = numendptr;
    while ( !iswdigit(*str) && *str!='+' && *str!='-' && str!=endptr )
	str++;
    y_ = strtod( str, &numendptr );
    if ( str==numendptr ) return false;

    str = numendptr;
    while ( !iswdigit(*str) && *str!='+' && *str!='-' && str!=endptr )
	str++;
    z_ = strtod( str, &numendptr );
    if ( str==numendptr ) return false;

    return true;
}


Coord::DistType Coord3::distTo( const Coord3& b ) const
{
    return Math::Sqrt( Coord3::sqDistTo( b ) );
}


Coord::DistType Coord3::sqDistTo( const Coord3& b ) const
{
    const DistType dx = x_-b.x_, dy = y_-b.y_, dz = z_-b.z_;
    return dx*dx + dy*dy + dz*dz;
}


bool Coord3::isSameAs( const Coord3& pos, const Coord3& eps ) const
{
    return fabs(x_-pos.x_)<eps.x_ &&
	   fabs(y_-pos.y_)<eps.y_ &&
	   fabs(z_-pos.z_)<eps.z_;
}


const Coord3& Coord3::udf()
{
   return udfcoord3;
}


// TrcKey

TrcKey::TrcKey()
{
    *this = udf();
}


TrcKey::TrcKey( const BinID& bid )
    : geomsystem_( OD::Geom3D )
    , pos_( bid )
{
}


TrcKey::TrcKey( const Pos::GeomID& id, Pos::TraceID tid )
    : geomsystem_( OD::Geom2D )
    , pos_( id.asInt(), tid )
{
}


TrcKey::TrcKey( const Pos::IdxPair& pos, bool is2d )
    : geomsystem_( is2d ? OD::Geom2D : OD::Geom3D )
    , pos_( pos )
{
}


TrcKey::TrcKey( OD::GeomSystem gs, const Pos::IdxPair& bid )
    : geomsystem_( gs )
    , pos_( bid )
{
}


TrcKey TrcKey::getSynth( Pos::TraceID tid )
{
    return TrcKey( OD::GeomSynth,
		   Pos::IdxPair( gtGeomID(OD::GeomSynth).asInt(), tid ) );
}


TrcKey::~TrcKey()
{
}


bool TrcKey::operator==( const TrcKey& oth ) const
{ return oth.geomsystem_==geomsystem_ && oth.pos_==pos_; }


bool TrcKey::exists() const
{
    if ( isUdf() )
	return false;
    else if ( is3D() )
	return geometry().includes( pos_ );
    else if ( isSynthetic() )
	return geomID() == gtGeomID( OD::GeomSynth ) && !mIsUdf(trcNr());

    const Survey::Geometry2D& geom2d = Survey::GM().get2D( geomID() );
    return geom2d.isEmpty() ? false : geom2d.includes( lineNr(), trcNr() );
}


Pos::GeomID TrcKey::geomID() const
{
    return gtGeomID( geomsystem_, pos_.row() );
}


Pos::GeomID TrcKey::gtGeomID( OD::GeomSystem gs, IdxType lnr )
{
    return gs < OD::Geom2D ? Pos::GeomID( gs ) : Pos::GeomID( lnr );
}


Pos::IdxPair TrcKey::idxPair() const
{
    return Pos::IdxPair( pos_.row(), pos_.col() );
}


TrcKey& TrcKey::setGeomID( const Pos::GeomID& geomid )
{
    geomsystem_ = geomid.isUdf() ? udf().geomSystem() : geomid.geomSystem();
    if ( !is3D() ) //Also for synthetic
	setLineNr( geomid.asInt() );

    return *this;
}


TrcKey& TrcKey::setSurvID( OD::GeomSystem gs )
{
    return setGeomSystem( gs );
}


TrcKey& TrcKey::setGeomSystem( OD::GeomSystem gs )
{
    geomsystem_ = gs;
    if ( isSynthetic() )
	setGeomID( Pos::GeomID(OD::GeomSynth) );

    return *this;
}


TrcKey& TrcKey::setPosition( const BinID& bid )
{
    setGeomSystem( OD::Geom3D );
    pos_ = bid;
    return *this;
}


TrcKey& TrcKey::setPosition( const Pos::IdxPair& pos, bool is2d )
{
    setGeomSystem( is2D() ? OD::Geom2D : OD::Geom3D );
    pos_ = pos;
    return *this;
}


namespace Pos
{

static IdxPair2Coord& getSyntheticB2C()
{
    static PtrMan<IdxPair2Coord> synthb2c;
    if ( !synthb2c )
    {
	auto* newb2c = new IdxPair2Coord;
	const Coord c0( 500000., 4000000. );
	const Coord c1( 500000., 4000025. );
	const Coord c2( 500025., 4000000. );
	const IdxPair rc0( 1, 1 );
	const IdxPair rc1( 2, 1 );
	const IdxPair rc2( 1, 2 );
	newb2c->set3Pts( c0, c1, c2, rc0, rc1, rc2.col() );
	synthb2c.setIfNull( newb2c, true );
    }

    return *synthb2c.ptr();
}

} // namespace Pos

TrcKey& TrcKey::setFrom( const Coord& crd )
{
    if ( is3D() )
	setPosition( geometry().as3D()->transform( crd ) );
    else if ( isSynthetic() )
    {
	setGeomID( gtGeomID(OD::GeomSynth) );
	if ( Survey::GM().getGeometry(Survey::default3DGeomID()) )
	{
	    const BinID bid = geometry().as3D()->transform( crd );
	    setTrcNr( (bid.trcNr() - SI().crlRange().stop_) /
		       SI().crlRange().step_ );
	}
	else
	{
	    const BinID bid = Pos::getSyntheticB2C().transformBack( crd );
	    setTrcNr( bid.trcNr() );
	}
    }
    else
    {
	const Pos::GeomID gid = const_cast<const TrcKey&>(*this).geomID();
	const Survey::Geometry2D& geom2d = Survey::GM().get2D( gid );
	if ( !geom2d.isEmpty() )
	{
	    int trcnr = const_cast<const TrcKey&>(*this).trcNr();
	    float sp = mUdf(float);
	    geom2d.getPosByCoord( crd, trcnr, sp );
	    setTrcNr( trcnr );
	}
    }

    return *this;
}


Coord TrcKey::getCoord() const
{
    if ( is3D() )
	return geometry().as3D()->transform( pos_ );
    else if ( isSynthetic() )
    {
	if ( Survey::GM().getGeometry(Survey::default3DGeomID()) )
	{ //To ensure it never falls within SI()
	    const BinID pos( SI().inlRange().stop_ + SI().inlRange().step_,
			     SI().crlRange().stop_ +
			     SI().crlRange().step_ * trcNr() );
	    return geometry().as3D()->transform( pos );
	}
	else
	{ //No current survey, return 'plausible' coordinates
	    const BinID pos( 1, trcNr() );
	    return Pos::getSyntheticB2C().transform( pos );
	}
    }

    const Survey::Geometry2D& geom2d = Survey::GM().get2D( geomID() );
    return geom2d.isEmpty() ? Coord::udf() : geom2d.toCoord( trcNr() );
}


double TrcKey::sqDistTo( const TrcKey& oth ) const
{
    const Coord from = getCoord();
    const Coord to = oth.getCoord();
    return from.isUdf() || to.isUdf() ? mUdf(double)
				      : from.sqDistTo( to );
}


double TrcKey::distTo( const TrcKey& oth ) const
{
    const double sqdist = sqDistTo( oth );
    return mIsUdf(sqdist) ? sqdist : Math::Sqrt( sqdist );
}


const Survey::Geometry& TrcKey::geometry() const
{
    return is2D() ? (const Survey::Geometry&) Survey::GM().get2D( geomID() )
		  : (const Survey::Geometry&)Survey::Geometry3D::instance();
}


TrcKey TrcKey::getFor( const Pos::GeomID& gid ) const
{
    const OD::GeomSystem gs = gid.geomSystem();
    if ( gs == geomsystem_ )
	return *this;

    if ( isUdf() )
	return TrcKey( gs, Pos::IdxPair::udf() );

    TrcKey tk( geomsystem_, Pos::IdxPair(gid.asInt(),0) );
    tk.setFrom( getCoord() );
    return tk;
}


TrcKey TrcKey::getFor3D() const
{
    return getFor( Survey::default3DGeomID() );
}


TrcKey TrcKey::getFor2D( IdxType linenr ) const
{
    return getFor( Pos::GeomID(linenr) );
}


BufferString TrcKey::usrDispStr() const
{
    if ( isUdf() )
	return BufferString( sKey::Undef() );

    BufferString ret;
    switch ( geomsystem_ )
    {
	case OD::Geom3D:
	    ret.set( pos_.usrDispStr() );
	break;
	case OD::Geom2D:
	    ret.set( trcNr() ).add( "@'" )
	       .add( Survey::GM().getName(geomID()) ).add( "'" );
	break;
	case OD::GeomSynth:
	    ret.set( "[Synth]@" ).add( trcNr() );
	break;
	default:
	    ret.set( udf().usrDispStr() );
	break;
    }

    return ret;
}


const TrcKey& TrcKey::udf()
{
    static const TrcKey udf( OD::GeomSynth, Pos::IdxPair::udf() );
    return udf;
}


// Deprecated implementations of TrcKey:

TrcKey::TrcKey( OD::GeomSystem gs, const BinID& bid )
    : geomsystem_( gs )
    , pos_( bid )
{
}


Pos::GeomID TrcKey::geomID( OD::GeomSystem gs, const BinID& bid )
{
    const TrcKey tk( gs, (const Pos::IdxPair&)(bid) );
    return tk.geomID();
}


TrcKey::IdxType& TrcKey::lineNr()
{
    return pos_.lineNr();
}


TrcKey::IdxType& TrcKey::trcNr()
{
    return pos_.trcNr();
}



// TrcKeyValue

TrcKeyValue::TrcKeyValue( const TrcKey& tk, float val )
    : tk_(tk)
    , val_(val)
{}


TrcKeyValue::TrcKeyValue( const BinID& bid, float val )
    : tk_(bid)
    , val_(val)
{}


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


bool Pos::IdxPair2Coord::operator!=( const Pos::IdxPair2Coord& oth ) const
{
    return !(*this == oth);
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
    if ( ip1.row() == ip0.row() )
	return false;
    if ( ip0.col() == crl2 )
        return false;

    DirTransform nxtr, nytr;
    od_int32 cold = ip0.col() - crl2;
    nxtr.c = ( c0.x_ - c2.x_ ) / cold;
    nytr.c = ( c0.y_ - c2.y_ ) / cold;
    const od_int32 rowd = ip0.row() - ip1.row();
    cold = ip0.col() - ip1.col();
    nxtr.b = ( c0.x_ - c1.x_ ) / rowd - ( nxtr.c * cold / rowd );
    nytr.b = ( c0.y_ - c1.y_ ) / rowd - ( nytr.c * cold / rowd );
    nxtr.a = c0.x_ - nxtr.b * ip0.row() - nxtr.c * ip0.col();
    nytr.a = c0.y_ - nytr.b * ip0.row() - nytr.c * ip0.col();

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
