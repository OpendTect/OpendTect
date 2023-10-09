/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "rangeposprovider.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "posinfo2d.h"
#include "iopar.h"
#include "trckeyzsampling.h"
#include "keystrs.h"
#include "statrand.h"
#include "randcolor.h"

namespace OD
{

Stats::RandGen& ColorRandGen()
{
    mDefineStaticLocalObject( PtrMan<Stats::RandGen>, rgptr,
			      = new Stats::RandGen() );
    return *rgptr.ptr();
}


OD::Color getRandomColor( bool withtransp )
{
    Stats::RandGen& gen = ColorRandGen();
    return OD::Color(
	     (unsigned char) gen.getIndex(255),
	     (unsigned char) gen.getIndex(255),
	     (unsigned char) gen.getIndex(255),
	     (unsigned char) (withtransp ? gen.getIndex(255) : 0) );
}


OD::Color getRandStdDrawColor()
{
    static int curidx = -1;
    if ( curidx == -1 )
	curidx = ColorRandGen().getIndex( OD::Color::nrStdDrawColors() );
    else
    {
	curidx++;
	if ( curidx == OD::Color::nrStdDrawColors() )
	    curidx = 0;
    }

    return OD::Color::stdDrawColor( curidx );
}


OD::Color getRandomFillColor()
{
    Stats::RandGen& gen = ColorRandGen();
    return OD::Color(
	    (unsigned char) gen.getInt(155,255),
	    (unsigned char) gen.getInt(155,255),
	    (unsigned char) gen.getInt(155,255) );
}

} // namespace OD


#define mGet2DGeometry(gid) \
    const Survey::Geometry* geometry = Survey::GM().getGeometry( gid ); \
    const Survey::Geometry2D* geom2d = geometry ? geometry->as2D() : 0


Pos::RangeProvider3D::RangeProvider3D()
    : tkzs_(*new TrcKeyZSampling(true))
    , nrsamples_(mUdf(int))
    , gen_(*new Stats::RandGen())
{
    reset();
}


Pos::RangeProvider3D::RangeProvider3D( const Pos::RangeProvider3D& oth )
    : tkzs_(*new TrcKeyZSampling(false))
    , gen_(*new Stats::RandGen())
{
    *this = oth;
}


Pos::RangeProvider3D::~RangeProvider3D()
{
    delete &tkzs_;
    delete &gen_;
}


Pos::RangeProvider3D& Pos::RangeProvider3D::operator =(
					const Pos::RangeProvider3D& p )
{
    if ( &p != this )
    {
	tkzs_ = p.tkzs_;
	curbid_ = p.curbid_;
	curzidx_ = p.curzidx_;
	zsampsz_ = p.zsampsz_;
	nrsamples_ = p.nrsamples_;
	dorandom_ = p.dorandom_;
	enoughsamples_ = p.enoughsamples_;
	posindexlst_.setEmpty();
    }
    return *this;
}


void Pos::RangeProvider3D::setSampling( const TrcKeyZSampling& tkzs )
{
    tkzs_ = tkzs;
    if ( dorandom_ )
    {
	// For random sampling use the survey steps/sampling
	const TrcKeyZSampling& sitkzs = SI().sampling( false );
	tkzs_.hsamp_.step_ = sitkzs.hsamp_.step_;
	tkzs_.zsamp_.step = sitkzs.zsamp_.step;
    }
    zsampsz_ = tkzs.zsamp_.nrSteps()+1;
}


const char* Pos::RangeProvider3D::type() const
{
    return sKey::Range();
}


bool Pos::RangeProvider3D::initialize( TaskRunner* )
{
    if ( dorandom_ )
    {
	// For random sampling use the survey steps/sampling
	const TrcKeyZSampling& sitkzs = SI().sampling( false );
	tkzs_.hsamp_.step_ = sitkzs.hsamp_.step_;
	tkzs_.zsamp_.step = sitkzs.zsamp_.step;
	// Check the number of samples doesn't exceed the number available
	enoughsamples_ = nrsamples_<tkzs_.totalNr();

    }
    reset();
    return true;
}


