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

bool init()
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
    range_ = wd_->track().dahRange();
    params_.getDahRange( *wd_, range_ );

    return applyFilter();
}

#define cLogStepFact 10

bool applyFilter()
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
			 Stats::UseAvg, logs ),
		     lsnear( d2t, &track, zrg, zintime, extractstep, zintime,
			     Stats::TakeNearest, logs );
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

    return true;
}

void computeLayerModelIntersection(
	const InterpolationLayerModel& layermodel )
{
    intersections_.setSize( layermodel.nrLayers(), mUdf(float) );
    StepInterval<float> dahrg = track_->dahRange();
    dahrg.step = 5;

    const int nrdah = dahrg.nrSteps() + 1;
    for ( int lidx=0; lidx<layermodel.nrLayers(); lidx++ )
    {
	for ( int dahidx=1; dahidx<nrdah; dahidx++ )
	{
	    const float prevdah = dahrg.atIndex( dahidx-1 );
	    const float dah = dahrg.atIndex( dahidx );
	    const Coord3 prevpos = track_->getPos( prevdah );
	    const Coord3 pos = track_->getPos( dah );
	    const float prevlayerz =
		layermodel.getZ( SI().transform(prevpos.getXY()), lidx );
	    const float layerz =
		layermodel.getZ( SI().transform(pos.getXY()), lidx );
	    const float avgz = (prevlayerz+layerz) / 2;
	    if ( avgz>=prevpos.z_ && avgz<pos.z_ )
	    {
		const float slope = (dah-prevdah) / float(pos.z_-prevpos.z_);
		const float calcdah = prevdah + slope*float(avgz-prevpos.z_);
		intersections_[lidx] = calcdah;
		break;
	    }
	}
    }
}


Well::Track*		track_;
ConstRefMan<Well::Log>	log_;
RefMan<Well::Data>	wd_;
TrcKeyZSampling		bbox_;
DBKey			dbky_;
BufferString		logname_;
TypeSet<float>		intersections_;
StepInterval<float>	range_;
Well::ExtractParams	params_;

};


#define mDefWinSz SI().zIsTime() ? log->dahStep(true)*20 : SI().zStep()

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
    delete gridder_; gridder_ = 0;
    delete layermodel_; layermodel_ = 0;
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
	if ( !info->init() )
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

	info->computeLayerModelIntersection( *layermodel_ );
	infos_ += info;
    }

    return true;
}


static TypeSet<float> getMDs( const WellLogInfo& info, float layeridx )
{
    TypeSet<float> mds;
    const float fidx0 = Math::Floor( layeridx );
    const float fidx1 = Math::Ceil( layeridx );
    const int idx0 = mNINT32(fidx0);
    const int idx1 = mNINT32(fidx1);
    if ( !info.intersections_.validIdx(idx0) ||
	 !info.intersections_.validIdx(idx1) )
	return mds;

    const float dah0 = info.intersections_[idx0];
    const float dah1 = info.intersections_[idx1];
    if ( mIsZero(fidx1-fidx0,mDefEps) )
	mds += dah0;
    else
    {
	const float slope = (dah1-dah0) / (fidx1-fidx0);
	mds += dah0 + slope*(layeridx-fidx0);
    }

    return mds;
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
    const int lastzidx = outputarray.info().getSize(2) - 1;

    BinID nearbid = bid;


    PtrMan<Gridder2D> gridder = gridder_->clone();
    const TrcKeySampling& hs = output->sampling().hsamp_;
    const Coord gridpoint( hs.toCoord(nearbid) );

    mAllocVarLenArr(float,vals,lastzidx+1);
    int lasthcidx=-1, firsthcidx=-1;
    for ( int idx=lastzidx; idx>=0; idx-- )
    {
	vals[idx] = mUdf(float);
	const float z = output->sampling().zsamp_.atIndex( idx );
	const float layeridx = layermodel_->getLayerIndex( bid, z );
	if ( mIsUdf(layeridx) ) continue;

	TypeSet<Coord> wellposes;
	TypeSet<float> logvals;

	for ( int idy=0; idy<infos_.size(); idy++ )
	{
	    if ( !infos_.validIdx(idy) ) continue;
	    WellLogInfo* info = infos_[idy];
	    const Well::Log* log = info ? info->log_ : 0;
	    if ( !log ) continue;

	    TypeSet<float> mds = getMDs( *info, layeridx );

	    if ( mds.isEmpty() || mIsUdf(mds[0]) )
		continue;

	    const Coord3 pos = info->track_->getPos( mds[0] );
	    if ( pos.isUdf() )
		continue;

	    if ( !info->range_.isUdf() )
	    {
		if ( !info->range_.includes( mds[0], false ) ) continue;
	    }
	    float lv = Well::LogDataExtracter::calcVal( *log, mds[0],
					    mDefWinSz, params_.samppol_ );


	    if ( mIsUdf(lv) )
		continue;

	    wellposes += pos.getXY();
	    logvals += lv;
	}



	gridder->setPoints( wellposes );
	gridder->setValues( logvals );
	vals[idx] = gridder->getValue( gridpoint );
	if ( extension_==None )
	    continue;

	if ( lasthcidx==-1 )
	{
	    lasthcidx = idx;
	    firsthcidx = idx;
	}
	else if ( idx<firsthcidx )
	    firsthcidx = idx;
    }

    const int outputinlidx = outputinlrg_.nearestIndex( bid.inl() );
    const int outputcrlidx = outputcrlrg_.nearestIndex( bid.crl() );
    const bool useextension = extension_!=None && lasthcidx!=-1;
    for ( int idx=lastzidx; idx>=0; idx-- )
    {
	float val = vals[idx];
	if ( useextension )
	{
	    if ( idx>lasthcidx )
		val = vals[lasthcidx];
	    else if ( idx<firsthcidx )
		val = vals[firsthcidx];
	}

	outputarray.set( outputinlidx, outputcrlidx, idx, val );
    }

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
