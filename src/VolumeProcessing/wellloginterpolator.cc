/*+
 *(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *Author:	Y.C. Liu
 *Date:		April 2007
-*/


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
#include "wellmanager.h"
#include "welltrack.h"
#include "dbman.h"


namespace VolProc
{

static const char* sKeyNrWells()	{ return "Nr of wells"; }
static const char* sKeyWellLogID()	{ return "WellLog ID"; }
static const char* sKeyLogName()	{ return "Log name"; }
static const char* sKeyAlgoName()	{ return "Algorithm"; }
static const char* sKeyExtension()	{ return "Output boundary"; }
static const char* sKeyLogExtension()	{ return "Logs Extension"; }
static const char* sKeyLayerModel()	{ return "Layer Model"; }


class WellLogInfo
{
public:
WellLogInfo( const DBKey& mid, const char* lognm, Well::ExtractParams params )
    : dbky_(mid)
    , logname_(lognm)
    , params_(params)
    , track_(0)
{}

bool init( const InterpolationLayerModel& layermodel, bool extend )
{
    const bool zistime = SI().zIsTime();
    RefMan<Well::Data> wd = Well::MGR().fetchForEdit( dbky_,
				  Well::LoadReqs( Well::Trck, Well::D2T ) );
    if ( !wd )
	return false;

    delete track_;
    track_ = new Well::Track( wd->track() );
    if ( zistime )
	track_->toTime( *wd );

    wd_ = wd;

    return applyFilter( layermodel, extend );
}

#define cLogStepFact 10

bool applyFilter( const InterpolationLayerModel& layermodel, bool extend )
{
    if ( log_ )
	log_->unRef();

    if ( !wd_ )
	return false;

    ConstRefMan<Well::Log> inplog = Well::MGR().getLog( dbky_, logname_ );
    if ( !inplog )
	return false;

    MonitorLock ml( *inplog );

    const Well::Track& track = wd_->track();
    const Well::D2TModel* d2t = wd_->d2TModelPtr();
    const Interval<float> zrg( bbox_.zsamp_ );
    const float extractstep = bbox_.zsamp_.step / cLogStepFact;
    const bool zintime = SI().zIsTime();
    ObjectSet<const Well::Log> logs;
    logs += inplog;
    Well::LogSampler ls( d2t, &track, zrg, zintime, extractstep, zintime,
			 params_.samppol_, logs ),
		     lsnear( d2t, &track, zrg, zintime, extractstep, zintime,
			     Stats::TakeNearest, logs );
    if ( !ls.execute() || !lsnear.execute() )
	return false;

    const BufferString newlognm( inplog->name(), " filtered" );
    const Interval<float> mdrange( inplog->dahRange() );
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

    return createInterpolationFunctions( layermodel, extend, mdrange );
}

bool createInterpolationFunctions( const InterpolationLayerModel& layermodel,
				   bool extend, const Interval<float> mdrg )
{
    logfunc_.setEmpty();
    mdfunc_.setEmpty();

    const TrcKeySampling& hsamp = bbox_.hsamp_;
    Interval<float> range( mdrg );
    if ( !extend )
	params_.getDahRange( *wd_, range );

    Well::LogIter iter( *log_ );
    while ( iter.next() )
    {
	const float md = iter.dah();
	const Coord3 pos = track_->getPos( md );
	const BinID bid = hsamp.toTrcKey( pos.getXY() ).position();
	const float layerz = layermodel.getLayerIndex(bid,mCast(float,pos.z_));
	const float logval = iter.value();
	if ( mIsUdf(layerz) || mIsUdf(logval) ||
		( !extend && !range.includes(md,false) ) )
	    continue;

	logfunc_.add( layerz, logval );
	mdfunc_.add( layerz, md );
    }

    return !logfunc_.isEmpty();
}


Well::Track*		track_;
ConstRefMan<Well::Log>	log_;
RefMan<Well::Data>	wd_;
TrcKeyZSampling		bbox_;
DBKey			dbky_;
BufferString		logname_;
Well::ExtractParams	params_;
PointBasedMathFunction	logfunc_;
PointBasedMathFunction	mdfunc_;

};



WellLogInterpolator::WellLogInterpolator()
    : gridder_(0)
    , extension_(ExtrapolateEdgeValue)
    , extlog_(0)
    , layermodel_(0)
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
    params_.setEmpty();
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


void WellLogInterpolator::setGridder( const char* nm, float radius )
{
    delete gridder_;

    if ( FixedString(nm) == RadialBasisFunctionGridder2D::sFactoryKeyword() )
    {
	gridder_ = new RadialBasisFunctionGridder2D();
	return;
    }

    if ( FixedString(nm) ==  TriangulatedGridder2D::sFactoryKeyword() )
    {
	gridder_ = new TriangulatedGridder2D();
	return;
    }

    if ( FixedString(nm) == InverseDistanceGridder2D::sFactoryKeyword() )
    {
	InverseDistanceGridder2D* invgrid = new InverseDistanceGridder2D();
	gridder_ = invgrid;
	if ( !mIsUdf(radius) )
	    invgrid->setSearchRadius( radius );

	return;
    }
}


const char* WellLogInterpolator::getGridderName() const
{
    if ( gridder_ )
	return gridder_->factoryKeyword();

    return 0;
}


float WellLogInterpolator::getSearchRadius() const
{
    mDynamicCastGet( InverseDistanceGridder2D*, invgrid, gridder_ );
    return invgrid ? invgrid->getSearchRadius() : mUdf( float );
}


bool WellLogInterpolator::is2D() const
{ return false; }


void WellLogInterpolator::setWellData( const DBKeySet& ids, const char* lognm )
{
    wellmids_ = ids;
    logname_ = lognm;
}


void WellLogInterpolator::setWellExtractParams(
					    const Well::ExtractParams& param)
{
    params_ = param;
}


void WellLogInterpolator::getWellNames( BufferStringSet& res ) const
{
    for ( int idx=0; idx<wellmids_.size(); idx++ )
	res.add( DBM().nameOf(wellmids_[idx]) );
}


const char* WellLogInterpolator::getLogName() const
{ return logname_.buf(); }

void WellLogInterpolator::getWellIDs( DBKeySet& ids ) const
{ ids = wellmids_; }


bool WellLogInterpolator::prepareComp( int )
{
    if ( !layermodel_ || !layermodel_->prepare(0) )
	return false;

    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    if ( !output || output->isEmpty() || !gridder_ || is2D() )
	return false;

    const TrcKeySampling& hs = output->sampling().hsamp_;
    outputinlrg_ = hs.inlRange();
    outputcrlrg_ = hs.crlRange();

    for ( int idx=0; idx<wellmids_.size(); idx++ )
    {
	WellLogInfo* info = new WellLogInfo( wellmids_[idx], logname_,
								    params_ );
	if ( !info->init(*layermodel_,extlog_) )
	{
	    if ( info->wd_ )
	    {
		errmsg_ = tr("Cannot load log '%1' for well '%2'")
				.arg( logname_ ).arg( info->wd_->name() );
	    }
	    else
	    {
		errmsg_ = tr("Cannot load log '%1' for well with ID '%2'")
				.arg( logname_ ).arg( wellmids_[idx] );
	    }
	    delete info;
	    deepErase( infos_ );
	    return false;
	}

	infos_ += info;
    }

    return true;
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

    const TrcKeySampling& hs = output->sampling().hsamp_;
    const int outputinlidx = outputinlrg_.nearestIndex( bid.inl() );
    const int outputcrlidx = outputcrlrg_.nearestIndex( bid.crl() );
    const Coord gridpoint( hs.toCoord(bid) );
    Gridder2D* gridder = gridder_->clone();
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
	    const Coord pos = infos_[iwell]->track_->getPos( md ).getXY();
	    const float logval = infos_[iwell]->logfunc_.getValue( layeridx );
	    if ( mIsUdf(md) || mIsUdf(logval) || mIsUdf(pos) )
		continue;

	    points += pos;
	    logvals += logval;
	}