void Pos::RangeProvider3D::reset()
{
    curbid_ = BinID( tkzs_.hsamp_.start_.inl(),
		     tkzs_.hsamp_.start_.crl()-tkzs_.hsamp_.step_.crl() );
    curzidx_ = tkzs_.zsamp_.nrSteps();
    zsampsz_ = tkzs_.zsamp_.nrSteps()+1;
}


bool Pos::RangeProvider3D::toNextPos()
{
    if ( dorandom_ && enoughsamples_ )
    {
	postuple pos;
	od_int64 idx;
	const TrcKeySampling& hsamp = tkzs_.hsamp_;
	const od_int64 totalNrTraces = hsamp.totalNr();
	do
	{
	    idx = gen_.getIndex( totalNrTraces );
	    curzidx_ = gen_.getIndex( zsampsz_-1 );
	    pos = postuple( idx, curzidx_ );
	} while ( posindexlst_.isPresent(pos) );
	curbid_ = hsamp.atIndex( idx );
	posindexlst_ += pos;
	if ( posindexlst_.size() > nrsamples_ )
	    return false;
    }
    else
    {
	curbid_.crl() += tkzs_.hsamp_.step_.crl();
	if ( curbid_.crl() > tkzs_.hsamp_.stop_.crl() )
	{
	    curbid_.inl() += tkzs_.hsamp_.step_.inl();
	    if ( curbid_.inl() > tkzs_.hsamp_.stop_.inl() )
		return false;
	    curbid_.crl() = tkzs_.hsamp_.start_.crl();
	}
	curzidx_ = 0;
    }

    return true;
}


#define mZrgEps (1e-6*tkzs_.zsamp_.step)

bool Pos::RangeProvider3D::toNextZ()
{
    if ( dorandom_ && enoughsamples_ )
	return toNextPos();
    else
    {
	curzidx_++;
	if ( curzidx_>=zsampsz_ )
	    return toNextPos();
    }

    return true;
}


float Pos::RangeProvider3D::curZ() const
{
    if ( curzidx_<0 || curzidx_>=zsampsz_ )
	return mUdf(float);

    return tkzs_.zsamp_.atIndex( curzidx_ );
}


bool Pos::RangeProvider3D::includes( const BinID& bid, float z ) const
{
    bool issel = tkzs_.hsamp_.includes(bid);
    if ( !issel ) return false;
    if ( mIsUdf(z) ) return true;

    return z < tkzs_.zsamp_.stop+mZrgEps && z > tkzs_.zsamp_.start - mZrgEps;
}


void Pos::RangeProvider3D::usePar( const IOPar& iop )
{
    dorandom_ = false;
    iop.getYN( sKey::Random(), dorandom_ );
    tkzs_.usePar( iop );
    if ( dorandom_ )
	iop.get( sKey::NrValues(), nrsamples_ );

    zsampsz_ = tkzs_.zsamp_.nrSteps()+1;
}


void Pos::RangeProvider3D::fillPar( IOPar& iop ) const
{
    tkzs_.fillPar( iop );
    iop.setYN( sKey::Random(), dorandom_ );
    if ( dorandom_ )
	iop.set( sKey::NrValues(), nrsamples_ );
}


void Pos::RangeProvider3D::getSummary( BufferString& txt ) const
{
    txt.set( tkzs_.hsamp_.start_.toString() ).add( "-" );
    txt.add( tkzs_.hsamp_.stop_.toString() ); // needs to be a separate line
    const int nrsamps = zsampsz_;
    if ( nrsamps > 1 )
	txt.add( " (" ).add( nrsamps ).add( " samples)" );
}


void Pos::RangeProvider3D::getExtent( BinID& start, BinID& stop ) const
{
    start = tkzs_.hsamp_.start_; stop = tkzs_.hsamp_.stop_;
}


