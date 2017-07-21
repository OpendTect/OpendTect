/*+
 *(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *Author:	Y.C. Liu
 *Date:		April 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "wellloginterpolator.h"

#include "arrayndimpl.h"
#include "fftfilter.h"
#include "gridder2d.h"
#include "interpollayermodel.h"
#include "seisdatapack.h"
#include "survinfo.h"
#include "welldata.h"
#include "wellextractdata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "wellreader.h"
#include "welltrack.h"
#include "wellextractdata.h"
#include "hiddenparam.h"


namespace VolProc
{

static const char* sKeyNrWells()	{ return "Nr of wells"; }
static const char* sKeyWellLogID()	{ return "WellLog ID"; }
static const char* sKeyLogName()	{ return "Log name"; }
static const char* sKeyAlgoName()	{ return "Algorithm"; }
static const char* sKeyLayerModel()	{ return "Layer Model"; }


class WellLogInfo
{
public:
WellLogInfo( const MultiID& mid, const char* lognm, Well::ExtractParams params )
    : mid_(mid)
    , logname_(lognm)
    , params_(params)
    , track_(0)
    , log_(0)
{}

~WellLogInfo()
{
    delete track_;
    delete log_;
}

bool init( const InterpolationLayerModel& layermodel )
{
    const bool isloaded = Well::MGR().isLoaded( mid_ );
    const bool zistime = SI().zIsTime();
    RefMan<Well::Data> wd = new Well::Data;
    if ( isloaded )
    {
	if ( !wd )
	    return false;

	const Well::Data* wddb = Well::MGR().get( mid_ );
	if ( !wddb || wddb->track().size() < 2 ||
	     ( zistime && (!wddb->d2TModel() || wddb->d2TModel()->size() < 2)) )
	    return false;

	wd = const_cast<Well::Data*>( wddb );
	if ( !wd->logs().getLog(logname_) )
	{
	    Well::Reader wrdr( mid_, *wd );
	    if ( !wrdr.getLog(logname_) )
		return false;
	}
    }
    else
    {
	Well::Reader wrdr( mid_, *wd );
	if ( !wrdr.getTrack() || ( zistime && !wrdr.getD2T() ) ||
	     !wrdr.getLog(logname_) )
	    return false;
    }

    delete track_;
    track_ = new Well::Track( wd->track() );
    if ( zistime )
	track_->toTime( *wd );

    const Well::Log* log = wd->logs().getLog( logname_ );
    if ( !log )
	return false;

    delete log_;
    log_ = applyFilter( *wd, *log );

    return log_ && createInterpolationFunctions( layermodel );
}

#define cLogStepFact 10

Well::Log* applyFilter( const Well::Data& wd, const Well::Log& log ) const
{
    const Well::Track& track = wd.track();
    const Well::D2TModel* d2t = wd.d2TModel();
    Interval<float> mdrg;
    if ( wd.logs().getLog(log.name()) )
    {
	BufferStringSet lognms; lognms.add( log.name() );
	mdrg = params_.calcFrom( wd, lognms );
    }
    else
	mdrg = log.dahRange();

    if ( mdrg.isUdf() )
	return 0;

    Interval<float> zrg;
    const bool zintime = SI().zIsTime();
    if ( zintime )
    {
	if ( !d2t ) return 0;
	zrg.start = d2t->getTime( mdrg.start, track );
	zrg.stop = d2t->getTime( mdrg.stop, track );
	if ( zrg.isUdf() ) return 0;
    }

    ObjectSet<const Well::Log> logs;
    logs += &log;
    const float extractstep = bbox_.zsamp_.step / cLogStepFact;
    Well::LogSampler ls( d2t, &track, zrg, zintime, extractstep,
			 zintime, params_.samppol_, logs ),
		     lsnear( d2t, &track, zrg, zintime, extractstep,
			     zintime, Stats::TakeNearest, logs );
    if ( !ls.execute() || !lsnear.execute() )
	return 0;

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
	return 0;

    Well::Log* filteredlog = new Well::Log;
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
	delete filteredlog;
	return 0;
    }

    return filteredlog;
}

bool createInterpolationFunctions( const InterpolationLayerModel& layermodel )
{
    logfunc_.setEmpty();
    mdfunc_.setEmpty();

    const TrcKeySampling& hsamp = bbox_.hsamp_;
    const int nrlogvals = log_->size();
    for ( int idz=0; idz<nrlogvals; idz++ )
    {
	const float md = log_->dah( idz );
	const Coord3 pos = track_->getPos( md );
	const BinID bid = hsamp.toTrcKey( pos ).position();
	const float layerz = layermodel.getLayerIndex( bid, mCast(float,pos.z));
	const float logval = log_->value( idz );
	if ( mIsUdf(layerz) || mIsUdf(logval) )
	    continue;

	logfunc_.add( layerz, logval );
	mdfunc_.add( layerz, md );
    }

    return !logfunc_.isEmpty();
}


Well::Track*		track_;
TrcKeyZSampling		bbox_;
MultiID			mid_;
BufferString		logname_;
const Well::Log*	log_;
Well::ExtractParams	params_;
PointBasedMathFunction	logfunc_;
PointBasedMathFunction	mdfunc_;

};


HiddenParam<WellLogInterpolator,InverseDistanceGridder2D*> invdistgridder_(0);
HiddenParam<WellLogInterpolator,PolyTrend::Order>
					   trendorder_(PolyTrend::Order(0));
HiddenParam<WellLogInterpolator,Well::ExtractParams*> params_( 0 );

WellLogInterpolator::WellLogInterpolator()
    : gridder_(0)
    , extension_(ExtrapolateEdgeValue)
    , extlog_(0)
    , layermodel_(0)
{
    invdistgridder_.setParam( this, new InverseDistanceGridder2D() );
    trendorder_.setParam( this, PolyTrend::None );
    params_.setParam( this, new Well::ExtractParams() );
}


WellLogInterpolator::~WellLogInterpolator()
{
    releaseData();
    invdistgridder_.removeParam( this );
    trendorder_.removeParam( this );
    params_.removeParam( this );
}


void WellLogInterpolator::releaseData()
{
    Step::releaseData();
    deleteAndZeroPtr( gridder_ );
    deleteAndZeroPtr( layermodel_ );
    deepErase( infos_ );
    InverseDistanceGridder2D* invdistgridder = invdistgridder_.getParam( this );
    delete invdistgridder;
    invdistgridder_.setParam( this, new InverseDistanceGridder2D() );
    params_.getParam( this )->setEmpty();
}


void WellLogInterpolator::setLayerModel( InterpolationLayerModel* mdl )
{
    delete layermodel_;
    layermodel_ = mdl;
}


const InterpolationLayerModel* WellLogInterpolator::getLayerModel() const
{ return layermodel_; }


void WellLogInterpolator::setGridder( const char* nm, float radius )
{
    delete gridder_;

    if ( FixedString(nm) == RadialBasisFunctionGridder2D::sFactoryKeyword() )
    {
	gridder_ = new RadialBasisFunctionGridder2D();
	trendorder_.setParam( this, PolyTrend::Order2 );
	return;
    }

    if ( FixedString(nm) ==  TriangulatedGridder2D::sFactoryKeyword() )
    {
	gridder_ = new TriangulatedGridder2D();
	trendorder_.setParam( this, PolyTrend::None );
	return;
    }

    if ( FixedString(nm) == InverseDistanceGridder2D::sFactoryKeyword() )
    {
	InverseDistanceGridder2D* invgrid = new InverseDistanceGridder2D();
	gridder_ = invgrid;
	trendorder_.setParam( this, PolyTrend::None );
	if ( !mIsUdf(radius) )
	    invgrid->setSearchRadius( radius );

	return;
    }
}


const char* WellLogInterpolator::getGridderName() const
{
    mDynamicCastGet( InverseDistanceGridder2D*, invgrid, gridder_ );
    if ( invgrid )
	return invgrid->factoryKeyword();

    mDynamicCastGet( TriangulatedGridder2D*, trigrid, gridder_ );
    if ( trigrid )
	return trigrid->factoryKeyword();

    mDynamicCastGet( RadialBasisFunctionGridder2D*, rbfgrid, gridder_ );
    if ( rbfgrid )
	return rbfgrid->factoryKeyword();

    return 0;
}


float WellLogInterpolator::getSearchRadius() const
{
    mDynamicCastGet( InverseDistanceGridder2D*, invgrid, gridder_ );
    return invgrid ? invgrid->getSearchRadius() : mUdf( float );
}


bool WellLogInterpolator::is2D() const
{ return false; }


void WellLogInterpolator::setWellData( const TypeSet<MultiID>& ids,
				       const char* lognm )
{
    wellmids_ = ids;
    logname_ = lognm;
}


void WellLogInterpolator::setWellExtractParams(
					const Well::ExtractParams& params )
{
    Well::ExtractParams* curparams = params_.getParam( this );
    if ( !curparams )
	return;

    *curparams = params;
}


const Well::ExtractParams& WellLogInterpolator::getWellExtractParams()
{
    return *params_.getParam( this );
}


void WellLogInterpolator::getWellNames( BufferStringSet& res ) const
{
    for ( int idx=0; idx<wellmids_.size(); idx++ )
    {
	Well::Data* data = Well::MGR().get( wellmids_[idx] );
	if ( data )
	    res.add( data->name() );
    }
}


const char* WellLogInterpolator::getLogName() const
{ return logname_.buf(); }

void WellLogInterpolator::getWellIDs( TypeSet<MultiID>& ids ) const
{ ids = wellmids_; }


bool WellLogInterpolator::prepareComp( int )
{
    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    if ( !output || output->isEmpty() || is2D() )
	return false;

    if ( !gridder_ )
	{ errmsg_ = tr("Cannot retrieve gridding method"); return false; }

    if ( !layermodel_ || !layermodel_->prepare(0) )
    {
	errmsg_ = tr("Could not prepare the interpolation model");
	return false;
    }

    const TrcKeySampling& hs = output->sampling().hsamp_;
    outputinlrg_ = hs.inlRange();
    outputcrlrg_ = hs.crlRange();
    uiStringSet errmsgs;
    for ( int idx=0; idx<wellmids_.size(); idx++ )
    {
	WellLogInfo* info = new WellLogInfo( wellmids_[idx], logname_,
					     getWellExtractParams() );
	if ( !info->init(*layermodel_) )
	{
	    RefMan<Well::Data> wd = Well::MGR().get( wellmids_[idx] );
	    if ( wd )
	    {
		errmsgs.add( tr("Cannot load log '%1' for well '%2'")
				    .arg( logname_ ).arg( wd->name() ) );
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

    return true;
}


static void getCornerPoints( const BinID& bid, TypeSet<Coord>& corners )
{
    corners.setEmpty();
    const TrcKeyZSampling tkzs( SI().sampling( false ) );
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
    invdistgridder.setPoints( initialpoints );
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
    if ( infos_.isEmpty() )
	return false;

    if ( !outputinlrg_.includes( bid.inl(), true ) ||
	 !outputcrlrg_.includes( bid.crl(), true ) ||
         (bid.inl()-outputinlrg_.start)%outputinlrg_.step ||
	 (bid.crl()-outputcrlrg_.start)%outputcrlrg_.step )
	return true;

    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    Array3D<float>& outputarray = output->data(0);
    const int nrz = output->sampling().nrZ();
    const int nrwells = infos_.size();

    TypeSet<Coord> corners;
    getCornerPoints( bid, corners );

    const TrcKeySampling& hs = output->sampling().hsamp_;
    const int outputinlidx = outputinlrg_.nearestIndex( bid.inl() );
    const int outputcrlidx = outputcrlrg_.nearestIndex( bid.crl() );
    const Coord gridpoint( hs.toCoord(bid) );
    Gridder2D* gridder = gridder_->clone();
    Gridder2D* invdistgridder = invdistgridder_.getParam( this )->clone();
    TypeSet<Coord> points;
    TypeSet<float> logvals;
    for ( int idz=0; idz<nrz; idz++ )
    {
	const float z = output->sampling().zsamp_.atIndex( idz );
	const float layeridx = layermodel_->getLayerIndex( bid, z );
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
	    const Coord pos = infos_[iwell]->track_->getPos( md ).coord();
	    const float logval = infos_[iwell]->logfunc_.getValue( layeridx );
	    if ( mIsUdf(md) || mIsUdf(logval) || mIsUdf(pos) )
		continue;

	    points += pos;
	    logvals += logval;
	}

	if ( points.isEmpty() )
	{
	    outputarray.set( outputinlidx, outputcrlidx, idz, mUdf(float) );
	    continue;
	}

	addCornerPoints( *gridder, *invdistgridder, corners, points, logvals );
	if ( !gridder->setPoints(points) || !gridder->setValues(logvals) )
	{
	    outputarray.set( outputinlidx, outputcrlidx, idz, mUdf(float) );
	    continue;
	}

	gridder->setTrend( trendorder_.getParam(this) );
	const float val = gridder->getValue( gridpoint );
	outputarray.set( outputinlidx, outputcrlidx, idz, val );
    }

    delete gridder;
    delete invdistgridder;

    return true;
}


void WellLogInterpolator::fillPar( IOPar& pars ) const
{
    Step::fillPar( pars );

    params_.getParam( this )->fillPar( pars );

    pars.set( sKeyAlgoName(), getGridderName() );
    if ( gridder_ )
	gridder_->fillPar( pars );

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
    const Stats::UpscaleType samppol = params_.getParam( this )->samppol_;
    params_.getParam( this )->usePar( pars );
    if ( !hassamplepol )
	params_.getParam( this )->samppol_ = samppol;

    pars.get( sKeyLogName(), logname_ );

    wellmids_.erase();
    int nrwells = 0;
    pars.get( sKeyNrWells(), nrwells );
    for ( int idx=0; idx<nrwells; idx++ )
    {
	MultiID mid;
	const BufferString key = IOPar::compKey( sKeyWellLogID(), idx );
	if ( pars.get(key,mid) )
	    wellmids_ += mid;
    }

    float radius = mUdf(float);
    pars.get( InverseDistanceGridder2D::sKeySearchRadius(), radius );
    BufferString nm;
    pars.get( sKeyAlgoName(), nm );
    setGridder( nm.buf(), radius );

    PtrMan<IOPar> lmpar = pars.subselect( sKeyLayerModel() );
    if ( lmpar )
    {
	BufferString lmtype;
	lmpar->get( InterpolationLayerModel::sKeyModelType(), lmtype );
	delete layermodel_;
	layermodel_ = InterpolationLayerModel::factory().create( lmtype );
	if ( !layermodel_ || !layermodel_->usePar(*lmpar) )
	    { deleteAndZeroPtr( layermodel_ ); }
    }

    return true;
}


od_int64 WellLogInterpolator::extraMemoryUsage( OutputSlotID,
	const TrcKeySampling& hsamp, const StepInterval<int>& zsamp ) const
{
    return 0;
}


} // namespace VolProc
