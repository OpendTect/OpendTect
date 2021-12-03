/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : October 2006
-*/


#include "velocitygridder.h"

#include "arrayndimpl.h"
#include "binidvalset.h"
#include "gridder2d.h"
#include "interpollayermodel.h"
#include "iopar.h"
#include "keystrs.h"
#include "paralleltask.h"
#include "progressmeter.h"
#include "seisdatapack.h"
#include "survinfo.h"
#include "task.h"
#include "velocityfunction.h"
#include "velocityfunctiongrid.h"
#include "uistrings.h"


namespace VolProc
{

class VelGriddingTask;


class VelGriddingFromFuncTask : public ParallelTask
{
public:
				VelGriddingFromFuncTask(VelGriddingTask&);
				~VelGriddingFromFuncTask();

    bool			isOK() const;

    od_int64			nrIterations() const;
    bool			doPrepare(int);
    bool			doWork(od_int64,od_int64,int);

    const BinIDValueSet&	completedBids() const { return completedbids_; }

protected:
    ObjectSet<Vel::Function>	velfuncs_;
    Vel::GriddedSource*		velfuncsource_;
    VelGriddingTask&		task_;

    Threads::Mutex		lock_;
    BinIDValueSet		completedbids_;
};


class VelGriddingFromVolumeTask : public ParallelTask
{
public:
				VelGriddingFromVolumeTask(VelGriddingTask&);
				~VelGriddingFromVolumeTask();

    od_int64			nrIterations() const;
    bool			doPrepare(int);
    bool			doWork(od_int64,od_int64,int);

    const BinIDValueSet&	completedBids() const { return completedbids_; }

protected:

    Threads::Mutex		lock_;
    BinIDValueSet		completedbids_;

    ObjectSet<Gridder2D>	gridders_;
    VelGriddingTask&		task_;
};


class VelGriddingTask : public SequentialTask
{ mODTextTranslationClass(VelGriddingTask);
public:
				VelGriddingTask(VelocityGridder&);
    int				nextStep();

    BinID			getNextBid();
    bool			report1Done();
				//!<Returns false if process should continue
    od_int64			nrDone() const;
    od_int64			totalNr() const       { return totalnr_; }
    uiString			uiMessage() const;
    uiString			uiNrDoneText() const
						{ return tr("CDPs gridded"); }

    VelocityGridder&		getStep()	      { return step_; }
    const BinIDValueSet&	remainingBids() const { return remainingbids_; }
    const BinIDValueSet&	definedBids() const   { return definedbids_; }
    const TypeSet<Coord>&	definedPts() const    { return definedpts_; }
    const TypeSet<BinIDValueSet::SPos>& definedPos() const
							{ return definedpos_;}

protected:
    int				nrdone_;
    mutable Threads::Mutex	lock_;

    BinIDValueSet::SPos		curpos_;

    BinIDValueSet		remainingbids_;
    BinIDValueSet		definedbids_;

    TypeSet<Coord>		definedpts_;
    TypeSet<BinIDValueSet::SPos> definedpos_;