void Pos::RangeProvider3D::getZRange( Interval<float>& zrg ) const
{
    assign( zrg, tkzs_.zsamp_ );
    mDynamicCastGet(StepInterval<float>*,szrg,&zrg)
    if ( szrg )
	szrg->step = tkzs_.zsamp_.step;
}


od_int64 Pos::RangeProvider3D::estNrPos() const
{
    if ( dorandom_ && enoughsamples_ )
	return nrsamples_;

    return tkzs_.hsamp_.totalNr();
}


int Pos::RangeProvider3D::estNrZPerPos() const
{
    if ( dorandom_ && enoughsamples_ )
	return 1;

    return zsampsz_;
}


void Pos::RangeProvider3D::getTrcKeyZSampling( TrcKeyZSampling& tkzs ) const
{
    tkzs = tkzs_;
}


void Pos::RangeProvider3D::initClass()
{
    Pos::Provider3D::factory().addCreator( create, sKey::Range() );
}


Pos::RangeProvider2D::RangeProvider2D()
    : curgeom_(0)
    , curtrcrg_(StepInterval<int>::udf())
    , curzrg_(StepInterval<float>::udf())
{
    zrgs_ += SI().zRange( false );
    trcrgs_ += StepInterval<int>(1,mUdf(int),1);
    reset();
}


Pos::RangeProvider2D::RangeProvider2D( const Pos::RangeProvider2D& p )
    : curgeom_(0)
    , curlinezsampsz_(0)
    , curtrcrg_(StepInterval<int>::udf())
    , curzrg_(StepInterval<float>::udf())
{
    *this = p;
}


Pos::RangeProvider2D::~RangeProvider2D()
{
    unRefAndNullPtr( curgeom_ );
}


Pos::RangeProvider2D& Pos::RangeProvider2D::operator =(
					const Pos::RangeProvider2D& p )
{
    if ( &p != this )
    {
	zrgs_ = p.zrgs_;
	trcrgs_ = p.trcrgs_;
	curtrcidx_ = p.curtrcidx_;
	curlineidx_ =  p.curlineidx_;
	unRefAndNullPtr( curgeom_ );
	curzidx_ = p.curzidx_;
	curlinezsampsz_ = p.curlinezsampsz_;
	for ( int idx=0; idx<p.nrLines(); idx++ )
	    addGeomID( p.geomID(idx) );
	getCurRanges();
    }

    return *this;
}


const char* Pos::RangeProvider2D::type() const
{
    return sKey::Range();
}


void Pos::RangeProvider2D::reset()
{
    curlineidx_ = 0;
    unRefAndNullPtr( curgeom_ );
    StepInterval<float> zrg = zrgs_[0];
    if ( !geomids_.isEmpty() )
    {
	mGet2DGeometry(geomids_[0]);
	if ( geom2d )
	{
	    curgeom_ = geom2d;
	    curgeom_->ref();
	    getCurRanges();
	}
    }

    curtrcidx_ = -1;
    curzidx_ = zrg.nrSteps();
    curlinezsampsz_ = zrg.nrSteps()+1;
}


void Pos::RangeProvider2D::setTrcRange( const StepInterval<int>& trcrg,
					int lidx )
{
    if ( !trcrgs_.validIdx(lidx) )
	return;

    trcrgs_[lidx] = trcrg;
    if ( !geomids_.validIdx(lidx) )
	return;

    mGet2DGeometry(geomids_[lidx]);
    if ( !geom2d )
	return;

    trcrgs_[lidx].limitTo( geom2d->data().trcNrRange() );
}


void Pos::RangeProvider2D::setZRange( const StepInterval<float>& zrg, int lidx )
{
    if ( !zrgs_.validIdx(lidx) )
	return;

    zrgs_[lidx] = zrg;
    if ( !geomids_.validIdx(lidx) )
	return;

    mGet2DGeometry(geomids_[lidx]);
    if ( !geom2d )
	return;

    zrgs_[lidx].limitTo( geom2d->data().zRange() );
}


