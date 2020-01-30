/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
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
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "posinfo2d.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "uistrings.h"
#include "statrand.h"

namespace Pos
{

const char* EMSurfaceProvider::id1Key()		{ return "ID.1"; }
const char* EMSurfaceProvider::id2Key()		{ return "ID.2"; }
const char* EMSurfaceProvider::zstepKey()	{ return "Z Step"; }
const char* EMSurfaceProvider::extraZKey()	{ return "Extra Z"; }

} // namespace Pos


Pos::EMSurfaceProvider::EMSurfaceProvider()
    : Filter()
    , surf1_(0)
    , surf2_(0)
    , hs_(true)
    , zstep_(SI().zStep())
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


Pos::EMSurfaceProvider::EMSurfaceProvider( const EMSurfaceProvider& oth )
    : Filter(oth)
    , surf1_(0)
    , surf2_(0)
{
    *this = oth;
}


Pos::EMSurfaceProvider::~EMSurfaceProvider()
{
    if ( surf1_ ) surf1_->unRef();
    if ( surf2_ ) surf2_->unRef();
}


Pos::EMSurfaceProvider& Pos::EMSurfaceProvider::operator=(
					const EMSurfaceProvider& oth )
{
    if ( &oth == this )
	return *this;

    Filter::operator = ( oth );

    if ( surf1_ ) surf1_->unRef();
    if ( surf2_ ) surf2_->unRef();
    surf1_ = oth.surf1_; if ( surf1_ ) surf1_->ref();
    surf2_ = oth.surf2_; if ( surf2_ ) surf2_->ref();
    zstep_ = oth.zstep_;
    extraz_ = oth.extraz_;
    id1_ = oth.id1_;
    id2_ = oth.id2_;
    hs_ = oth.hs_;
    zrg1_ = oth.zrg1_; zrg2_ = oth.zrg2_;
    estnrpos_ = -1;
    nrsamples_ = oth.nrsamples_;
    dorandom_ = oth.dorandom_;
    enoughsamples_ = oth.enoughsamples_;
    maxidx_ = oth.maxidx_;

    return *this;
}


const char* Pos::EMSurfaceProvider::type() const
{
    return sKey::Surface();
}


static void getSurfRanges( const EM::Surface& surf, TrcKeySampling& hs,
			   Interval<float>& zrg, od_int64& estnrpos )
{
    bool veryfirst = true;
    EM::RowColIterator it( surf );
    EM::PosID posid = it.next();
    while ( !posid.isInvalid() )
    {
	const Coord3 coord = surf.getPos( posid );
	const BinID bid( SI().transform(coord.getXY()) );
	if ( veryfirst )
	{
	    veryfirst = false;
	    hs.start_ = hs.stop_ = bid;
	    zrg.start = zrg.stop = (float) coord.z_;
	}
	else
	{
	    if ( coord.z_ < zrg.start ) zrg.start = (float) coord.z_;
	    if ( coord.z_ > zrg.stop ) zrg.stop = (float) coord.z_;
	    hs.include( bid );
	}
	estnrpos++;

	posid = it.next();
    }
}


bool Pos::EMSurfaceProvider::initialize( const TaskRunnerProvider& trprov )
{
    if ( nrSurfaces() == 0 )
	return false;

    EM::ObjectManager& emman = EM::getMgr( id1_ );
    EM::Object* emobj = emman.getObject( id1_ );
    if ( !emobj )
	emobj = emman.loadIfNotFullyLoaded( id1_, trprov );
    mDynamicCastGet(EM::Surface*,surf1,emobj)
    if ( !surf1 ) return false;
    surf1_ = surf1; surf1_->ref();

    getSurfRanges( *surf1_, hs_, zrg1_, estnrpos_ );

    if ( id2_.isValid() )
    {
	emobj = emman.getObject( id2_ );
	if ( !emobj )
	    emobj = emman.loadIfNotFullyLoaded( id2_, trprov );
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
	mDynamicCastGet( const Geometry::RowColSurface*, rcs,
			surf1_->geometryElement() );
	if ( rcs )
	    rcs->getPosIDs( posids_ );

	maxidx_ = posids_.size();
	// Need to avoid situation where there aren't enough samples available
	// to avoid an infinite loop when sampling
	// This is a crude lower bound, need to think about using zrg's as well
	enoughsamples_ = nrsamples_ < maxidx_ ;
	estnrpos_ = nrsamples_;
    }

    return true;
}


