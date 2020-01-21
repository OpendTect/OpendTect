/*+
 *(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *Author:	Y.C. Liu
 *Date:		April 2007
-*/


#include "wellloginterpolator.h"

#include "fftfilter.h"
#include "gridder2d.h"
#include "interpollayermodel.h"
#include "keystrs.h"
#include "seisdatapack.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "wellextractdata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellmanager.h"
#include "welltrack.h"


namespace VolProc
{

static const char* sKeyNrWells()	{ return "Nr of wells"; }
static const char* sKeyWellLogID()	{ return "WellLog ID"; }
static const char* sKeyLogName()	{ return "Log name"; }
static const char* sKeyLayerModel()	{ return "Layer Model"; }


class WellLogInfo
{
public:
WellLogInfo( const DBKey& mid, const char* lognm, Well::ExtractParams params )
    : dbky_(mid)
    , logname_(lognm)
    , params_(params)
    , track_(0)
    , logisvelocity_(false)
{}

bool init( InterpolationLayerModel& layermodel )
{
    const bool zistime = SI().zIsTime();
    RefMan<Well::Data> wd = Well::MGR().fetchForEdit( dbky_,
		Well::LoadReqs(Well::Trck,Well::D2T,
			       Well::Logs).add(Well::Mrkrs) );
    if ( !wd )
	return false;

    delete track_;
    track_ = new Well::Track( wd->track() );
    if ( !getTrackSampling(wd->d2TModel()) )
	return false;

    layermodel.addSampling( bbox_.hsamp_ );
    if ( zistime )
	track_->toTime( *wd );

    wd_ = wd;

    return applyFilter( layermodel );
}


bool getTrackSampling( const Well::D2TModel& d2tmodel )
{
    Well::SimpleTrackSampler wts( *track_, d2tmodel, false, true );
    if ( !wts.execute() )
	return false;

    TrcKeySampling& tks = bbox_.hsamp_;
    const auto gs = tks.geomSystem();
    tks.init( false );
    TypeSet<BinID> trackpos;
    wts.getBIDs( trackpos );
    for ( int idx=0; idx<trackpos.size(); idx++ )
	tks.include( trackpos[idx] );

    tks.setGeomSystem( gs );
    return tks.isDefined();
}

#define cLogStepFact 10

bool applyFilter( const InterpolationLayerModel& layermodel )
{
    if ( log_ )
	log_->unRef();

    if ( !wd_ )
	return false;

    ConstRefMan<Well::Log> inplog = Well::MGR().getLog( dbky_, logname_ );
    if ( !inplog )
	return false;

    MonitorLock ml( *inplog );
    logisvelocity_ = inplog->propType() == PropertyRef::Vel;

    const Well::Track& track = wd_->track();
    const Well::D2TModel* d2t = wd_->d2TModelPtr();
    Interval<float> mdrg;
    BufferStringSet lognms; lognms.add( logname_ );
    mdrg = params_.calcFrom( *wd_, lognms );

    Interval<float> zrg;
    const bool zintime = SI().zIsTime();
    if ( zintime )
    {
	if ( !d2t ) return false;
	zrg.start = d2t->getTime( mdrg.start, track );
	zrg.stop = d2t->getTime( mdrg.stop, track );
	if ( zrg.isUdf() ) return false;
    }
    else
	zrg = track.zRange();

    ObjectSet<const Well::Log> logs;
    logs += inplog;
    const float extractstep = bbox_.zsamp_.step / cLogStepFact;
    Well::LogSampler ls( d2t, &track, zrg, zintime, extractstep,
			 zintime, params_.samppol_, logs ),
		     lsnear( d2t, &track, zrg, zintime, extractstep,
			     zintime, Stats::TakeNearest, logs );
    if ( !ls.execute() || !lsnear.execute() )
	return false;

    const BufferString newlognm( inplog->name(), " filtered" );
    ml.unlockNow();

    const int nrz = ls.nrZSamples();
    Array1DImpl<float> reglog( nrz );
    const int logidx = logs.size()-1;
    for ( int idz=0; idz<nrz; idz++ )
    {
	const float val = ls.getLogVal( logidx, idz );
	const float outval = mIsUdf(val) ? lsnear.getLogVal(logidx,idz) : val;
	reglog.set( idz, outval );
    }

    FFTFilter filter( nrz, extractstep );
    filter.setLowPass( 1.f / (2.f*bbox_.zsamp_.step) );
    if ( !filter.apply(reglog) )
	return false;

    RefMan<Well::Log> filteredlog = new Well::Log( newlognm );
    const int intstep = mNINT32(cLogStepFact);
    for ( int idz=0; idz<nrz; idz+=intstep )
    {
	const float md = ls.getDah( idz );
	if ( mIsUdf(md) )
	    continue;

	filteredlog->addValue( md, reglog.get(idz) );
    }

    if ( filteredlog->isEmpty() )
    {
	filteredlog = 0;
	return false;
    }

    log_ = filteredlog;
    wd_->logs().add( filteredlog );

    return createInterpolationFunctions( layermodel );
}

bool createInterpolationFunctions( const InterpolationLayerModel& layermodel )
{
    logfunc_.setEmpty();
    mdfunc_.setEmpty();

    const TrcKeySampling& hsamp = bbox_.hsamp_;
    Well::LogIter iter( *log_ );
    while ( iter.next() )
    {
	const float md = iter.dah();
	const Coord3 pos = track_->getPos( md );
	const TrcKey tk( hsamp.toTrcKey( pos.getXY() ) );
	const float layerz = layermodel.getLayerIndex(tk,mCast(float,pos.z_));
	const float logval = iter.value();
	if ( mIsUdf(layerz) || mIsUdf(logval) )
	    continue;

	logfunc_.add( layerz, logval );
	mdfunc_.add( layerz, md );
    }

    log_ = 0;

    return !logfunc_.isEmpty();
}


Well::Track*		track_;
RefMan<Well::Data>	wd_;
TrcKeyZSampling		bbox_;
DBKey			dbky_;
BufferString		logname_;
ConstRefMan<Well::Log>	log_;
bool			logisvelocity_;
Well::ExtractParams	params_;
PointBasedMathFunction	logfunc_;
PointBasedMathFunction	mdfunc_;

};



WellLogInterpolator::WellLogInterpolator()
    : gridder_(0)
    , layermodel_(0)
    , invdistgridder_(new InverseDistanceGridder2D())
    , trendorder_(PolyTrend::None)
    , doinverse_(false)
{}


WellLogInterpolator::~WellLogInterpolator()
{
    releaseData();
}


void WellLogInterpolator::releaseData()
{
    Step::releaseData();
    deleteAndZeroPtr( gridder_ );
    deleteAndZeroPtr( layermodel_ );
    deepErase( infos_ );
    deleteAndZeroPtr( invdistgridder_ );
}


void WellLogInterpolator::setLayerModel( InterpolationLayerModel* mdl )
{
    delete layermodel_;
    layermodel_ = mdl;
}


const InterpolationLayerModel* WellLogInterpolator::getLayerModel() const
{ return layermodel_; }


void WellLogInterpolator::setGridder( const Gridder2D* gridder )
{
    delete gridder_;
    gridder_ = gridder->clone();
}


void WellLogInterpolator::setGridder( const IOPar& par )
{
    deleteAndZeroPtr( gridder_ );
    PtrMan<IOPar> velgridpar = par.subselect( Gridder2D::sKeyGridder() );
    if ( velgridpar )
    {
	BufferString nm;
	velgridpar->get( sKey::Name(), nm );
	gridder_ = Gridder2D::factory().create( nm.buf() );
	if ( !gridder_ )
	    return;

	if ( !gridder_->usePar(*velgridpar) )
	    { deleteAndZeroPtr( gridder_ ); return; }

	PolyTrend::OrderDef().parse( *velgridpar, PolyTrend::sKeyOrder(),
				     trendorder_ );
    }
    else
    {
	IOPar gridpar;
	BufferString griddernm;
	if ( !par.get("Algorithm",griddernm) )
	    return;

	gridder_ = Gridder2D::factory().create( griddernm.buf() );
	if ( !gridder_ )
	    return;

	float radius;
	gridpar.set( sKey::Name(), griddernm );
	if ( par.get(InverseDistanceGridder2D::sKeySearchRadius(),radius) )
	    gridpar.set( InverseDistanceGridder2D::sKeySearchRadius(), radius );

	if ( !gridder_->usePar(par) )
	    { deleteAndZeroPtr( gridder_ ); return; }
    }
}


bool WellLogInterpolator::is2D() const
{ return false; }


void WellLogInterpolator::setWellData( const DBKeySet& ids, const char* lognm )
{
    wellmids_ = ids;
    logname_ = lognm;
}


void WellLogInterpolator::getWellNames( BufferStringSet& res ) const
{
    for ( int idx=0; idx<wellmids_.size(); idx++ )
	res.add( wellmids_[idx].name() );
}


const char* WellLogInterpolator::getLogName() const
{ return logname_.buf(); }

void WellLogInterpolator::getWellIDs( DBKeySet& ids ) const
{ ids = wellmids_; }


bool WellLogInterpolator::prepareComp( int )
{
    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    if ( !output || output->isEmpty() || is2D() )
	return false;

    if ( !gridder_ )
	{ errmsg_ = tr("Cannot retrieve gridding method"); return false; }

    const TrcKeySampling hs( output->horSubSel() );
    if ( !layermodel_ || !layermodel_->prepare(hs,SilentTaskRunnerProvider()) )
	{ errmsg_ = tr("Could not prepare the interpolation"); return false; }

    outputinlrg_ = hs.inlRange();
    outputcrlrg_ = hs.crlRange();
    uiStringSet errmsgs;
    for ( int idx=0; idx<wellmids_.size(); idx++ )
    {
	WellLogInfo* info = new WellLogInfo( wellmids_[idx], logname_,
								    params_ );
	if ( !info->init(*layermodel_) )
	{
	    if ( info->wd_ )
	    {
		errmsgs.add( tr("Cannot load log '%1' for well '%2'")
				.arg( logname_ ).arg( info->wd_->name() ) );
	    }
	    else
	    {
		errmsgs.add( tr("Cannot load log '%1' for well with ID '%2'")
				.arg( logname_ ).arg( wellmids_[idx] ) );
	    }
	    delete info;
	    continue;
	}

	infos_ += info;
    }

    if ( !errmsgs.isEmpty() )
    {
	errmsg_ = errmsgs.cat();
	deepErase( infos_ );
	return false;
    }

    if ( infos_.isEmpty() )
    {
	errmsg_ = tr("All wells rejected for processing");
	return false;
    }

    doinverse_ = infos_[0]->logisvelocity_;

    return true;
}


static void getCornerPoints( const BinID& bid, TypeSet<Coord>& corners )
{
    corners.setEmpty();
    const TrcKeyZSampling tkzs( true );
    const TrcKeySampling& tks = tkzs.hsamp_;

    TypeSet<BinID> cornerbids;
    cornerbids.add( bid );

    cornerbids.addIfNew( BinID( tks.start_.inl(), tks.start_.crl() ) );
    cornerbids.addIfNew( BinID( tks.start_.inl(), tks.stop_.crl() ) );
    cornerbids.addIfNew( BinID( tks.stop_.inl(), tks.start_.crl() ) );
    cornerbids.addIfNew( BinID( tks.stop_.inl(), tks.stop_.crl() ) );

    for ( int idx=1; idx<cornerbids.size(); idx++ )
    {
	const BinID& cornerbid = cornerbids[idx];
	corners.add( tks.toCoord(cornerbid) );
    }
}


static void addCornerPoints( const Gridder2D& gridder,
			     Gridder2D& invdistgridder,
			     const TypeSet<Coord>& corners,
			     TypeSet<Coord>& points, TypeSet<float>& vals )
{
    if ( BufferString(gridder.factoryKeyword()) ==
	    InverseDistanceGridder2D::sFactoryKeyword() ||
	    points.size() < 2 )
	return;

    TypeSet<Coord> initialpoints( points );
    TypeSet<float> initialvals( vals );
    invdistgridder.setPoints( initialpoints, SilentTaskRunnerProvider() );
    invdistgridder.setValues( initialvals );

    for ( int idx=0; idx<corners.size(); idx++ )
    {
	const Coord& gridpoint = corners[idx];
	const float cornerval = invdistgridder.getValue( gridpoint );
	if ( mIsUdf(cornerval) )
	    continue;

	points += gridpoint;
	vals += cornerval;
    }
}


bool WellLogInterpolator::computeBinID( const BinID& bid, int )
{
    if ( !outputinlrg_.includes( bid.inl(), true ) ||
	 !outputcrlrg_.includes( bid.crl(), true ) ||
         (bid.inl()-outputinlrg_.start)%outputinlrg_.step ||
	 (bid.crl()-outputcrlrg_.start)%outputcrlrg_.step )
	return true;

    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    Array3D<float>& outputarray = output->data(0);
    const int nrz = output->nrZ();
    const int nrwells = infos_.size();

    TypeSet<Coord> corners;
    getCornerPoints( bid, corners );

    const TrcKeySampling hs( output->horSubSel() );
    const int outputinlidx = outputinlrg_.nearestIndex( bid.inl() );
    const int outputcrlidx = outputcrlrg_.nearestIndex( bid.crl() );
    const Coord gridpoint( hs.toCoord(bid) );
    const TrcKey tk( hs.geomSystem(), bid );
    Gridder2D* gridder = gridder_->clone();
    Gridder2D* invdistgridder = invdistgridder_->clone();
    TypeSet<Coord> points;
    TypeSet<float> logvals;
    const auto zrg( output->zRange() );
    for ( int idz=0; idz<nrz; idz++ )
    {
	const float z = zrg.atIndex( idz );
	const float layeridx = layermodel_->getLayerIndex( tk, z );
	if ( mIsUdf(layeridx) )
	{
	    outputarray.set( outputinlidx, outputcrlidx, idz, mUdf(float) );
	    continue;
	}

	points.setEmpty();
	logvals.setEmpty();
	for ( int iwell=0; iwell<nrwells; iwell++ )
	{
	    const float md = infos_[iwell]->mdfunc_.getValue( layeridx );
	    const Coord pos = infos_[iwell]->track_->getPos( md ).getXY();
	    const float logval = infos_[iwell]->logfunc_.getValue( layeridx );
	    if ( mIsUdf(md) || mIsUdf(logval) || mIsUdf(pos) )
		continue;

	    points += pos;
	    logvals += doinverse_ ? 1.f / logval : logval;
	}

	if ( points.isEmpty() )
	{
	    outputarray.set( outputinlidx, outputcrlidx, idz, mUdf(float) );
	    continue;
	}

	addCornerPoints( *gridder, *invdistgridder, corners, points, logvals );
	if ( !gridder->setPoints(points,SilentTaskRunnerProvider())
	  || !gridder->setValues(logvals) )
	{
	    outputarray.set( outputinlidx, outputcrlidx, idz, mUdf(float) );
	    continue;
	}

	gridder->setTrend( trendorder_ );
	float val = gridder->getValue( gridpoint );
	if ( doinverse_ && !mIsUdf(val) ) val = 1.f / val;
	outputarray.set( outputinlidx, outputcrlidx, idz, val );
    }

    delete gridder;
    delete invdistgridder;

    return true;
}


void WellLogInterpolator::fillPar( IOPar& pars ) const
{
    Step::fillPar( pars );

    params_.fillPar( pars );

    if ( gridder_ )
    {
	IOPar gridpar;
	gridpar.set( sKey::Name(), gridder_->factoryKeyword() );
	gridder_->fillPar( gridpar );
	gridpar.set( PolyTrend::sKeyOrder(),
		     PolyTrend::OrderDef().getKey(trendorder_) );
	pars.mergeComp( gridpar, Gridder2D::sKeyGridder() );
    }

    pars.set( sKeyLogName(), logname_ );
    pars.set( sKeyNrWells(), wellmids_.size() );
    for ( int idx=0; idx<wellmids_.size(); idx++ )
    {
	const BufferString key = IOPar::compKey( sKeyWellLogID(), idx );
	pars.set( key, wellmids_[idx] );
    }

    if ( layermodel_ )
    {
	IOPar lmpar;
	layermodel_->fillPar( lmpar );
	pars.mergeComp( lmpar, sKeyLayerModel() );
    }
}


bool WellLogInterpolator::usePar( const IOPar& pars )
{
    if ( !Step::usePar(pars) )
	return false;

    const bool hassamplepol = pars.find( Well::ExtractParams::sKeySamplePol() );
    const Stats::UpscaleType samppol = params_.samppol_;
    params_.usePar( pars );
    if ( !hassamplepol )
	params_.samppol_ = samppol;

    pars.get( sKeyLogName(), logname_ );

    wellmids_.erase();
    int nrwells = 0;
    pars.get( sKeyNrWells(), nrwells );
    for ( int idx=0; idx<nrwells; idx++ )
    {
	DBKey mid;
	const BufferString key = IOPar::compKey( sKeyWellLogID(), idx );
	if ( pars.get(key,mid) )
	    wellmids_ += mid;
    }

    setGridder( pars );

    PtrMan<IOPar> lmpar = pars.subselect( sKeyLayerModel() );
    BufferString lmtype( ZSliceInterpolationModel::sFactoryKeyword() );
    if ( lmpar )
	lmpar->get( InterpolationLayerModel::sKeyModelType(), lmtype );

    delete layermodel_;
    layermodel_ = InterpolationLayerModel::factory().create( lmtype );
    if ( !layermodel_ || (lmpar && !layermodel_->usePar(*lmpar)) )
	deleteAndZeroPtr( layermodel_ );

    return true;
}


od_int64 WellLogInterpolator::extraMemoryUsage( OutputSlotID,
					    const TrcKeyZSampling& tkzs ) const
{
    return layermodel_ ? layermodel_->getMemoryUsage( tkzs.hsamp_ ) : 0;
}


} // namespace VolProc