TrcKey Pos::RangeProvider2D::curTrcKey() const
{
    const Pos::GeomID gid = curGeomID();
    if ( !gid.isValid() )
	return TrcKey::udf();

    return TrcKey( gid, curNr() );
}


Pos::GeomID Pos::RangeProvider2D::curGeomID() const
{
    if ( geomids_.validIdx(curlineidx_) )
	return geomids_[curlineidx_];

    return Pos::GeomID::udf();
}


void Pos::RangeProvider2D::getCurRanges() const
{
    curtrcrg_ = trcrgs_.validIdx(curlineidx_) ? trcrgs_[curlineidx_]
					      : trcrgs_[0];
    const StepInterval<int> geomtrcrg = curgeom_->data().trcNrRange();
    curtrcrg_.limitTo( geomtrcrg );
    const int firstvalidgeomidx =
	geomtrcrg.indexOnOrAfter( curtrcrg_.start, mDefEps );
    curtrcrg_.start = geomtrcrg.atIndex( firstvalidgeomidx );
    curzrg_ = zrgs_.validIdx(curlineidx_) ? zrgs_[curlineidx_]
					  : zrgs_[0];
    curzrg_.limitTo( curgeom_->data().zRange() );
    curlinezsampsz_ = curzrg_.nrSteps()+1;
}


const Survey::Geometry2D* Pos::RangeProvider2D::curGeom() const
{
    if ( !curgeom_ )
    {
	mGet2DGeometry( curGeomID() );
	if ( geom2d )
	{
	    const_cast<Pos::RangeProvider2D*>(this)->curgeom_ = geom2d;
	    curgeom_->ref();
	    getCurRanges();
	}
    }

    return curgeom_;
}


bool Pos::RangeProvider2D::toNextPos()
{
    while ( true )
    {
	StepInterval<int> trcrg = curTrcRange();
	if ( trcrg.isUdf() )
	    return false;

	curtrcidx_++;
	if ( curtrcidx_ <= trcrg.nrSteps() )
	    break;

	curlineidx_++;
	unRefAndNullPtr( curgeom_ );
	if ( !geomids_.validIdx(curlineidx_) )
	    return false;

	curtrcidx_ = -1;
    }

    curzidx_ = 0;
    return true;
}


#undef mZrgEps
#define mZrgEps (1e-6*zrg.step)

bool Pos::RangeProvider2D::toNextZ()
{
    curzidx_++;
    if ( curzidx_ >= curlinezsampsz_ )
	return toNextPos();

    return true;
}


int Pos::RangeProvider2D::curNr() const
{
    StepInterval<int> trcrg = curTrcRange();
    return trcrg.atIndex( curtrcidx_ );
}


float Pos::RangeProvider2D::curZ() const
{
    const StepInterval<float> zrg = curZRange();
    if ( curzidx_<0 || curzidx_>=curlinezsampsz_ )
	return mUdf(float);

    return zrg.atIndex( curzidx_ );
}


Coord Pos::RangeProvider2D::curCoord() const
{
    ConstRefMan<Survey::Geometry2D> curgeom = curGeom();
    if ( curgeom )
    {
	PosInfo::Line2DPos l2dpos;
	if ( curgeom->data().getPos(curNr(),l2dpos) )
	    return l2dpos.coord_;
    }

    return Coord::udf();
}


StepInterval<int> Pos::RangeProvider2D::curTrcRange() const
{
    if ( !curGeom() )
	return StepInterval<int>::udf();

    return curtrcrg_;
}


StepInterval<float> Pos::RangeProvider2D::curZRange() const
{
    if ( !curGeom() )
	return StepInterval<float>::udf();

    return curzrg_;
}