void Pos::EMSurfaceProvider::reset()
{
    delete iterator_; iterator_ = 0;
    if ( surf1_ )
	iterator_ = new EM::RowColIterator( *surf1_ );
    curpos_ = EM::PosID::getInvalid();
}


bool Pos::EMSurfaceProvider::toNextPos()
{
    if ( dorandom_  && enoughsamples_ )
    {
	postuple pos;
	od_int64 idx;
	const Stats::RandGen& randGen = Stats::randGen();
	TrcKeyZSampling sitkzs;
	SI().getSampling( sitkzs);
	do
	{
	    idx = randGen.getIndex( maxidx_ );
	    curpos_ = posids_[idx];

	    if ( curpos_.isInvalid() )
		return false;

	    curzrg_.start = curzrg_.stop = (float) surf1_->getPos( curpos_ ).z_;
	    if ( surf2_ )
	    {
		const float stop =
		(float) surf2_->getPos( curpos_ ).z_;
		if ( !mIsUdf(stop) )
		    curzrg_.stop = stop;
	    }

	    curzrg_.sort();
	    curzrg_ += extraz_;

	    if ( surf2_ )
	    {
		SI().snapZ( curzrg_.start, OD::SnapUpward );
		SI().snapZ( curzrg_.stop, OD::SnapDownward );
	    }
	    if ( !mIsUdf(curzrg_.start) && !mIsUdf(curzrg_.stop) )
	    {
		int zsamp = randGen.getInt( sitkzs.zIdx( curzrg_.start ),
					    sitkzs.zIdx( curzrg_.stop ) );
		curz_ = sitkzs.zAtIndex( zsamp );
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
	if ( curpos_.isInvalid() )
	    return false;

	curzrg_.start = curzrg_.stop = (float) surf1_->getPos( curpos_ ).z_;
	if ( surf2_ )
	{
	    const float stop =
	    (float) surf2_->getPos( curpos_ ).z_;
	    if ( !mIsUdf(stop) )
		curzrg_.stop = stop;
	}

	curzrg_.sort();
	curzrg_ += extraz_;

	Interval<float> unsnappedzrg = curzrg_;
	if ( surf2_ )
	{
	    SI().snapZ( curzrg_.start, OD::SnapUpward );
	    SI().snapZ( curzrg_.stop, OD::SnapDownward );
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


bool Pos::EMSurfaceProvider::toNextZ()
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


float Pos::EMSurfaceProvider::curZ() const
{ return curz_; }


bool Pos::EMSurfaceProvider::hasZAdjustment() const
{ return surf1_ && !surf2_; }


float Pos::EMSurfaceProvider::adjustedZ( const Coord& c, float z ) const
{
    if ( !hasZAdjustment() ) return z;

    const BinID bid = SI().transform( c );
    return (float) surf1_->getPos( EM::PosID::getFromRowCol(bid) ).z_;
}


#define mGetSurfKey(k) IOPar::compKey(sKey::Surface(),k)


void Pos::EMSurfaceProvider::usePar( const IOPar& iop )
{
    dorandom_ = false;
    iop.getYN( sKey::Random(), dorandom_ );
    if ( dorandom_ )
	iop.get( sKey::NrValues(), nrsamples_);

    iop.get( mGetSurfKey(id1Key()), id1_ );
    iop.get( mGetSurfKey(id2Key()), id2_ );
    iop.get( mGetSurfKey(zstepKey()), zstep_ );
    iop.get( mGetSurfKey(extraZKey()), extraz_ );

    if ( !id1_.isValid() )
	return;
    EM::SurfaceIOData sd;
    EM::IOObjInfo eminfo( id1_ );
    if ( !eminfo.isOK() )
	return;

    TrcKeySampling hs;
    hs.set( eminfo.getInlRange(), eminfo.getCrlRange() );
    hs_ = hs;

    if ( !id2_.isValid() )
	return;
    eminfo = EM::IOObjInfo( id2_ );
    if ( !eminfo.isOK() )
	return;
    hs.set( eminfo.getInlRange(), eminfo.getCrlRange() );
    hs_.limitTo( hs );
    // TODO: get zrg's
}


void Pos::EMSurfaceProvider::fillPar( IOPar& iop ) const
{
    iop.set( mGetSurfKey(id1Key()), id1_ );
    iop.set( mGetSurfKey(id2Key()), id2_ );
    iop.set( mGetSurfKey(zstepKey()), zstep_ );
    iop.set( mGetSurfKey(extraZKey()), extraz_ );
    iop.setYN( sKey::Random(), dorandom_ );
    if ( dorandom_ )
	iop.set( sKey::NrValues(), nrsamples_ );
}


void Pos::EMSurfaceProvider::getSummary( uiString& txt ) const
{
    switch ( nrSurfaces() )
    {
	case 0:
	return;
	case 1:
	    txt.appendPhrase(tr("On '%1'","xyz lies On abc")
	       .arg( id1_.name() ),
	       uiString::Space, uiString::OnSameLine );
	break;
	default:
	    txt.appendPhrase(tr("Between '%1' and '%2'")
		.arg(id1_.name()).arg(id2_.name()),
		uiString::Space, uiString::OnSameLine );
	break;
    }
}


bool Pos::EMSurfaceProvider::getZRange( const TrcKey& tk,
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


void Pos::EMSurfaceProvider::getZRange( Interval<float>& zrg ) const
{
    if ( !surf1_ ) return;

    zrg = zrg1_;

    if ( surf2_ )
    {
	if ( zrg2_.start < zrg.start ) zrg.start = zrg2_.start;
	if ( zrg2_.stop > zrg.stop ) zrg.stop = zrg2_.stop;
    }
}


int Pos::EMSurfaceProvider::estNrZPerPos() const
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


int Pos::EMSurfaceProvider::nrSurfaces() const
{
    if ( !surf1_ )
	return id1_.isInvalid() ? 0 : (id2_.isInvalid() ? 1 : 2);
    return surf2_ ? 2 : 1;
}



Pos::EMSurfaceProvider3D::EMSurfaceProvider3D()
    : Provider3D()
    , EMSurfaceProvider()
{}


Pos::EMSurfaceProvider3D::EMSurfaceProvider3D( const EMSurfaceProvider3D& oth )
    : Provider3D( oth )
    , EMSurfaceProvider( oth )
{
    *this = oth;
}


Pos::EMSurfaceProvider3D& Pos::EMSurfaceProvider3D::operator=(
						const EMSurfaceProvider3D& oth )
{
    if ( &oth == this )
	return *this;

    Provider3D::operator = ( oth );
    EMSurfaceProvider::operator = ( oth );

    return *this;
}


BinID Pos::EMSurfaceProvider3D::curBinID() const
{
    return curpos_.getBinID();
}


bool Pos::EMSurfaceProvider3D::includes( const BinID& bid, float z ) const
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


void Pos::EMSurfaceProvider3D::getExtent( BinID& start, BinID& stop ) const
{ start = hs_.start_; stop = hs_.stop_; }


void Pos::EMSurfaceProvider3D::initClass()
{ Provider3D::factory().addCreator( create, sKey::Surface(),
				    uiStrings::sHorizon() ); }


// ***** EMSurfaceProvider2D ****
Pos::EMSurfaceProvider2D::EMSurfaceProvider2D()
    : Provider2D()
    , EMSurfaceProvider()
{}


Pos::EMSurfaceProvider2D::EMSurfaceProvider2D( const EMSurfaceProvider2D& oth )
    : Provider2D( oth )
    , EMSurfaceProvider( oth )
{
    *this = oth;
}


Pos::EMSurfaceProvider2D& Pos::EMSurfaceProvider2D::operator=(
						const EMSurfaceProvider2D& oth )
{
    if ( &oth == this )
	return *this;

    Provider2D::operator = ( oth );
    EMSurfaceProvider::operator = ( oth );

    return *this;
}


const char* Pos::EMSurfaceProvider2D::curLine() const
{
    BinID bid = curpos_.getBinID();
    if ( surf1_ )
    {
	mDynamicCastGet(EM::Horizon2D*,hor2d,surf1_);
	if ( !hor2d )
	    return 0;

	return hor2d->geometry().lineName( bid.inl() );
    }

    return 0;
}


Bin2D Pos::EMSurfaceProvider2D::curBin2D() const
{
    return Bin2D::decode( curpos_.getBinID() );
}


Coord Pos::EMSurfaceProvider2D::curCoord() const
{ return surf1_ ? surf1_->getPos( curpos_ ).getXY() : Coord(0,0); }


TrcKey Pos::EMSurfaceProvider2D::curTrcKey() const
{
    mDynamicCastGet(EM::Horizon2D*,hor2d,surf1_);
    if ( !hor2d )
	return TrcKey::udf();

    return hor2d->geometry().getTrcKey( curpos_ );
}


bool Pos::EMSurfaceProvider2D::includes( const Coord& c, float z ) const
{
    PosInfo::Line2DPos pos;
    for ( int lidx=0; lidx<nrLines(); lidx++ )
    {
	const auto& geom2d = SurvGeom2D::get( geomID(lidx) );
	if ( geom2d.isEmpty() )
	    continue;

	const PosInfo::Line2DData& l2d = geom2d.data();
	if ( l2d.getPos(c,pos,SI().inlDistance()))
	{
	    if ( includes(pos.nr_,z,lidx) )
		return true;
	}
    }

    return false;
}


bool Pos::EMSurfaceProvider2D::includes( int trcnr, float z, int lidx ) const
{
    Interval<float> zrg;
    const bool res =
	EMSurfaceProvider::getZRange( TrcKey(geomID(lidx),trcnr), zrg );
    if ( !res || zrg.isUdf() )
	return false;

    return mIsUdf(z) ? true : zrg.includes( z, false );
}


void Pos::EMSurfaceProvider2D::getExtent( Interval<int>& intv, int lidx ) const
{ intv.start = intv.stop = 0; }


void Pos::EMSurfaceProvider2D::initClass()
{ Provider2D::factory().addCreator( create, sKey::Surface(),
						    uiStrings::sSurface() ); }


// ***** EMSurface2DProvider3D ****
Pos::EMSurface2DProvider3D::EMSurface2DProvider3D()
    : Provider3D()
    , EMSurfaceProvider()
    , dpssurf1_(*new DataPointSet(true,true))
    , dpssurf2_(*new DataPointSet(true,true))
{}


Pos::EMSurface2DProvider3D::EMSurface2DProvider3D(
				const EMSurface2DProvider3D& p )
    : Provider3D()
    , EMSurfaceProvider()
    , dpssurf1_(*new DataPointSet(true,false))
    , dpssurf2_(*new DataPointSet(true,false))
{
    *this = p;
}


Pos::EMSurface2DProvider3D::~EMSurface2DProvider3D()
{
    dpssurf1_.unRef();
    dpssurf2_.unRef();
}



Pos::EMSurface2DProvider3D& Pos::EMSurface2DProvider3D::operator =(
					const EMSurface2DProvider3D& oth )
{
    if ( &oth == this )
	return *this;

    Provider3D::operator = ( oth );
    EMSurfaceProvider::operator = ( oth );

    dpssurf1_ = oth.dpssurf1_;
    dpssurf2_ = oth.dpssurf2_;

    return *this;
}


void Pos::EMSurface2DProvider3D::mkDPS( const EM::Surface& s,
					DataPointSet& dps )
{
    mDynamicCastGet(const EM::Horizon2D&,surf,s)

    EM::RowColIterator it( surf );
    while ( true )
    {
	EM::PosID posid = it.next();
	if ( posid.isInvalid() )
	    break;

	const BinID bid2d = BinID( posid.getRowCol() );
	DataPointSet::Pos pos( surf.getPos(posid) );
	pos.set( Bin2D::decode(bid2d) );
	dps.addRow( DataPointSet::DataRow(pos) );
    }
}


void Pos::EMSurface2DProvider3D::reset()
{
    SilentTaskRunnerProvider trprov;
    initialize( trprov );
}


bool Pos::EMSurface2DProvider3D::initialize( const TaskRunnerProvider& trprov )
{
    if ( !EMSurfaceProvider::initialize(trprov) || !surf1_ )
	return false;

    mkDPS( *surf1_, dpssurf1_ );
    if ( surf2_ )
	mkDPS( *surf2_, dpssurf2_ );
    return true;
}


BinID Pos::EMSurface2DProvider3D::curBinID() const
{
    return !surf1_
	? BinID(0,0)
	: SI().transform( surf1_->getPos(curpos_).getXY() );
}


bool Pos::EMSurface2DProvider3D::includes( const BinID& bid, float z ) const
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


void Pos::EMSurface2DProvider3D::getExtent( BinID& start, BinID& stop ) const
{
    start = hs_.start_;
    stop = hs_.stop_;
}


Pos::EMImplicitBodyProvider::EMImplicitBodyProvider()
    : Provider3D()
    , tkzs_(true)
    , imparr_(0)
    , threshold_(0)
    , embody_(0)
    , useinside_(true)
    , bbox_(false)
    , initializedbody_(false)
    , curbid_(-1,-1)
    , curz_(mUdf(float))
{}


Pos::EMImplicitBodyProvider::EMImplicitBodyProvider(
					const EMImplicitBodyProvider& oth )
    : Provider3D()
{
    *this = oth;
}


Pos::EMImplicitBodyProvider::~EMImplicitBodyProvider()
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


Pos::EMImplicitBodyProvider& Pos::EMImplicitBodyProvider::operator = (
				    const EMImplicitBodyProvider& oth )
{
    if ( &oth == this )
	return *this;

    Provider3D::operator = ( oth );

    tkzs_ = oth.tkzs_;
    mCopyImpArr( oth.imparr_ );
    threshold_ = oth.threshold_;
    useinside_ = oth.useinside_;
    bbox_ = oth.bbox_;
    embody_ = oth.embody_;
    curbid_ = oth.curbid_;
    curz_ = oth.curz_;
    initializedbody_ = oth.initializedbody_;

    return *this;
}


void Pos::EMImplicitBodyProvider::getTrcKeyZSampling(
					TrcKeyZSampling& cs ) const
{
    cs = useinside_ ? tkzs_ : bbox_;
}


void Pos::EMImplicitBodyProvider::reset()
{
    SilentTaskRunnerProvider trprov;
    initialize( trprov );
}


bool Pos::EMImplicitBodyProvider::initialize( const TaskRunnerProvider& trprov )
{
    if ( !embody_ )
	return false;

    EM::ImplicitBody* body = embody_->createImplicitBody(trprov,false);
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

void Pos::EMImplicitBodyProvider::usePar( const IOPar& iop )
{
    DBKey dbky;
    if ( !iop.get( mGetBodyKey("ID"), dbky ) )
	return;

    EM::ObjectManager& emman = EM::getMgr( dbky );
    EM::Object* emobj = emman.getObject( dbky );
    if ( !emobj )
    {
	SilentTaskRunnerProvider trprov;
	emobj = emman.loadIfNotFullyLoaded( dbky, trprov );
    }

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


void Pos::EMImplicitBodyProvider::fillPar( IOPar& iop ) const
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


bool Pos::EMImplicitBodyProvider::isOK() const
{
    if ( !initializedbody_ )
    {
	EMImplicitBodyProvider* ep = const_cast<EMImplicitBodyProvider*>(this);
	SilentTaskRunnerProvider trprov;
	ep->initialize( trprov );
    }

    return imparr_;
}


void Pos::EMImplicitBodyProvider::getSummary( uiString& txt ) const
{
    if ( !embody_ )
    {
	txt.appendPhrase( tr("Empty body"), uiString::Space,
						    uiString::OnSameLine );
	return;
    }
    uiString str = useinside_ ? tr("Inside of %1") : tr("Outside of %1");
    txt.appendPhrase( str.arg(toUiString(embody_->storageName())),
				uiString::Space, uiString::OnSameLine  );
    if ( !useinside_ )
    {
	txt.appendPhrase(tr("Bounding cube range: Inline( %1, %2 ), "
	    "Crossline( %3, %4 ), Z( %5, %6 ).").arg(bbox_.hsamp_.start_.inl())
	    .arg(bbox_.hsamp_.stop_.inl()).arg(bbox_.hsamp_.start_.crl())
	    .arg(bbox_.hsamp_.stop_.crl()).arg(bbox_.zsamp_.start)
	    .arg(bbox_.zsamp_.stop), uiString::NoSep );
    }
}


void Pos::EMImplicitBodyProvider::getExtent( BinID& start, BinID& stop ) const
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


void Pos::EMImplicitBodyProvider::getZRange( Interval<float>& zrg ) const
{ zrg = useinside_ ? tkzs_.zsamp_ : bbox_.zsamp_; }


bool Pos::EMImplicitBodyProvider::includes( const Coord& c, float z ) const
{ return includes( SI().transform(c), z ); }


bool Pos::EMImplicitBodyProvider::includes( const BinID& bid, float z ) const
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


od_int64 Pos::EMImplicitBodyProvider::estNrPos() const
{ return isOK() ? imparr_->totalSize() : 0; }


int Pos::EMImplicitBodyProvider::estNrZPerPos() const
{ return  isOK() ? imparr_->getSize(2) : 0; }


void Pos::EMImplicitBodyProvider::initClass()
{ Provider3D::factory().addCreator( create, sKey::Body(),
						    uiStrings::sBody() ); }


bool Pos::EMImplicitBodyProvider::toNextPos()
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


bool Pos::EMImplicitBodyProvider::toNextZ()
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
Pos::EMRegion3DProvider::EMRegion3DProvider()
    : Provider3D()
    , useinside_(true)
    , bbox_(false)
    , region_(*new EM::Region3D)
{}


Pos::EMRegion3DProvider::EMRegion3DProvider(  const EMRegion3DProvider& oth )
    : Provider3D(oth)
    , region_(*new EM::Region3D)
{
    *this = oth;
}


Pos::EMRegion3DProvider::~EMRegion3DProvider()
{
   delete &region_;
}


void Pos::EMRegion3DProvider::reset()
{
}


bool Pos::EMRegion3DProvider::initialize( const TaskRunnerProvider& )
{
    return true;
}


Pos::EMRegion3DProvider& Pos::EMRegion3DProvider::operator=(
					const EMRegion3DProvider& ep)
{
    if ( &ep == this )
	return *this;

    Provider3D::operator = ( ep );

    bbox_ = ep.bbox_;
    region_ = ep.region_;
    curbid_ = ep.curbid_;
    curz_ = ep.curz_;
    useinside_ = ep.useinside_;

    return *this;
}


void Pos::EMRegion3DProvider::getTrcKeyZSampling( TrcKeyZSampling& tkzs ) const
{ tkzs = bbox_; }


void Pos::EMRegion3DProvider::usePar( const IOPar& iop )
{
}


void Pos::EMRegion3DProvider::fillPar( IOPar& iop ) const
{
}


void Pos::EMRegion3DProvider::getSummary( uiString& txt ) const
{
}


bool Pos::EMRegion3DProvider::toNextPos()
{ return false; } //TODO


bool Pos::EMRegion3DProvider::toNextZ()
{ return false; } //TODO


void Pos::EMRegion3DProvider::getExtent( BinID& start, BinID& stop ) const
{
    TrcKeyZSampling tkzs; getTrcKeyZSampling( tkzs );
    start = tkzs.hsamp_.start_;
    stop = tkzs.hsamp_.stop_;
}


void Pos::EMRegion3DProvider::getZRange( Interval<float>& zrg ) const
{ zrg = bbox_.zsamp_; }


bool Pos::EMRegion3DProvider::includes( const Coord& c, float z ) const
{ return includes( SI().transform(c), z ); }


bool Pos::EMRegion3DProvider::includes( const BinID& bid, float z ) const
{
    return region_.isInside( TrcKey(bid), z, false );
}


od_int64 Pos::EMRegion3DProvider::estNrPos() const
{
    return bbox_.totalNr();
}


int Pos::EMRegion3DProvider::estNrZPerPos() const
{
    return bbox_.nrZ();
}


void Pos::EMRegion3DProvider::initClass()
{ Provider3D::factory().addCreator( create, "Region3D", tr("3D Region") ); }

// namespace Pos
