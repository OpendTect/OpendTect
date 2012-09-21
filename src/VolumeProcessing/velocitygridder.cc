/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : October 2006
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "velocitygridder.h"

#include "arraynd.h"
#include "attribdatacubes.h"
#include "binidvalset.h"
#include "gridder2d.h"
#include "iopar.h"
#include "keystrs.h"
#include "progressmeter.h"
#include "survinfo.h"
#include "velocityfunction.h"
#include "velocityfunctiongrid.h"

namespace VolProc
{
class VelGriddingStepTask;


class VelGriddingFromFuncTask : public ParallelTask
{
public:
    			VelGriddingFromFuncTask( VelGriddingStepTask& );
			~VelGriddingFromFuncTask();

    bool		isOK() const;

    od_int64		nrIterations() const;
    bool		doPrepare(int);
    bool		doWork(od_int64,od_int64,int);

    const BinIDValueSet&	completedBids() const { return completedbids_; }

protected:
    ObjectSet<Vel::Function>	velfuncs_;
    Vel::GriddedSource*		velfuncsource_;
    VelGriddingStepTask&	task_;

    Threads::Mutex		lock_;
    BinIDValueSet		completedbids_;
};


class VelGriddingFromVolumeTask : public ParallelTask
{
public:
    			VelGriddingFromVolumeTask( VelGriddingStepTask& );
			~VelGriddingFromVolumeTask();

    od_int64		nrIterations() const;
    bool		doPrepare(int);
    bool		doWork(od_int64,od_int64,int);

    const BinIDValueSet&	completedBids() const { return completedbids_; }

protected:

    Threads::Mutex		lock_;
    BinIDValueSet		completedbids_;

    ObjectSet<Gridder2D>	gridders_;
    VelGriddingStepTask&	task_;
};


class VelGriddingStepTask : public SequentialTask
{
public:
    				VelGriddingStepTask(VelGriddingStep&);
    int				nextStep();

    BinID			getNextBid();
    bool			report1Done();
    				//!<Returns false if process should continue
    od_int64			nrDone() const;
    od_int64			totalNr() const       { return totalnr_; }
    const char*			nrDoneText() const    { return "CDPs gridded"; }

    VelGriddingStep&		getStep()	      { return step_; }
    const BinIDValueSet&	remainingBids() const { return remainingbids_; }
    const BinIDValueSet&	definedBids() const   { return definedbids_; }
    const TypeSet<Coord>&	definedPts() const    { return definedpts_; }
    const TypeSet<BinIDValueSet::Pos>& definedPos() const { return definedpos_;}

protected:
    int				nrdone_;
    mutable Threads::Mutex	lock_;

    BinIDValueSet::Pos		curpos_;

    BinIDValueSet		remainingbids_;
    BinIDValueSet		definedbids_;

    TypeSet<Coord>		definedpts_;
    TypeSet<BinIDValueSet::Pos>	definedpos_;

    VelGriddingStep&		step_;
    od_int64			totalnr_;
};


VelGriddingStepTask::VelGriddingStepTask( VelGriddingStep& step )
    : nrdone_( 0 )
    , remainingbids_( 0, false )
    , definedbids_( 0, false )
    , step_( step )
{
    const Attrib::DataCubes* output = step_.getOutput();
    const HorSampling hrg = output->cubeSampling().hrg;

    HorSamplingIterator iterator( hrg );
    BinID bid;
    while ( iterator.next( bid ) )
	remainingbids_.add( bid );

    totalnr_ = hrg.totalNr();
}


bool VelGriddingStepTask::report1Done()
{
    if ( progressmeter_ )
	++(*progressmeter_);

    Threads::MutexLocker lock( lock_ );
    nrdone_++;

    return shouldContinue();
}


od_int64 VelGriddingStepTask::nrDone() const
{
    Threads::MutexLocker lock( lock_ );
    return nrdone_;
}


BinID VelGriddingStepTask::getNextBid()
{
    Threads::MutexLocker lock( lock_ );
    if ( !remainingbids_.next(curpos_) )
    {
	pErrMsg("Too many bids requested");
	return BinID(-1,-1);
    }

    return remainingbids_.getBinID(curpos_);
}


int VelGriddingStepTask::nextStep()
{
    curpos_ = BinIDValueSet::Pos(-1,-1);

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

	BinIDValueSet::Pos pos;
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



VelGriddingFromFuncTask::VelGriddingFromFuncTask( VelGriddingStepTask& task )
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
    Attrib::DataCubes* output = task_.getStep().getOutput();
    const int zsz = output->getZSz();
    const SamplingData<float> zsd((float) output->zstep_*output->z0_,(float) output->zstep_);

    Vel::Function* func = velfuncs_[thread];
    const StepInterval<float> zrg( zsd.start, zsd.atIndex(zsz-1), zsd.step );
    func->setDesiredZRange( zrg );

    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const BinID bid = task_.getNextBid();

	if ( !func->moveTo( bid ) )
	    continue;

	const int inlidx = output->inlsampling_.nearestIndex( bid.inl );
	const int crlidx = output->crlsampling_.nearestIndex( bid.crl );

	for ( int idy=0; idy<zsz; idy++ )
	{
	    const float z = (float) ((output->z0_+idy) * output->zstep_);
	    const float vel = func->getVelocity( z );

	    output->setValue( 0, inlidx, crlidx, idy, vel );
	}

	Threads::MutexLocker lock( lock_ );
	completedbids_.add( bid );

	if ( !task_.report1Done() )
	    break;
    }

    return true;
}



VelGriddingFromVolumeTask::VelGriddingFromVolumeTask(VelGriddingStepTask& task )
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

	//gridder->setGridArea(

	gridders_ += gridder;
    }

    return true;
}


