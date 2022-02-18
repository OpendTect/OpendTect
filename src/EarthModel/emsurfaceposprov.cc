/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2005
-*/


#include "emsurfaceposprov.h"

#include "arrayndimpl.h"
#include "datapointset.h"
#include "embody.h"
#include "emhorizon2d.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emregion.h"
#include "emrowcoliterator.h"
#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "emsurfaceiodata.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "posinfo2d.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "uistrings.h"
#include "statrand.h"

#include "errmsg.h"

namespace Pos
{

const char* EMSurfaceProvider::id1Key()		{ return "ID.1"; }
const char* EMSurfaceProvider::id2Key()		{ return "ID.2"; }
const char* EMSurfaceProvider::zstepKey()	{ return "Z Step"; }
const char* EMSurfaceProvider::extraZKey()	{ return "Extra Z"; }


EMSurfaceProvider::EMSurfaceProvider()
    : surf1_(0)
    , surf2_(0)
    , hs_(SI().sampling(false).hsamp_)
    , zstep_(SI().zRange(true).step)
    , extraz_(0,0)
    , zrg1_(0,0)
    , zrg2_(0,0)
    , iterator_(0)
    , curz_(mUdf(float))
    , curzrg_(0,0)
    , estnrpos_(-1)
    , nrsamples_(mUdf(int))
    , dorandom_(false)
    , enoughsamples_(true)
    , maxidx_(0)
{}


EMSurfaceProvider::EMSurfaceProvider( const EMSurfaceProvider& pp )
    : surf1_(0)
    , surf2_(0)
{
    *this = pp;
}


EMSurfaceProvider::~EMSurfaceProvider()
{
    if ( surf1_ ) surf1_->unRef();
    if ( surf2_ ) surf2_->unRef();
}


void EMSurfaceProvider::copyFrom( const EMSurfaceProvider& pp )
{
    if ( &pp == this ) return;

    if ( surf1_ ) surf1_->unRef();
    if ( surf2_ ) surf2_->unRef();
    surf1_ = pp.surf1_; if ( surf1_ ) surf1_->ref();
    surf2_ = pp.surf2_; if ( surf2_ ) surf2_->ref();
    zstep_ = pp.zstep_;
    extraz_ = pp.extraz_;
    id1_ = pp.id1_;
    id2_ = pp.id2_;
    hs_ = pp.hs_;
    zrg1_ = pp.zrg1_; zrg2_ = pp.zrg2_;
    nrsamples_ = pp.nrsamples_;
    dorandom_ = pp.dorandom_;
    enoughsamples_ = pp.enoughsamples_;
    maxidx_ = pp.maxidx_;
    estnrpos_ = -1;
}


const char* EMSurfaceProvider::type() const
{ return sKey::Surface(); }


static void getSurfRanges( const EM::Surface& surf, TrcKeySampling& hs,
			   Interval<float>& zrg, od_int64& estnrpos )
{
    bool veryfirst = true;
    for ( int idx=0; idx<surf.nrSections(); idx++ )
    {
	EM::RowColIterator it( surf, surf.sectionID(idx) );
	EM::PosID posid = it.next();
	while ( posid.objectID() != -1 )
	{
	    const Coord3 coord = surf.getPos( posid );
	    const BinID bid( SI().transform(coord) );
	    if ( veryfirst )
	    {
		veryfirst = false;
		hs.start_ = hs.stop_ = bid;
		zrg.start = zrg.stop = (float) coord.z;
	    }
	    else
	    {
		if ( coord.z < zrg.start ) zrg.start = (float) coord.z;
		if ( coord.z > zrg.stop ) zrg.stop = (float) coord.z;
		hs.include( bid );
	    }
	    estnrpos++;

	    posid = it.next();
	}
    }
}


bool EMSurfaceProvider::initialize( TaskRunner* taskr )
{
    if ( nrSurfaces() == 0 ) return false;

    EM::EMObject* emobj = EM::EMM().getObject( EM::EMM().getObjectID(id1_) );
    if ( !emobj )
	emobj = EM::EMM().loadIfNotFullyLoaded( id1_, taskr );
    mDynamicCastGet(EM::Surface*,surf1,emobj)
    if ( !surf1 ) return false;
    surf1_ = surf1; surf1_->ref();

    getSurfRanges( *surf1_, hs_, zrg1_, estnrpos_ );

    if ( !id2_.isUdf() )
    {
	emobj = EM::EMM().getObject( EM::EMM().getObjectID(id2_) );
	if ( !emobj )
	    emobj = EM::EMM().loadIfNotFullyLoaded( id2_, taskr );
	mDynamicCastGet(EM::Surface*,surf2,emobj)
	if ( !surf2 ) return false;
	surf2_ = surf2; surf2_->ref();
	TrcKeySampling hs( hs_ );
	od_int64 estnrpos2 = -1;
	getSurfRanges( *surf2_, hs, zrg2_, estnrpos2 );
	if ( estnrpos2 < estnrpos_ )
	    estnrpos_ = estnrpos2;
	hs_.limitTo( hs );
    }

    reset();

    if ( dorandom_ )
    {
	maxidx_ = iterator_->maxIndex();
	// Need to avoid situation where there aren't enough samples available
	// to avoid an infinite loop when sampling
	// This is a crude lower bound, need to think about using zrg's as well
	enoughsamples_ = nrsamples_ < maxidx_ ;
	estnrpos_ = nrsamples_;
    }

    return true;
}


void EMSurfaceProvider::reset()
{
    delete iterator_; iterator_ = 0;
    if ( surf1_ )
	iterator_ = new EM::RowColIterator( *surf1_, surf1_->sectionID(0) );
    curpos_ = EM::PosID( -1, -1, -1 );
}


bool EMSurfaceProvider::toNextPos()
{
    if ( dorandom_  && enoughsamples_ )
    {
	postuple pos;
	od_int64 idx;
	const Stats::RandGen& randGen = Stats::randGen();
	do
	{
	    idx = randGen.getIndex( maxidx_ );
	    curpos_ = iterator_->fromIndex( idx );
	    curz_ = mUdf(float);

	    if ( curpos_.objectID() == -1 )
		return false;

	    curzrg_.start = curzrg_.stop =
			    (float) surf1_->getPos( curpos_ ).z;

	    if ( surf2_ )
	    {
		const float stop =
		(float) surf2_->getPos( surf2_->sectionID(0),
					curpos_.subID()).z;
		if ( !mIsUdf(stop) )
		    curzrg_.stop = stop;
	    }

	    curzrg_.sort();
	    curzrg_ += extraz_;

	    if ( surf2_ )
	    {
		SI().snapZ( curzrg_.start, 1 );
		SI().snapZ( curzrg_.stop, -1 );
	    }

	    const TrcKeyZSampling& tkzs = SI().sampling( false );
	    if ( !mIsUdf(curzrg_.start) && !mIsUdf(curzrg_.stop) )
	    {
		int zsamp = randGen.getInt( tkzs.zIdx( curzrg_.start ),
						 tkzs.zIdx( curzrg_.stop ) );
		curz_ = tkzs.zAtIndex( zsamp );
		pos = postuple( idx, zsamp );
	    }
	} while ( mIsUdf(curz_) ? false : posindexlst_.isPresent(pos) );
	posindexlst_ += pos;
	if ( posindexlst_.size() > nrsamples_ )
	    return false;
    }
    else
    {
	curpos_ = iterator_->next();
	if ( curpos_.objectID() == -1 )
	    return false;

	curzrg_.start = curzrg_.stop = (float) surf1_->getPos( curpos_ ).z;
	if ( surf2_ )
	{
	    const float stop =
	    (float) surf2_->getPos( surf2_->sectionID(0), curpos_.subID()).z;
	    if ( !mIsUdf(stop) )
		curzrg_.stop = stop;
	}

	curzrg_.sort();
	curzrg_ += extraz_;

	Interval<float> unsnappedzrg = curzrg_;
	if ( surf2_ )
	{
	    SI().snapZ( curzrg_.start, 1 );
	    SI().snapZ( curzrg_.stop, -1 );
	    if ( !unsnappedzrg.includes(curzrg_.start, false) )
	    {
		curz_ = mUdf(float);
		return true;
	    }
	}

	curz_ = curzrg_.start;
    }
    return true;
}


bool EMSurfaceProvider::toNextZ()
{
    if ( dorandom_ )
	return toNextPos();
    else
    {
	if ( mIsUdf(curz_) || (!surf2_ && mIsZero(extraz_.width(),mDefEps) ) )
	    return toNextPos();

	curz_ += zstep_;
	if ( curz_ > (curzrg_.stop+mDefEps) )
	    return toNextPos();
    }

    return true;
}


float EMSurfaceProvider::curZ() const
{ return curz_; }


bool EMSurfaceProvider::hasZAdjustment() const
{ return surf1_ && !surf2_; }


float EMSurfaceProvider::adjustedZ( const Coord& c, float z ) const
{
    if ( !hasZAdjustment() ) return z;

    const BinID bid = SI().transform( c );
    return (float) surf1_->getPos( surf1_->sectionID(0), bid.toInt64() ).z;
}


#define mGetSurfKey(k) IOPar::compKey(sKey::Surface(),k)

void EMSurfaceProvider::usePar( const IOPar& iop )
{
    dorandom_ = false;
    iop.getYN( sKey::Random(), dorandom_ );
    if ( dorandom_ )
	iop.get( sKey::NrValues(), nrsamples_);

    iop.get( mGetSurfKey(id1Key()), id1_ );
    iop.get( mGetSurfKey(id2Key()), id2_ );
    iop.get( mGetSurfKey(zstepKey()), zstep_ );
    iop.get( mGetSurfKey(extraZKey()), extraz_ );

    if ( id1_.isUdf() ) return;
    EM::SurfaceIOData sd;
    EM::IOObjInfo eminfo( id1_ );
    if ( !eminfo.isOK() ) return;

    TrcKeySampling hs;
    hs.set( eminfo.getInlRange(), eminfo.getCrlRange() );
    hs_ = hs;

    if ( id2_.isUdf() ) return;
    eminfo = EM::IOObjInfo( id2_ );
    if ( !eminfo.isOK() ) return;
    hs.set( eminfo.getInlRange(), eminfo.getCrlRange() );
    hs_.limitTo( hs );
    // TODO: get zrg's
}


void EMSurfaceProvider::fillPar( IOPar& iop ) const
{
    iop.set( mGetSurfKey(id1Key()), id1_ );
    iop.set( mGetSurfKey(id2Key()), id2_ );
    iop.set( mGetSurfKey(zstepKey()), zstep_ );
    iop.set( mGetSurfKey(extraZKey()), extraz_ );
    iop.setYN( sKey::Random(), dorandom_ );
    if ( dorandom_ )
	iop.set( sKey::NrValues(), nrsamples_ );
}


void EMSurfaceProvider::getSummary( BufferString& txt ) const
{
    switch ( nrSurfaces() )
    {
	case 0:
	return;
	case 1:
	    txt += "On '"; txt += IOM().nameOf(id1_); txt += "'";
	break;
	default:
	    txt += "Between '"; txt += IOM().nameOf(id1_);
	    txt += "' and '"; txt += IOM().nameOf(id2_);
	break;
    }
}


bool EMSurfaceProvider::getZRange( const TrcKey& tk,
				   Interval<float>& zrg ) const
{
    zrg.setUdf();
    mDynamicCastGet(EM::Horizon*,hor1,surf1_)
    if ( !hor1 )
	return false;

    const float z1 = hor1->getZ( tk );
    if ( mIsUdf(z1) )
	return false;

    mDynamicCastGet(EM::Horizon*,hor2,surf2_)
    if ( hor2 )
    {
	const float z2 = hor2->getZ( tk );
	if ( mIsUdf(z2) )
	    return false;

	zrg.set( z1, z2 );
	zrg.sort();
    }
    else
    {
	zrg.start = z1 - SI().zStep()/2;
	zrg.stop = z1 + SI().zStep()/2;
    }

    zrg += extraz_;
    return true;
}


void EMSurfaceProvider::getZRange( Interval<float>& zrg ) const
{
    if ( !surf1_ ) return;

    zrg = zrg1_;

    if ( surf2_ )
    {
	if ( zrg2_.start < zrg.start ) zrg.start = zrg2_.start;
	if ( zrg2_.stop > zrg.stop ) zrg.stop = zrg2_.stop;
    }
}


int EMSurfaceProvider::estNrZPerPos() const
{
    if ( !dorandom_ ) {
	if ( !surf2_ ) return 1;
	Interval<float> avgzrg( (zrg1_.start + zrg2_.start)*.5f,
				(zrg1_.stop + zrg2_.stop)*.5f );
	return (int)((avgzrg.stop-avgzrg.start) / zstep_ + .5);
    }
    else
	return 1;
}


int EMSurfaceProvider::nrSurfaces() const
{
    if ( !surf1_ )
	return id1_.isUdf() ? 0 : (id2_.isUdf() ? 1 : 2);
    return surf2_ ? 2 : 1;
}


BinID EMSurfaceProvider3D::curBinID() const
{ return BinID::fromInt64( curpos_.subID() ); }


bool EMSurfaceProvider3D::includes( const BinID& bid, float z ) const
{
    if ( !surf1_ )
	return true;

    Interval<float> zrg;
    const bool res =
	EMSurfaceProvider::getZRange( TrcKey(bid), zrg );
    if ( !res || zrg.isUdf() )
	return false;

    return mIsUdf(z) ? true : zrg.includes( z, false );
}


void EMSurfaceProvider3D::getExtent( BinID& start, BinID& stop ) const
{ start = hs_.start_; stop = hs_.stop_; }


void EMSurfaceProvider3D::initClass()
{ Provider3D::factory().addCreator( create, sKey::Surface(),
				    uiStrings::sHorizon() ); }


// ***** EMSurfaceProvider2D ****
const char* EMSurfaceProvider2D::curLine() const
{
    BinID bid = BinID::fromInt64( curpos_.subID() );
    if ( surf1_ )
    {
	mDynamicCastGet(EM::Horizon2D*,hor2d,surf1_);
	if ( !hor2d )
	    return 0;

	return hor2d->geometry().lineName( bid.inl() );
    }

    return 0;
}


int EMSurfaceProvider2D::curNr() const
{ return BinID::fromInt64( curpos_.subID() ).trcNr(); }


Coord EMSurfaceProvider2D::curCoord() const
{ return surf1_ ? surf1_->getPos( curpos_ ) : Coord(0,0); }


TrcKey EMSurfaceProvider2D::curTrcKey() const
{
    mDynamicCastGet(EM::Horizon2D*,hor2d,surf1_);
    if ( !hor2d )
	return TrcKey::udf();

    return hor2d->geometry().getTrcKey( curpos_ );
}


bool EMSurfaceProvider2D::includes( const Coord& c, float z ) const
{
    PosInfo::Line2DPos pos;
    for ( int lidx=0; lidx<nrLines(); lidx++ )
    {
	mDynamicCastGet( const Survey::Geometry2D*, geom2d,
			 Survey::GM().getGeometry(geomID(lidx)) );
	if ( !geom2d )
	    continue;

	const PosInfo::Line2DData& l2d = geom2d->data();
	if ( l2d.getPos(c,pos,SI().inlDistance()))
	{
	    if ( includes(pos.nr_,z,lidx) )
		return true;
	}
    }

    return false;
}


bool EMSurfaceProvider2D::includes( int trcnr, float z, int lidx ) const
{
    Interval<float> zrg;
    const bool res =
	EMSurfaceProvider::getZRange( TrcKey(geomID(lidx),trcnr), zrg );
    if ( !res || zrg.isUdf() )
	return false;

    return mIsUdf(z) ? true : zrg.includes( z, false );
}


void EMSurfaceProvider2D::getExtent( Interval<int>& intv, int lidx ) const
{ intv.start = intv.stop = 0; }


void EMSurfaceProvider2D::initClass()
{ Provider2D::factory().addCreator( create, sKey::Surface() ); }


// ***** EMSurface2DProvider3D ****
EMSurface2DProvider3D::EMSurface2DProvider3D()
    :dpssurf1_(*new DataPointSet(true,true))
    ,dpssurf2_(*new DataPointSet(true,true))
{}


EMSurface2DProvider3D::~EMSurface2DProvider3D()
{
    delete &dpssurf1_;
    delete &dpssurf2_;
}


EMSurface2DProvider3D::EMSurface2DProvider3D(
	const EMSurface2DProvider3D& p )
    : dpssurf1_(*new DataPointSet(true,false))
    , dpssurf2_(*new DataPointSet(true,false))
{
    *this = p;
}


EMSurface2DProvider3D& EMSurface2DProvider3D::operator =(
	const EMSurface2DProvider3D& p )
{
    copyFrom(p);
    dpssurf1_ = p.dpssurf1_;
    dpssurf2_ = p.dpssurf2_;
    return *this;
}


void EMSurface2DProvider3D::mkDPS( const EM::Surface& s, DataPointSet& dps )
{
    mDynamicCastGet(const EM::Horizon2D&,surf,s)

    for ( int idx=0; idx<surf.nrSections(); idx++ )
    {
	EM::RowColIterator it( surf, surf.sectionID(idx) );
	while ( true )
	{
	    EM::PosID posid = it.next();
	    if ( posid.objectID() < 0 )
		break;

	    const BinID bid2d = posid.getRowCol();
	    DataPointSet::Pos pos( surf.getPos(posid) );
	    pos.nr_ = bid2d.crl();
	    dps.addRow( DataPointSet::DataRow( pos,
			mCast(unsigned short,bid2d.inl())) );
	}
    }
}


bool EMSurface2DProvider3D::initialize( TaskRunner* taskr )
{
    if ( !EMSurfaceProvider::initialize(taskr) || !surf1_ )
	return false;

    mkDPS( *surf1_, dpssurf1_ );
    if ( surf2_ )
	mkDPS( *surf2_, dpssurf2_ );
    return true;
}


BinID EMSurface2DProvider3D::curBinID() const
{ return !surf1_ ? BinID(0,0) : SI().transform( surf1_->getPos(curpos_) ); }


bool EMSurface2DProvider3D::includes( const BinID& bid, float z ) const
{
    if ( !surf1_ )
	return false;

    const DataPointSet::RowID rid1 = dpssurf1_.findFirst( bid );
    if ( rid1 < 0 )
	return false;
    DataPointSet::RowID rid2 = -1;
    if ( surf2_ )
    {
	rid2 = dpssurf2_.findFirst( bid );
	if ( rid2 < 0 )
	    return false;
    }

    Interval<float> zrg( dpssurf1_.z(rid1), 0 ); zrg.stop = zrg.start;
    if ( rid2 >= 0 )
	zrg.include( dpssurf2_.z(rid2), false );
    zrg += extraz_;
    zrg.widen( SI().zStep() * .5f, false );
    return zrg.includes( z, false );
}


void EMSurface2DProvider3D::getExtent( BinID& start, BinID& stop ) const
{
    start = hs_.start_;
    stop = hs_.stop_;
}


EMImplicitBodyProvider::EMImplicitBodyProvider()
    : tkzs_(true)
    , imparr_(0)
    , threshold_(0)
    , embody_(0)
    , useinside_(true)
    , bbox_(false)
    , initializedbody_(false)
    , curbid_(-1,-1)
    , curz_(mUdf(float))
{}


EMImplicitBodyProvider::EMImplicitBodyProvider(
	const EMImplicitBodyProvider& ep )
    : tkzs_(ep.tkzs_)
    , imparr_(ep.imparr_)
    , threshold_(ep.threshold_)
    , embody_(ep.embody_)
    , useinside_(ep.useinside_)
    , bbox_(ep.bbox_)
    , initializedbody_(ep.initializedbody_)
    , curbid_(ep.curbid_)
    , curz_(ep.curz_)
{}


EMImplicitBodyProvider::~EMImplicitBodyProvider()
{
    delete imparr_;
    if ( embody_ ) embody_->unRefBody();
}


#define mCopyImpArr( sourcearr ) \
    delete imparr_; imparr_ = 0; \
    if ( sourcearr ) \
    { \
	mDeclareAndTryAlloc( Array3DImpl<float>*, newarr, \
		Array3DImpl<float>(sourcearr->info()) ); \
	if ( newarr ) \
	{ \
	    newarr->copyFrom( *sourcearr ); \
	    imparr_ = newarr; \
	} \
    }


EMImplicitBodyProvider& EMImplicitBodyProvider::operator = (
	const EMImplicitBodyProvider& ep )
{
    if ( &ep !=this )
    {
	useinside_ = ep.useinside_;
	embody_ = ep.embody_;
	tkzs_ = ep.tkzs_;
	bbox_ = ep.bbox_;
	threshold_ = ep.threshold_;
	mCopyImpArr( ep.imparr_ );
	initializedbody_ = ep.initializedbody_;
    }

    return *this;
}


void EMImplicitBodyProvider::getTrcKeyZSampling( TrcKeyZSampling& cs ) const
{ cs = useinside_ ? tkzs_ : bbox_; }


bool EMImplicitBodyProvider::initialize( TaskRunner* taskr )
{
    if ( !embody_ )
	return false;

    EM::ImplicitBody* body = embody_->createImplicitBody(taskr,false);
    if ( !body || !body->arr_ )
    {
	delete imparr_; imparr_ = 0;
	return false;
    }

    mCopyImpArr( body->arr_ );
    tkzs_ = body->tkzs_;
    threshold_ = body->threshold_;

    const TrcKeyZSampling& cs = useinside_ ? tkzs_ : bbox_;
    curbid_ = cs.hsamp_.start_;
    curz_ = mUdf(float);

    initializedbody_ = true;
    return imparr_;
}


#define mGetBodyKey(k) IOPar::compKey(sKey::Body(),k)

void EMImplicitBodyProvider::usePar( const IOPar& iop )
{
    MultiID mid;
    if ( !iop.get( mGetBodyKey("ID"), mid ) )
	return;

    EM::EMObject* emobj = EM::EMM().getObject( EM::EMM().getObjectID(mid) );
    if ( !emobj )
	emobj = EM::EMM().loadIfNotFullyLoaded( mid );
    mDynamicCastGet(EM::Body*,emb,emobj);
    if ( !emb )
	return;

    if ( embody_ ) embody_->unRefBody();
    embody_ = emb;
    embody_->refBody();
    embody_->getBodyRange( tkzs_ );

    iop.getYN( sKeyUseInside(), useinside_ );

    Interval<int> inlrg, crlrg;
    Interval<float> zrg;
    iop.get( sKeyBBInlrg(), inlrg );
    iop.get( sKeyBBCrlrg(), crlrg );
    iop.get( sKeyBBZrg(), zrg );
    bbox_.hsamp_.set( inlrg, crlrg );
    bbox_.zsamp_.setFrom( zrg );

    initializedbody_ = false;
}


void EMImplicitBodyProvider::fillPar( IOPar& iop ) const
{
    if ( embody_ )
	iop.set( mGetBodyKey("ID"), embody_->storageID() );
    iop.setYN( sKeyUseInside(), useinside_ );
    if ( !useinside_ )
    {
	iop.set( sKeyBBInlrg(), bbox_.hsamp_.inlRange() );
	iop.set( sKeyBBCrlrg(), bbox_.hsamp_.crlRange() );
	iop.set( sKeyBBZrg(), bbox_.zsamp_ );
    }
}


bool EMImplicitBodyProvider::isOK() const
{
    if ( !initializedbody_ )
    {
	EMImplicitBodyProvider* ep = const_cast<EMImplicitBodyProvider*>(this);
	ep->initialize( 0 );
    }

    return imparr_;
}


void EMImplicitBodyProvider::getSummary( BufferString& txt ) const
{
    if ( !embody_ )
    {
	txt += "Empty geobody";
	return;
    }

    txt += useinside_ ? "Inside of " : "Outside of ";
    txt += embody_->storageName();
    if ( !useinside_ )
    {
	txt += "  Within cube range: Inline( ";
	txt += bbox_.hsamp_.start_.inl(); txt += ", ";
	txt += bbox_.hsamp_.stop_.inl(); txt += " ), Crossline( ";
	txt += bbox_.hsamp_.start_.crl(); txt += ", ";
	txt += bbox_.hsamp_.stop_.crl(); txt += " ), Z( ";
	txt += bbox_.zsamp_.start; txt += ", ";
	txt += bbox_.zsamp_.stop; txt += " ).";
    }
}


void EMImplicitBodyProvider::getExtent( BinID& start, BinID& stop ) const
{
    if ( !isOK() )
    {
	start = stop = BinID(0,0);
	return;
    }

    const TrcKeyZSampling& cs = useinside_ ? tkzs_ : bbox_;
    start = cs.hsamp_.start_;
    stop = cs.hsamp_.stop_;
}


void EMImplicitBodyProvider::getZRange( Interval<float>& zrg ) const
{ zrg = useinside_ ? tkzs_.zsamp_ : bbox_.zsamp_; }


bool EMImplicitBodyProvider::includes( const Coord& c, float z ) const
{ return includes( SI().transform(c), z ); }


bool EMImplicitBodyProvider::includes( const BinID& bid, float z ) const
{
    const TrcKeyZSampling& bb = useinside_ ? tkzs_ : bbox_;
    if ( mIsUdf(z) ) return bb.hsamp_.includes(bid);

    if ( !isOK() || !bb.hsamp_.includes(bid) || !bb.zsamp_.includes(z,false) )
	return false;

    const int inlidx = tkzs_.inlIdx(bid.inl());
    const int crlidx = tkzs_.crlIdx(bid.crl());
    const int zidx = tkzs_.zIdx(z);
    const bool inbody = imparr_->info().validPos(inlidx,crlidx,zidx) &&
	imparr_->get(inlidx,crlidx,zidx) >= threshold_;

    return useinside_==inbody;
}


od_int64 EMImplicitBodyProvider::estNrPos() const
{ return isOK() ? imparr_->info().getTotalSz() : 0; }


int EMImplicitBodyProvider::estNrZPerPos() const
{ return  isOK() ?  imparr_->info().getSize(2) : 0; }


void EMImplicitBodyProvider::initClass()
{ Provider3D::factory().addCreator( create, sKey::Body() ); }


bool EMImplicitBodyProvider::toNextPos()
{
    const TrcKeyZSampling& cs = useinside_ ? tkzs_ : bbox_;

    curbid_.crl() += cs.hsamp_.step_.crl();
    if ( curbid_.crl() > cs.hsamp_.stop_.crl() )
    {
	curbid_.inl() += cs.hsamp_.step_.inl();
	curbid_.crl() = cs.hsamp_.start_.crl();
    }

    curz_ = cs.zsamp_.start;
    return curbid_.inl()<=cs.hsamp_.stop_.inl();
}


bool EMImplicitBodyProvider::toNextZ()
{
    const TrcKeyZSampling& bb = useinside_ ? tkzs_ : bbox_;
    if ( !mIsUdf(curz_) )
	curz_ += bb.zsamp_.step;
    if ( mIsUdf(curz_) || curz_ > bb.zsamp_.stop )
    {
	if ( !toNextPos() )
	    return false;
    }

    while ( !includes(curbid_,curz_) )
    {
	curz_ += bb.zsamp_.step;
	if ( curz_ > bb.zsamp_.stop && !toNextPos() )
	    return false;
    }

    return true;
}

// EMRegion3DProvider
EMRegion3DProvider::EMRegion3DProvider()
    : useinside_(true)
    , bbox_(false)
    , region_(*new EM::Region3D)
{}


EMRegion3DProvider::EMRegion3DProvider(  const EMRegion3DProvider& ep )
    : useinside_(ep.useinside_)
    , bbox_(ep.bbox_)
    , region_(*new EM::Region3D)
{}


EMRegion3DProvider::~EMRegion3DProvider()
{
   delete &region_;
}


bool EMRegion3DProvider::initialize( TaskRunner* )
{
    return true;
}


EMRegion3DProvider& EMRegion3DProvider::operator=( const EMRegion3DProvider& ep)
{
    if ( &ep !=this )
    {
	useinside_ = ep.useinside_;
	bbox_ = ep.bbox_;
    }

    return *this;
}


void EMRegion3DProvider::getTrcKeyZSampling( TrcKeyZSampling& tkzs ) const
{ tkzs = bbox_; }


void EMRegion3DProvider::usePar( const IOPar& iop )
{
}


void EMRegion3DProvider::fillPar( IOPar& iop ) const
{
}


void EMRegion3DProvider::getSummary( BufferString& txt ) const
{
}


bool EMRegion3DProvider::toNextPos()
{ return false; } //TODO


bool EMRegion3DProvider::toNextZ()
{ return false; } //TODO


void EMRegion3DProvider::getExtent( BinID& start, BinID& stop ) const
{
    TrcKeyZSampling tkzs; getTrcKeyZSampling( tkzs );
    start = tkzs.hsamp_.start_;
    stop = tkzs.hsamp_.stop_;
}


void EMRegion3DProvider::getZRange( Interval<float>& zrg ) const
{ zrg = bbox_.zsamp_; }


bool EMRegion3DProvider::includes( const Coord& c, float z ) const
{ return includes( SI().transform(c), z ); }


bool EMRegion3DProvider::includes( const BinID& bid, float z ) const
{
    return region_.isInside( TrcKey(bid), z, false );
}


od_int64 EMRegion3DProvider::estNrPos() const
{ return bbox_.totalNr(); }


int EMRegion3DProvider::estNrZPerPos() const
{ return bbox_.nrZ(); }


void EMRegion3DProvider::initClass()
{ Provider3D::factory().addCreator( create, "Region3D" ); }

} // namespace Pos