	if ( points.isEmpty() || !gridder->setPoints(points) ||
	     !gridder->setValues(logvals) )
	{
	    outputarray.set( outputinlidx, outputcrlidx, idz, mUdf(float) );
	    continue;
	}

	const float val = gridder->getValue( gridpoint );
	outputarray.set( outputinlidx, outputcrlidx, idz, val );
    }

    delete gridder;

    return true;
}


void WellLogInterpolator::fillPar( IOPar& pars ) const
{
    Step::fillPar( pars );
    params_.fillPar( pars );

    pars.set( sKeyExtension(), extension_ );
    pars.setYN( sKeyLogExtension(), extlog_ );

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
    params_.usePar( pars );

    int extension = 0;
    pars.get( sKeyExtension(), extension );
    extension_ = (ExtensionModel)extension;

    pars.getYN( sKeyLogExtension(), extlog_ );
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

    float radius = 0;;
    pars.get( InverseDistanceGridder2D::sKeySearchRadius(), radius );
    BufferString nm;
    pars.get( sKeyAlgoName(), nm );
    setGridder( nm.buf(), radius );

    delete layermodel_; layermodel_ = 0;
    PtrMan<IOPar> lmpar = pars.subselect( sKeyLayerModel() );
    if ( lmpar )
    {
	BufferString lmtype;
	lmpar->get( InterpolationLayerModel::sKeyModelType(), lmtype );
	layermodel_ = InterpolationLayerModel::factory().create( lmtype );
	if ( !layermodel_ || !layermodel_->usePar(*lmpar) )
	{ delete layermodel_; layermodel_ = 0; }
    }

    if ( layermodel_ )
    {
	const bool yn = extension_==ExtrapolateEdgeValue;
	layermodel_->setZStart( yn ? SI().zRange(false).start : mUdf(float) );
	layermodel_->setZStop( yn ? SI().zRange(false).stop : mUdf(float) );
    }

    return true;
}


od_int64 WellLogInterpolator::extraMemoryUsage( OutputSlotID,
	const TrcKeySampling& hsamp, const StepInterval<int>& zsamp ) const
{
    return 0;
}


const Well::ExtractParams& WellLogInterpolator::getSelParams()
{
    return params_;
}


} // namespace VolProc
