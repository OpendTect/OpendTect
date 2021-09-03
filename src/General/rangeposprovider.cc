/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2008
-*/


#include <tuple>

#include "rangeposprovider.h"

#include "fullsubsel.h"
#include "iopar.h"
#include "keystrs.h"
#include "posinfo2d.h"
#include "statrand.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "trckeyzsampling.h"
#include "uistrings.h"

#define mGet2DGeometry(gid) \
    const auto& geom2d = SurvGeom::get2D( gid )


Pos::RangeProvider3D::RangeProvider3D()
    : Pos::Provider3D()
    , tkzs_(*new TrcKeyZSampling(true))
    , zsampsz_(0)
    , nrsamples_(mUdf(int))
    , dorandom_(false)
    , enoughsamples_(true)
{
    reset();
}


Pos::RangeProvider3D::RangeProvider3D( const Pos::RangeProvider3D& p )
    : Pos::Provider3D(p)
    , tkzs_(*new TrcKeyZSampling(false))
{
    *this = p;
}


Pos::RangeProvider3D::~RangeProvider3D()
{
    delete &tkzs_;
}


Pos::RangeProvider3D& Pos::RangeProvider3D::operator =(
					const Pos::RangeProvider3D& oth )
{
    if ( &oth == this )
	return *this;

    Pos::Provider3D::operator = ( oth );

    tkzs_ = oth.tkzs_;
    curbid_ = oth.curbid_;
    curzidx_ = oth.curzidx_;
    zsampsz_ = oth.zsampsz_;
    nrsamples_ = oth.nrsamples_;
    dorandom_ = oth.dorandom_;
    enoughsamples_ = oth.enoughsamples_;
    posindexlst_.setEmpty();

    return *this;
}


void Pos::RangeProvider3D::setSampling( const TrcKeyZSampling& tkzs )
{
    tkzs_ = tkzs;
    if ( dorandom_ )
    {
	// For random sampling use the survey steps/sampling
	TrcKeyZSampling sitkzs;
	SI().getSampling( sitkzs );
	tkzs_.hsamp_.step_ = sitkzs.hsamp_.step_;
	tkzs_.zsamp_.step = sitkzs.zsamp_.step;
    }
    zsampsz_ = tkzs.zsamp_.nrSteps()+1;
}


const char* Pos::RangeProvider3D::type() const
{
    return sKey::Range();
}