bool VelGriddingFromVolumeTask::doWork( od_int64 start, od_int64 stop,
					int thread )
{
    Attrib::DataCubes& output = *task_.getStep().getOutput();
    Array3D<float>& array = output.getCube(0);
    ValueSeries<float>* storage = array.getStorage();
    if ( !storage )
		return false;

    const int zsz = output.getZSz();
    Gridder2D* gridder = gridders_[thread];
    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const BinID bid = task_.getNextBid();
	const Coord coord = SI().transform( bid );
	if ( !gridder->setGridPoint( coord ) )
	    continue;

	if ( !gridder->init() )
	    continue;

	const TypeSet<int>& usedvals = gridder->usedValues();
	const TypeSet<float>& weights = gridder->weights();

	if ( !usedvals.size() )
	    continue;

	ObjectSet<const float> sourceptrs;
	TypeSet<od_int64> srcoffsets;
	for ( int idy=0; idy<usedvals.size(); idy++ )
	{
	    const BinID sourcebid =
		task_.definedBids().getBinID(task_.definedPos()[usedvals[idy]]);

	    const int inlidx =
		output.inlsampling_.nearestIndex( sourcebid.inl );
	    const int crlidx =
		output.crlsampling_.nearestIndex( sourcebid.crl );

	    const od_int64 offset = array.info().getOffset( inlidx, crlidx, 0 );

	    if ( storage->arr() )
		sourceptrs += storage->arr() + offset;
	    else
		srcoffsets += offset;
	}

	const int inlidx = output.inlsampling_.nearestIndex( bid.inl );
	const int crlidx = output.crlsampling_.nearestIndex( bid.crl );
	const od_int64 targetoffset = array.info().getOffset(inlidx,crlidx,0);
	float* dstptr = output.getCube(0).getData();

	for ( od_int64 idy=0; idy<zsz; idy++ )
	{
	    int nrvals = 0;
	    float wsum = 0;
	    float sum = 0;
	    for ( int idz=weights.size()-1; idz>=0; idz-- )
	    {
		const float val = dstptr
		    ? sourceptrs[idz][idy]
		    : storage->value( idy+srcoffsets[idz] );
		if ( mIsUdf(val) )
		    continue;

		sum += val*weights[idz];
		nrvals ++;
		wsum += weights[idz];
	    }

	    const float val = !nrvals || mIsZero(wsum,1e-5)
		? mUdf(float)
		: sum/wsum;

	    if ( dstptr )
		dstptr[idy+targetoffset] = val;
	    else
		storage->setValue( idy+targetoffset, val );
	}

	Threads::MutexLocker lock( lock_ );
	completedbids_.add( bid );

	if ( !task_.report1Done() )
	    break;
    }

    return true;
}


VelGriddingStep::VelGriddingStep()
    : gridder_( 0 )
{ }


VelGriddingStep::~VelGriddingStep()
{
    releaseData();
}


void VelGriddingStep::releaseData()
{
    Step::releaseData();
    deepUnRef( sources_ );
    delete gridder_;
    gridder_ = 0;
}


void VelGriddingStep::setSources( ObjectSet<Vel::FunctionSource>& nvfs )
{
    deepUnRef( sources_ );
    sources_ = nvfs;
    deepRef( sources_ );
}


void VelGriddingStep::setGridder( Gridder2D* gridder )
{
    delete gridder_;
    gridder_ = gridder;
}


const Gridder2D* VelGriddingStep::getGridder() const
{ return gridder_; }


const ObjectSet<Vel::FunctionSource>& VelGriddingStep::getSources() const
{ return sources_; }


bool VelGriddingStep::needsInput() const
{ return false; }


const VelocityDesc* VelGriddingStep::getVelDesc() const
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



Task* VelGriddingStep::createTask()
{
    if ( !gridder_ )
	return 0;

    return new VelGriddingStepTask( *this );
}


void VelGriddingStep::fillPar( IOPar& par ) const
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
}


bool VelGriddingStep::usePar( const IOPar& par )
{
    if ( !Step::usePar( par ) )
	return false;

    int nrsources;
    if ( !par.get( sKeyNrSources(), nrsources ) )
	return false;

    deepUnRef( sources_ );

    for ( int idx=0; idx<nrsources; idx++ )
    {
	const char* parseerror = "Parse error";

	const BufferString idxstr( 0, idx, 0 );
	PtrMan<IOPar> sourcepar = par.subselect( idxstr.buf() );
	if ( !sourcepar )
	{
	    errmsg_ = parseerror;
	    return false;
	}

	BufferString sourcetype;
	if ( !sourcepar->get( sKeyType(), sourcetype ) )
	{
	    errmsg_ = parseerror;
	    return false;
	}

	MultiID mid;
	if ( !sourcepar->get( sKeyID(), mid ) )
	{
	    errmsg_ = parseerror;
	    return false;
	}

	Vel::FunctionSource* source =
	    Vel::FunctionSource::factory().create( sourcetype.buf(), mid );
	if ( !source )
	{
	    errmsg_ = "Cannot create a velocity source of type ";
	    errmsg_.add( sourcetype.buf() ).add( ". " )
	    	   .add( Vel::FunctionSource::factory().errMsg() );
	    return false;
	}

	source->ref();

	if ( !source->usePar( *sourcepar ) )
	{
	    errmsg_ = "Cannot parse velocity source's paramters (";
	    errmsg_ += sourcetype.buf();
	    errmsg_ += " ).";

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
	
	if ( !gridder->usePar( *velgridpar ) )
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

    return true;
}


}; //Namespace