bool Pos::RangeProvider2D::includes( int nr, float z, int lidx ) const
{
    if ( lidx == curlineidx_ || !geomids_.validIdx(lidx) )
	return curTrcRange().includes(nr,false)
	    && curZRange().includes(z,false);

    mGet2DGeometry(geomids_[lidx]);
    if ( !geom2d )
	return false;

    StepInterval<int> trcrg = trcrgs_.validIdx(lidx) ? trcrgs_[lidx]
						     : trcrgs_[0];
    trcrg.limitTo( geom2d->data().trcNrRange() );
    StepInterval<float> zrg = zrgs_.validIdx(lidx) ? zrgs_[lidx] : zrgs_[0];
    zrg.limitTo( geom2d->data().zRange() );
    return trcrg.includes(nr,false) && zrg.includes(z,false);
}


bool Pos::RangeProvider2D::includes( const Coord& c, float z ) const
{
    ConstRefMan<Survey::Geometry2D> curgeom = curGeom();

    if ( curgeom )
    {
	const TypeSet<PosInfo::Line2DPos>& pos = curgeom->data().positions();
	for ( int idx=0; idx<pos.size(); idx++ )
	    if ( pos[idx].coord_ == c )
		return curTrcRange().includes(pos[idx].nr_,false)
			&& curZRange().includes(z,false);
    }

    for ( int lidx=0; lidx<nrLines(); lidx++ )
    {
	mGet2DGeometry(geomids_[lidx]);
	if ( !geom2d )
	    continue;

	const TypeSet<PosInfo::Line2DPos>& pos = geom2d->data().positions();
	for ( int idx=0; idx<pos.size(); idx++ )
	{
	    if ( pos[idx].coord_ == c )
	    {
		StepInterval<int> trcrg = trcrgs_.validIdx(lidx) ? trcrgs_[lidx]
								 : trcrgs_[0];
		StepInterval<float> zrg = zrgs_.validIdx(lidx) ? zrgs_[lidx]
							       : zrgs_[0];
		trcrg.limitTo( geom2d->data().trcNrRange() );
		zrg.limitTo( geom2d->data().zRange() );
		return trcrg.includes(pos[idx].nr_,false)
			&& zrg.includes(z,false);
	    }
	}
    }

    return false;
}


void Pos::RangeProvider2D::getExtent( Interval<int>& rg, int lidx ) const
{
    if ( lidx == curlineidx_ || !geomids_.validIdx(lidx) )
    {
	rg = curTrcRange();
	if ( !rg.isUdf() )
	    return;
    }

    rg = trcrgs_.validIdx(lidx) ? trcrgs_[lidx] : trcrgs_[0];
    if ( !geomids_.validIdx(lidx) )
	return;

    mGet2DGeometry(geomids_[lidx]);
    if ( !geom2d )
	return;

    rg.limitTo( geom2d->data().trcNrRange() );
}


void Pos::RangeProvider2D::getZRange( Interval<float>& zrg, int lidx ) const
{
    if ( lidx == curlineidx_ || !geomids_.validIdx(lidx) )
    {
	zrg = curZRange();
	if ( !zrg.isUdf() )
	    return;
    }

    zrg = zrgs_.validIdx(lidx) ? zrgs_[lidx] : zrgs_[0];
    if ( !geomids_.validIdx(lidx) )
	return;

    mGet2DGeometry(geomids_[lidx]);
    if ( !geom2d )
	return;

    zrg.limitTo( geom2d->data().zRange() );
}