bool Pos::RangeProvider3D::initialize( const TaskRunnerProvider& )
{
    if ( dorandom_ )
    {
	// For random sampling use the survey steps/sampling
	TrcKeyZSampling sitkzs;
	SI().getSampling( sitkzs );
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
	const Stats::RandGen& randGen = Stats::randGen();
	const TrcKeySampling& hsamp = tkzs_.hsamp_;
	const od_int64 totalNrTraces = hsamp.totalNr();
	do
	{
	    idx = randGen.getIndex( totalNrTraces );
	    curzidx_ = randGen.getInt( 0, zsampsz_-1 );
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

    Survey::FullSubSel fss;
    fss.setEmpty();
    fss.usePar( iop );
    if ( fss.is3D() && fss.nrGeomIDs() == 1 )
    {
	tkzs_.hsamp_.setInlRange( fss.inlRange() );
	tkzs_.hsamp_.setCrlRange( fss.crlRange() );
	tkzs_.zsamp_ = fss.zRange();
    }
    else
    {
	iop.getYN( sKey::Random(), dorandom_ );
	tkzs_.usePar( iop );
	if ( dorandom_ )
	    iop.get( sKey::NrValues(), nrsamples_);
    }

    zsampsz_ = tkzs_.zsamp_.nrSteps()+1;
}


void Pos::RangeProvider3D::fillPar( IOPar& iop ) const
{
    tkzs_.fillPar( iop );
    iop.setYN( sKey::Random(), dorandom_ );
    if ( dorandom_ )
	iop.set( sKey::NrValues(), nrsamples_ );
}


void Pos::RangeProvider3D::getSummary( uiString& txt ) const
{
    txt.appendPhrase( toUiString("%1 - %2").arg(tkzs_.hsamp_.start_)
	    .arg(tkzs_.hsamp_.stop_), uiString::Space, uiString::OnSameLine );
    const int nrsamps = zsampsz_;
    if ( nrsamps > 1 )
	txt.appendPhrase( toUiString(" (%1 %2)").arg(nrsamps)
			.arg( uiStrings::sSample(mPlural).toLower() ) );
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
    if ( dorandom_ )
	return nrsamples_;
    else
	return tkzs_.hsamp_.totalNr();
}


int Pos::RangeProvider3D::estNrZPerPos() const
{
    if ( dorandom_ )
	return 1;
    else
	return zsampsz_;
}


void Pos::RangeProvider3D::getTrcKeyZSampling( TrcKeyZSampling& tkzs ) const
{
    tkzs = tkzs_;
}


void Pos::RangeProvider3D::setHSampling( const TrcKeySampling& tks )
{
    tkzs_.hsamp_ = tks;
    if ( dorandom_ )
    {
	// For random sampling use the survey steps/sampling
	TrcKeyZSampling sitkzs;
	SI().getSampling( sitkzs );
	tkzs_.hsamp_.step_ = sitkzs.hsamp_.step_;
    }
}


void Pos::RangeProvider3D::initClass()
{
    Pos::Provider3D::factory().addCreator( create, sKey::Range(),
							uiStrings::sRange() );
}


Pos::RangeProvider2D::RangeProvider2D()
    : curgeom_(0)
    , curtrcrg_(StepInterval<int>::udf())
    , curzrg_(StepInterval<float>::udf())
{
    zrgs_ += SI().zRange();
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
    unRefAndZeroPtr( curgeom_ );
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
	unRefAndZeroPtr( curgeom_ );
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
    unRefAndZeroPtr( curgeom_ );
    StepInterval<float> zrg = zrgs_[0];
    if ( !geomids_.isEmpty() )
    {
	mGet2DGeometry( geomids_[0] );
	curgeom_ = &geom2d;
	curgeom_->ref();
	getCurRanges();
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

    mGet2DGeometry( geomids_[lidx] );
    trcrgs_[lidx].limitTo( geom2d.data().trcNrRange() );
}


void Pos::RangeProvider2D::setZRange( const StepInterval<float>& zrg, int lidx )
{
    if ( !zrgs_.validIdx(lidx) )
	return;

    zrgs_[lidx] = zrg;
    if ( !geomids_.validIdx(lidx) )
	return;

    mGet2DGeometry( geomids_[lidx] );
    zrgs_[lidx].limitTo( geom2d.data().zRange() );
}


TrcKey Pos::RangeProvider2D::curTrcKey() const
{
    const Pos::GeomID gid = curGeomID();
    return gid.isValid() ? TrcKey( curBin2D() ) : TrcKey::udf();
}


Pos::GeomID Pos::RangeProvider2D::curGeomID() const
{
    return geomids_.validIdx(curlineidx_) ? geomids_[curlineidx_]
					  : Pos::GeomID();
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


const SurvGeom2D* Pos::RangeProvider2D::curGeom() const
{
    if ( !curgeom_ )
    {
	mGet2DGeometry( curGeomID() );
	const_cast<Pos::RangeProvider2D*>(this)->curgeom_ = &geom2d;
	curgeom_->ref();
	getCurRanges();
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
	unRefAndZeroPtr( curgeom_ );
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


int Pos::RangeProvider2D::curTrcNr() const
{
    StepInterval<int> trcrg = curTrcRange();
    return trcrg.atIndex( curtrcidx_ );
}


Bin2D Pos::RangeProvider2D::curBin2D() const
{
    return Bin2D( curGeomID(), curTrcNr() );
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
    ConstRefMan<SurvGeom2D> curgeom = curGeom();
    if ( curgeom )
    {
	PosInfo::Line2DPos l2dpos;
	if ( curgeom->data().getPos(curTrcNr(),l2dpos) )
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
    if ( lidx < 0 )
	return false;

    if ( lidx == curlineidx_ || !geomids_.validIdx(lidx) )
	return curTrcRange().includes(nr,false)
	    && curZRange().includes(z,false);

    mGet2DGeometry( geomids_[lidx] );

    StepInterval<int> trcrg = trcrgs_.validIdx(lidx) ? trcrgs_[lidx]
						     : trcrgs_[0];
    trcrg.limitTo( geom2d.data().trcNrRange() );
    StepInterval<float> zrg = zrgs_.validIdx(lidx) ? zrgs_[lidx] : zrgs_[0];
    zrg.limitTo( geom2d.data().zRange() );
    return trcrg.includes(nr,false) && zrg.includes(z,false);
}


bool Pos::RangeProvider2D::includes( const Coord& c, float z ) const
{
    ConstRefMan<SurvGeom2D> curgeom = curGeom();

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
	mGet2DGeometry( geomids_[lidx] );
	if ( geom2d.isEmpty() )
	    continue;

	const TypeSet<PosInfo::Line2DPos>& pos = geom2d.data().positions();
	for ( int idx=0; idx<pos.size(); idx++ )
	{
	    if ( pos[idx].coord_ == c )
	    {
		StepInterval<int> trcrg = trcrgs_.validIdx(lidx) ? trcrgs_[lidx]
								 : trcrgs_[0];
		StepInterval<float> zrg = zrgs_.validIdx(lidx) ? zrgs_[lidx]
							       : zrgs_[0];
		trcrg.limitTo( geom2d.data().trcNrRange() );
		zrg.limitTo( geom2d.data().zRange() );
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
    { rg = curTrcRange(); return; }

    rg = trcrgs_.validIdx(lidx) ? trcrgs_[lidx] : trcrgs_[0];
    mGet2DGeometry( geomids_[lidx] );
    rg.limitTo( geom2d.data().trcNrRange() );
}


void Pos::RangeProvider2D::getZRange( Interval<float>& zrg, int lidx ) const
{
    if ( lidx == curlineidx_ || !geomids_.validIdx(lidx) )
    { zrg = curZRange(); return; }

    zrg = zrgs_.validIdx(lidx) ? zrgs_[lidx] : zrgs_[0];
    mGet2DGeometry( geomids_[lidx] );
    zrg.limitTo( geom2d.data().zRange() );
}


void Pos::RangeProvider2D::usePar( const IOPar& iop )
{
    PtrMan<IOPar> geomidsubpar = iop.subselect( sKey::GeomID() );
    if ( !geomidsubpar || geomidsubpar->isEmpty() )
	return;

    GeomIDSet newgeomids;
    TypeSet<int> lidxs;
    int lastvalidlidx = -1;
    for ( int lidx=0; ; lidx++ )
    {
	Pos::GeomID geomid;
	geomidsubpar->get( toString(lidx), geomid );
	if ( !geomid.isValid() )
	{
	    if ( lidx-lastvalidlidx > 10 )
		break;
	    continue;
	}

	lastvalidlidx = lidx;
	newgeomids += geomid;
	lidxs += lidx;
    }
    if ( newgeomids.isEmpty() )
	return;

    geomids_ = newgeomids;
    trcrgs_.erase(); zrgs_.erase();
    for ( auto lidx : lidxs )
    {
	StepInterval<int> trcrg( 1, mUdf(int), 1 );
	iop.get( IOPar::compKey(sKey::TrcRange(),lidx), trcrg );
	trcrgs_ += trcrg;
	StepInterval<float> zrg( SI().zRange() );
	iop.get( IOPar::compKey(sKey::ZRange(),lidx), zrg );
	zrgs_ += zrg;
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
	StepInterval<int> linetrcrg = trcrgs_.validIdx(idx) ? trcrgs_[idx]
							    : trcrgs_[0];
	const StepInterval<int> geomtrcrg = geom2d.data().trcNrRange();
	linetrcrg.limitTo( geomtrcrg );
	sz += linetrcrg.nrSteps();
    }

    if ( !sz && !trcrgs_[0].isUdf() )
	sz = trcrgs_[0].nrSteps() + 1;

    return sz;
}


int Pos::RangeProvider2D::estNrZPerPos() const
{
    return zrgs_[0].nrSteps()+1;
}


void Pos::RangeProvider2D::getSummary( uiString& txt ) const
{
    if ( geomids_.size() < 2 && !trcrgs_.isEmpty() )
    {
	txt = tr("Trace Range: %1-%2-%3").arg(trcrgs_[0].start)
		    .arg(trcrgs_[0].stop).arg(trcrgs_[0].step);
	if ( !zrgs_.isEmpty() )
	{
	    txt.appendPhrase(toUiString("%1 %2")).arg(zrgs_[0].nrSteps() + 1)
			    .arg(uiStrings::sSample(mPlural)).parenthesize();
	}

	return;
    }

    txt = uiStrings::s2DLine();
    BufferStringSet geomnms;

    for ( int idx=0; idx<geomids_.size(); idx++ )
	geomnms.add( geomids_[idx].name() );

    txt.appendPhrase(toUiString(geomnms.getDispString()),uiString::MoreInfo,
						    uiString::OnSameLine);
}


void Pos::RangeProvider2D::initClass()
{
    Pos::Provider2D::factory().addCreator( create, sKey::Range(),
						    uiStrings::sRange() );
}