    VelocityGridder&		step_;
    od_int64			totalnr_;
};


// VelGriddingTask
VelGriddingTask::VelGriddingTask( VelocityGridder& step )
    : SequentialTask("Velocity Gridding Task")
    , nrdone_( 0 )
    , remainingbids_( 0, false )
    , definedbids_( 0, false )
    , step_( step )
{
    const RegularSeisDataPack* output = step_.getOutput();
    const TrcKeySampling hrg = output->sampling().hsamp_;

    TrcKeySamplingIterator iterator( hrg );
    BinID bid;
    while ( iterator.next( bid ) )
	remainingbids_.add( bid );

    totalnr_ = hrg.totalNr();
}


uiString VelGriddingTask::uiMessage() const
{
    return tr("Gridding velocities");
}


bool VelGriddingTask::report1Done()
{
    incrementProgress();
    Threads::MutexLocker lock( lock_ );
    nrdone_++;

    return shouldContinue();
}


od_int64 VelGriddingTask::nrDone() const
{
    Threads::MutexLocker lock( lock_ );
    return nrdone_;
}


BinID VelGriddingTask::getNextBid()
{
    Threads::MutexLocker lock( lock_ );
    if ( !remainingbids_.next(curpos_) )
    {
	pErrMsg("Too many bids requested");
	return BinID(-1,-1);
    }

    return remainingbids_.getBinID(curpos_);
}


int VelGriddingTask::nextStep()
{
    curpos_ = BinIDValueSet::SPos(-1,-1);

    bool change = false;

    if ( !nrdone_ )
    {
	VelGriddingFromFuncTask task( *this );
	if ( !task.execute() )
	    return ErrorOccurred();

	definedbids_.append( task.completedBids() );
	remainingbids_.remove( task.completedBids() );

	change = !task.completedBids().isEmpty();
    }
    else
    {
	definedpts_.erase();
	definedpos_.erase();

	BinIDValueSet::SPos pos;
	while ( definedbids_.next( pos, true ) )
	{
	    definedpts_ += SI().transform( definedbids_.getBinID(pos) );
	    definedpos_ += pos;
	}

	VelGriddingFromVolumeTask task( *this );
	if ( !task.execute() )
	    return ErrorOccurred();

	definedbids_.append( task.completedBids() );
	remainingbids_.remove( task.completedBids() );

	change = !task.completedBids().isEmpty();
    }

    return !change || remainingbids_.isEmpty() ? Finished() : MoreToDo();
}


// VelGriddingFromFuncTask
VelGriddingFromFuncTask::VelGriddingFromFuncTask( VelGriddingTask& task )
    : task_( task )
    , velfuncsource_( 0 )
    , completedbids_( 0, false )
{
    mTryAlloc( velfuncsource_, Vel::GriddedSource );
    if ( velfuncsource_ )
    {
	velfuncsource_->ref();
	velfuncsource_->setSource( const_cast<ObjectSet<Vel::FunctionSource>&>(
		    task_.getStep().getSources()) );
	velfuncsource_->setGridder( task_.getStep().getGridder()->clone() );
	velfuncsource_->setLayerModel( task_.getStep().getLayerModel() );
    }
}


VelGriddingFromFuncTask::~VelGriddingFromFuncTask()
{
    if ( velfuncsource_ )
	velfuncsource_->unRef();
   deepUnRef( velfuncs_ );
}


bool VelGriddingFromFuncTask::isOK() const
{
    return velfuncsource_ && velfuncsource_->getSources().size();
}


bool VelGriddingFromFuncTask::doPrepare( int nrthreads )
{
    if ( !isOK() || !nrthreads )
	return false;

   deepUnRef( velfuncs_ );

   for ( int idx=0; idx<nrthreads; idx++ )
   {
       RefMan<Vel::Function> velfunc = velfuncsource_->createFunction();
       velfunc->ref();
       velfuncs_ += velfunc;
   }

   return true;
}


od_int64 VelGriddingFromFuncTask::nrIterations() const
{
    return task_.remainingBids().totalSize();
}


bool VelGriddingFromFuncTask::doWork( od_int64 start, od_int64 stop,
				      int thread )
{
    RegularSeisDataPack* output = task_.getStep().getOutput();
    if ( !output || output->isEmpty() )
	return false;

    Vel::Function* func = velfuncs_[thread];
    const TrcKeySampling& hs = output->sampling().hsamp_;
    const StepInterval<float>& zrg( output->sampling().zsamp_ );
    func->setDesiredZRange( zrg );
    const int zsz = output->sampling().nrZ();

    for ( int idx=mCast(int,start); idx<=stop && shouldContinue(); idx++ )
    {
	const BinID bid = task_.getNextBid();

	if ( !func->moveTo(bid) )
	    continue;

	const int inlidx = hs.inlRange().nearestIndex( bid.inl() );
	const int crlidx = hs.crlRange().nearestIndex( bid.crl() );

	for ( int zidx=0; zidx<zsz; zidx++ )
	{
	    const float z = zrg.atIndex( zidx );
	    const float vel = func->getVelocity( z );

	    output->data(0).set( inlidx, crlidx, zidx, vel );
	}

	Threads::MutexLocker lock( lock_ );
	completedbids_.add( bid );

	if ( !task_.report1Done() )
	    break;
    }

    return true;
}


// VelGriddingFromVolumeTask
VelGriddingFromVolumeTask::VelGriddingFromVolumeTask(VelGriddingTask& task )
    : task_( task )
    , completedbids_( 0, false )
{}


VelGriddingFromVolumeTask::~VelGriddingFromVolumeTask()
{
    deepErase( gridders_ );
}


od_int64 VelGriddingFromVolumeTask::nrIterations() const
{ return task_.remainingBids().totalSize(); }


bool VelGriddingFromVolumeTask::doPrepare( int nrthreads )
{
    for ( int idx=0; idx<nrthreads; idx++ )
    {
	Gridder2D* gridder = task_.getStep().getGridder()->clone();
	if ( !gridder->setPoints( task_.definedPts() ) )
	{
	    delete gridder;
	    return false;
	}

	gridders_ += gridder;
    }

    return true;
}


bool VelGriddingFromVolumeTask::doWork( od_int64 start, od_int64 stop,
					int thread )
{
    RegularSeisDataPack* output = task_.getStep().getOutput();
    if ( !output || output->isEmpty() )
	return false;

    Array3D<float>& array = output->data(0);
    ValueSeries<float>* storage = array.getStorage();
    if ( !storage )
	return false;

    const TrcKeySampling& hs = output->sampling().hsamp_;
    const int zsz = output->sampling().nrZ();

    Gridder2D* gridder = gridders_[thread];
    for ( int idx=mCast(int,start); idx<=stop && shouldContinue(); idx++ )
    {
	const BinID bid = task_.getNextBid();
	const Coord pos( hs.toCoord(bid) );
	TypeSet<double> weights;
	TypeSet<int> usedvals;
	if ( !gridder->getWeights(pos,weights,usedvals) )
	    continue;

	ObjectSet<const float> sourceptrs;
	TypeSet<od_int64> srcoffsets;
	for ( int idy=0; idy<usedvals.size(); idy++ )
	{
	    const BinID sourcebid =
		task_.definedBids().getBinID(task_.definedPos()[usedvals[idy]]);

	    const int inlidx =
		hs.inlRange().nearestIndex( sourcebid.inl() );
	    const int crlidx =
		hs.crlRange().nearestIndex( sourcebid.crl() );

	    const od_int64 offset = array.info().getOffset( inlidx, crlidx, 0 );

	    if ( storage->arr() )
		sourceptrs += storage->arr() + offset;
	    else
		srcoffsets += offset;
	}

	const int inlidx = hs.inlRange().nearestIndex( bid.inl() );
	const int crlidx = hs.crlRange().nearestIndex( bid.crl() );
	const od_int64 targetoffset = array.info().getOffset(inlidx,crlidx,0);
	float* dstptr = output->data(0).getData();

	for ( od_int64 zidx=0; zidx<zsz; zidx++ )
	{
	    int nrvals = 0;
	    double wsum = 0;
	    double sum = 0;
	    for ( int idz=weights.size()-1; idz>=0; idz-- )
	    {
		const float val = dstptr
		    ? sourceptrs[idz][zidx]
		    : storage->value( zidx+srcoffsets[idz] );
		if ( mIsUdf(val) )
		    continue;

		sum += mCast(double,val) * weights[idz];
		nrvals ++;
		wsum += weights[idz];
	    }

	    const float val = !nrvals || mIsZero(wsum,1e-5)
		? mUdf(float)
		: mCast(float,sum/wsum);

	    if ( dstptr )
		dstptr[zidx+targetoffset] = val;
	    else
		storage->setValue( zidx+targetoffset, val );
	}

	Threads::MutexLocker lock( lock_ );
	completedbids_.add( bid );

	if ( !task_.report1Done() )
	    break;
    }

    return true;
}


// VelocityGridder
static const char* sKeyLayerModel()	{ return "Layer Model"; }

VelocityGridder::VelocityGridder()
    : gridder_(0)
    , layermodel_(0)
{}


VelocityGridder::~VelocityGridder()
{
    releaseData();
}


void VelocityGridder::releaseData()
{
    Step::releaseData();
    deepUnRef( sources_ );
    delete gridder_; gridder_ = 0;
    delete layermodel_; layermodel_ = 0;
}


void VelocityGridder::setSources( ObjectSet<Vel::FunctionSource>& nvfs )
{
    deepUnRef( sources_ );
    sources_ = nvfs;
    deepRef( sources_ );
}


const ObjectSet<Vel::FunctionSource>& VelocityGridder::getSources() const
{ return sources_; }


void VelocityGridder::setGridder( Gridder2D* gridder )
{
    delete gridder_;
    gridder_ = gridder;
}


const Gridder2D* VelocityGridder::getGridder() const
{ return gridder_; }


void VelocityGridder::setLayerModel( InterpolationLayerModel* mdl )
{
    delete layermodel_;
    layermodel_ = mdl;
}


const InterpolationLayerModel* VelocityGridder::getLayerModel() const
{ return layermodel_; }


bool VelocityGridder::needsInput() const
{ return false; }


const VelocityDesc* VelocityGridder::getVelDesc() const
{
    const int nrsources = sources_.size();
    if ( !nrsources )
	return 0;

    const VelocityDesc& res = sources_[0]->getDesc();

    for ( int idx=1; idx<nrsources; idx++ )
    {
	if ( res!=sources_[idx]->getDesc() )
	    return 0;
    }

    return &res;
}

#define cMaxIsolatedFunctions	100

Task* VelocityGridder::createTask()
{
    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    if ( !output || output->isEmpty() || !gridder_ ) return 0;

    const TrcKeyZSampling& tkzs = output->sampling();
    if ( !layermodel_ || !layermodel_->prepare(tkzs) )
	return 0;

    BinIDValueSet valset( 1, true );
    const OD::GeomSystem gs = tkzs.hsamp_.survid_;
    TrcKeySampling hsamp;
    Pos::IdxPairValueSet::SPos pos;
    const ObjectSet<Vel::FunctionSource>& sources = getSources();
    TypeSet<TrcKeySampling> tkss;
    for ( int idx=0; idx<sources.size(); idx++ )
    {
	if ( !sources[idx] )
	    continue;

	sources[idx]->getAvailablePositions( valset );
	pos.reset();
	while( valset.next(pos,true) )
	{
	    hsamp.init( false );
	    hsamp.survid_ = gs;
	    hsamp.include( TrcKey(gs,valset.getIdxPair(pos)) );
	    tkss += hsamp;
	}
    }

    const int nrfunctions = tkss.size();
    if ( nrfunctions < cMaxIsolatedFunctions )
    {
	for ( int idx=0; idx<nrfunctions; idx++ )
	    layermodel_->addSampling( tkss[idx] );	//Slow for HC!
    }
    else
    {
	hsamp.init( false );
	hsamp.survid_ = gs;
	for ( int idx=0; idx<nrfunctions; idx++ )
	    hsamp.include( tkss[idx] );

	layermodel_->addSampling( hsamp );
    }

    return new VelGriddingTask( *this );
}


void VelocityGridder::fillPar( IOPar& par ) const
{
    Step::fillPar( par );

    par.set( sKeyNrSources(), sources_.size() );
    for ( int idx=0; idx<sources_.size(); idx++ )
    {
	IOPar sourcepar;
	sourcepar.set( sKeyType(), sources_[idx]->factoryKeyword() );
	sourcepar.set( sKeyID(), sources_[idx]->multiID() );
	sources_[idx]->fillPar(sourcepar);

	const BufferString idxstr( 0, idx, 0 );
	par.mergeComp( sourcepar, idxstr.buf() );
    }

    if ( gridder_ )
    {
	IOPar gridpar;
	gridder_->fillPar( gridpar );
	gridpar.set( sKey::Name(), gridder_->factoryKeyword() );
	par.mergeComp( gridpar, sKeyGridder() );
    }

    if ( layermodel_ )
    {
	IOPar lmpar;
	layermodel_->fillPar( lmpar );
	par.mergeComp( lmpar, sKeyLayerModel() );
    }
}


bool VelocityGridder::usePar( const IOPar& par )
{
    if ( !Step::usePar( par ) )
	return false;

    int nrsources;
    if ( !par.get( sKeyNrSources(), nrsources ) )
	return false;

    deepUnRef( sources_ );

    for ( int idx=0; idx<nrsources; idx++ )
    {
	const uiString parseerror = tr("Parse error");

	const BufferString idxstr( 0, idx, 0 );
	PtrMan<IOPar> sourcepar = par.subselect( idxstr.buf() );
	if ( !sourcepar )
	{
	    errmsg_ = parseerror;
	    return false;
	}

	BufferString sourcetype;
	if ( !sourcepar->get(sKeyType(),sourcetype) )
	{
	    errmsg_ = parseerror;
	    return false;
	}

	MultiID mid;
	if ( !sourcepar->get(sKeyID(),mid) )
	{
	    errmsg_ = parseerror;
	    return false;
	}

	Vel::FunctionSource* source =
	    Vel::FunctionSource::factory().create( sourcetype.buf(), mid );
	if ( !source )
	{
	    errmsg_ =
		uiStrings::phrCannotCreate(tr("velocity source of type %1. %2")
			.arg( sourcetype.buf() )
			.arg( Vel::FunctionSource::factory().errMsg() ) );
	    return false;
	}

	source->ref();

	if ( !source->usePar(*sourcepar) )
	{
	    errmsg_ = tr("Cannot parse velocity source's paramters (%1).")
			.arg( sourcetype.buf() );

	    source->unRef();
	    return false;
	}

	sources_ += source;
    }

    Gridder2D* gridder = 0;

    PtrMan<IOPar> velgridpar = par.subselect( sKeyGridder() );
    if ( velgridpar )
    {
	BufferString nm;
	velgridpar->get( sKey::Name(), nm );
	gridder = Gridder2D::factory().create( nm.buf() );
	if ( !gridder )
	    return false;

	if ( !gridder->usePar(*velgridpar) )
	{
	    delete gridder;
	    return false;
	}
    }
    else //Old format
    {
	float searchradius = mUdf(float);
	par.get( "Searchradius", searchradius );
	InverseDistanceGridder2D* newgridder = new InverseDistanceGridder2D;
	newgridder->setSearchRadius( searchradius );
	gridder = newgridder;
    }

    setGridder( gridder );

    PtrMan<IOPar> lmpar = par.subselect( sKeyLayerModel() );
    BufferString lmtype( ZSliceInterpolationModel::sFactoryKeyword() );
    if ( lmpar )
	lmpar->get( InterpolationLayerModel::sKeyModelType(), lmtype );

    delete layermodel_;
    layermodel_ = InterpolationLayerModel::factory().create( lmtype );
    if ( !layermodel_ || (lmpar && !layermodel_->usePar(*lmpar)) )
	deleteAndZeroPtr( layermodel_ );

    return true;
}


od_int64 VelocityGridder::extraMemoryUsage( OutputSlotID,
	const TrcKeySampling& hsamp, const StepInterval<int>& ) const
{
    return layermodel_ ? layermodel_->getMemoryUsage( hsamp ) : 0;
}


} // namespace VolProc