void Pos::RangeProvider2D::usePar( const IOPar& iop )
{
    PtrMan<IOPar> subpartrcrg = iop.subselect( sKey::TrcRange() );
    if ( !subpartrcrg )
    {
	TrcKeyZSampling cs(false); cs.set2DDef();
	if ( cs.usePar(iop) )
	{
	    trcrgs_[0] = cs.hsamp_.crlRange();
	    zrgs_[0] = cs.zsamp_;
	}

	return;
    }

    int idx = 0;
    trcrgs_.erase();
    StepInterval<int> trcrg;
    while ( subpartrcrg->get(toString(idx++),trcrg) )
	trcrgs_ += trcrg;

    if ( trcrgs_.isEmpty() )
	trcrgs_ += StepInterval<int>( 1, mUdf(int), 1 );

    PtrMan<IOPar> subpargeom = iop.subselect( sKey::GeomID() );
    if ( !subpargeom )
	subpargeom = iop.subselect( sKey::ID() );
    if ( subpargeom )
    {
	idx = 0;
	geomids_.erase();
	BufferString str;
	PosInfo::Line2DKey l2dkey;
	Pos::GeomID geomid = Survey::GM().cUndefGeomID();
	while ( subpargeom->get(toString(idx++),str) )
	{
	    l2dkey.fromString( str );
	    if ( !l2dkey.haveLSID() )
		geomid.fromString( str.buf() );
	    else
	    {
		S2DPOS().setCurLineSet( l2dkey.lsID() );
		geomid =  Survey::GM().getGeomID(
					S2DPOS().getLineSet(l2dkey.lsID()),
					S2DPOS().getLineName(l2dkey.lineID()) );
	    }

	    if ( geomid != Survey::GM().cUndefGeomID() )
		addGeomID( geomid );
	}
    }

    PtrMan<IOPar> subparzrg = iop.subselect( sKey::ZRange() );
    if ( subparzrg )
    {
	idx = 0;
	zrgs_.erase();
	StepInterval<float> zrg;
	while ( subparzrg->get(toString(idx++),zrg) )
	    zrgs_ += zrg;

	if ( zrgs_.isEmpty() )
	    zrgs_ += SI().zRange( false );
    }
}


void Pos::RangeProvider2D::fillPar( IOPar& iop ) const
{
    for ( int lidx=0; lidx<geomids_.size(); lidx++ )
	iop.set( IOPar::compKey(sKey::GeomID(),lidx), geomids_[lidx] );
    for ( int lidx=0; lidx<trcrgs_.size(); lidx++ )
	iop.set( IOPar::compKey(sKey::TrcRange(),lidx), trcrgs_[lidx] );
    for ( int lidx=0; lidx<zrgs_.size(); lidx++ )
	iop.set( IOPar::compKey(sKey::ZRange(),lidx), zrgs_[lidx] );
}


od_int64 Pos::RangeProvider2D::estNrPos() const
{
    od_int64 sz = 0;
    for ( int idx=0; idx<geomids_.size(); idx++ )
    {
	mGet2DGeometry(geomids_[idx]);
	if ( !geom2d )
	    continue;

	StepInterval<int> linetrcrg = trcrgs_.validIdx(idx) ? trcrgs_[idx]
							    : trcrgs_[0];
	const StepInterval<int> geomtrcrg = geom2d->data().trcNrRange();
	linetrcrg.limitTo( geomtrcrg );
	sz += linetrcrg.nrSteps();
    }

    if ( !sz && !trcrgs_[0].isUdf() )
	sz = trcrgs_[0].nrSteps() + 1;

    return sz;
}


int Pos::RangeProvider2D::estNrZPerPos() const
{ return zrgs_[0].nrSteps()+1; }


void Pos::RangeProvider2D::getSummary( BufferString& txt ) const
{
    for ( int idx=0; idx<trcrgs_.size(); idx++ )
    {
	txt += "Line "; txt += idx; txt += ":";
	const StepInterval<int>& rg = trcrgs_[0];
	const StepInterval<float>& zrg = zrgs_.validIdx(idx) ? zrgs_[idx]
							     : zrgs_[0];
	if ( rg.isUdf() )
	    txt += "[all]";
	else
	{
	    txt += rg.start; txt += "-";
	    txt += rg.stop;
	    if ( rg.step != 1 )
	    { txt += " step "; txt += rg.step; }
	}

	txt += " ("; txt += zrg.nrSteps() + 1; txt += " samples)";
	txt += "\n";
    }
}


void Pos::RangeProvider2D::initClass()
{ Pos::Provider2D::factory().addCreator( create, sKey::Range() ); }
