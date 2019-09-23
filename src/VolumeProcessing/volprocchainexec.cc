/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : October 2006
-*/


#include "volprocchainexec.h"

#include "cubedata.h"
#include "odsysmem.h"
#include "horsubsel.h"
#include "seisdatapack.h"
#include "trckeyzsampling.h"
#include "threadwork.h"

uiString VolProc::ChainExecutor::sGetStepErrMsg()
{
    return uiStrings::phrCannotFind( tr("output step with id: %1") );
}


VolProc::ChainExecutor::ChainExecutor( Chain& vr )
    : ::Executor( "Volume processing" )
    , chain_( vr )
    , isok_( false )
    , outputdp_( 0 )
    , totalnrepochs_( 1 )
    , curepoch_( 0 )
{
    setName( toString(vr.name()) );
    web_ = chain_.getWeb();
    //TODO Optimize connections, check for indentical steps using same inputs
}


VolProc::ChainExecutor::~ChainExecutor()
{
    deepErase( tkzss_ );
    deepErase( epochs_ );
    if ( curepoch_ )
	delete curepoch_;
}


uiString VolProc::ChainExecutor::errMsg() const
{ return errmsg_; }


bool VolProc::ChainExecutor::setCalculationScope( const TrcKeySampling& tks,
						  const ZSampling& zsamp,
					od_int64& maxmemusage, int* nrchunks )
{
    isok_ = false;
    if ( !scheduleWork() )
	return isok_;

    const int nrchunksret = nrChunks( tks, zsamp, maxmemusage );
    if ( nrchunks )
	*nrchunks = nrchunksret;

    isok_ = nrchunksret > 0;

    return isok_;
}


void VolProc::ChainExecutor::adjustStepsNrComponents( Pos::GeomID geomid ) const
{
    for ( int iepoch=epochs_.size()-1; iepoch>=0; iepoch-- )
    {
	const ObjectSet<Step>& steps = epochs_[iepoch]->getSteps();
	for ( int istep=0; istep<steps.size(); istep++ )
	{
	    VolProc::Step* step = const_cast<VolProc::Step*>( steps[istep] );
	    const BufferString stepnm( step->factoryKeyword() );
	    TypeSet<VolProc::Chain::Connection> conns;
	    web_.getConnections( step->getID(), false, conns );
	    for ( int iconn=0; iconn<conns.size(); iconn++ )
	    {
		const VolProc::Step::OutputSlotID outslot =
						conns[iconn].outputslotid_;
		const int nroutcomps = step->getNrOutComponents(outslot,geomid);
		Step* inpstep =
			    chain_.getStepFromID( conns[iconn].inputstepid_ );
		inpstep->setInputNrComps( conns[iconn].inputslotid_,
					  nroutcomps );
	    }
	}
    }
}


#define mGetStep( var, id, errret ) \
Step* var = chain_.getStepFromID( id ); \
if ( !var ) \
{ \
    errmsg_ = sGetStepErrMsg().arg(toString(id)); \
    return errret; \
}


bool VolProc::ChainExecutor::scheduleWork()
{
    deepErase( epochs_ );
    scheduledsteps_.erase();

    ObjectSet<Step> unscheduledsteps;

    //This is the output step of the whole chain,
    //but the input step of the connection since in receives data
    mGetStep( chainoutputstep, chain_.outputstepid_, false );

    unscheduledsteps += chainoutputstep;

    int firstepoch = 0;
    while ( unscheduledsteps.size() )
    {
	Step* currentstep = unscheduledsteps.pop(); //an inputstep
	if ( !scheduledsteps_.addIfNew( currentstep ) )
	    continue;

	const int lateststepepoch = calculateLatestEpoch( currentstep->getID());
	if ( mIsUdf(lateststepepoch) )
	{
	    pErrMsg("Cannot calculate latest epoch");
	    return false;
	}

	firstepoch = mMAX( firstepoch, lateststepepoch );

	TypeSet<Chain::Connection> inputconnections;
	web_.getConnections( currentstep->getID(), true, inputconnections );
	for ( int idx=0; idx<inputconnections.size(); idx++ )
	{
	    mGetStep( outputstep, inputconnections[idx].outputstepid_, false );
	    if ( scheduledsteps_.isPresent( outputstep ) ||
		 unscheduledsteps.isPresent( outputstep ) )
		continue;

	    unscheduledsteps += outputstep;
	}
    }

    const int nrepochs = firstepoch+1;
    for ( int idx=0; idx<nrepochs; idx++ )
    {
	Epoch* epoch = new Epoch( *this );
	for ( int idy=0; idy<scheduledsteps_.size(); idy++ )
	    if ( calculateLatestEpoch( scheduledsteps_[idy]->getID() )==idx )
		epoch->addStep( scheduledsteps_[idy] );

	epochs_ += epoch;
    }

    totalnrepochs_ = epochs_.size();

    return true;
}


void VolProc::ChainExecutor::updateScheduledStepsSampling(
						    const TrcKeySampling& tks,
						    const ZSampling& zsamp )
{
    deepErase( tkzss_ );
    for ( int idx=0; idx<chain_.getSteps().size(); idx++ )
    {
	tkzss_ += new TrcKeyZSampling;
	tkzss_[idx]->setEmpty();
    }

    const int laststepidx = chain_.indexOf(
			    chain_.getStepFromID( chain_.outputstepid_ ) );
    tkzss_[laststepidx]->hsamp_ = tks;
    tkzss_[laststepidx]->zsamp_ = zsamp;
    for ( int iepoch=0; iepoch<totalnrepochs_; iepoch++ )
    {
	const ObjectSet<Step>& steps = epochs_[iepoch]->getSteps();
	for ( int istep=0; istep<steps.size(); istep++ )
	{
	    const Step* outstep = steps[istep];
	    const int outstepidx = chain_.indexOf( outstep );
	    const TrcKeyZSampling& outtkzs = *tkzss_[outstepidx];
	    TypeSet<Chain::Connection> conns;
	    web_.getConnections( outstep->getID(), true, conns );
	    for ( int iconn=0; iconn<conns.size(); iconn++ )
	    {
		const Step::ID inpstepid = conns[iconn].outputstepid_;
		const Step* inpstep = chain_.getStepFromID( inpstepid );
		const int inpstepidx = chain_.indexOf( inpstep );
		const TrcKeyZSampling tkzs =
				      outstep->getInputSampling( outtkzs );
		if ( tkzss_[inpstepidx]->hsamp_.isDefined() )
		    tkzss_[inpstepidx]->include( tkzs );
		else
		    *tkzss_[inpstepidx] = tkzs;
	    }
	}
    }
}


int VolProc::ChainExecutor::calculateLatestEpoch( Step::ID stepid ) const
{
    if ( stepid==chain_.outputstepid_ )
	return 0;

    TypeSet<Chain::Connection> outputconnections;
    web_.getConnections( stepid, false, outputconnections );

    int latestepoch = mUdf(int);
    for ( int idx=0; idx<outputconnections.size(); idx++ )
    {
	const int curepoch =
	    calculateLatestEpoch( outputconnections[idx].inputstepid_ );
	if ( mIsUdf(curepoch) )
	    continue;

	const int newepoch = curepoch+1;
	if ( mIsUdf(latestepoch) || newepoch>latestepoch )
	    latestepoch = newepoch;
    }

    return latestepoch;
}


bool VolProc::ChainExecutor::getCalculationScope( Step::ID stepid,
						  TrcKeyZSampling& tkzs ) const
{
    const int stepidx = chain_.indexOf( chain_.getStepFromID( stepid ) );
    if ( !tkzss_.validIdx(stepidx) )
	return false;

    tkzs = *tkzss_[stepidx];

    return true;
}


namespace VolProc
{

struct VolumeMemory
{
    Step::ID		creator_;
    Step::OutputSlotID	outputslot_;
    od_int64		nrbytes_;
    int			epoch_;

			VolumeMemory( Step::ID creator,
				      Step::OutputSlotID outputslot,
				      od_int64 nrbytes,
				      int epoch )
			: creator_(creator)
			, outputslot_(outputslot)
			, nrbytes_(nrbytes)
			, epoch_(epoch)				{}

    bool		operator==( VolumeMemory vm ) const
			{
			    return creator_ == vm.creator_ &&
				   outputslot_ == vm.outputslot_ &&
				   nrbytes_ == vm.nrbytes_ &&
				   epoch_ == vm.epoch_;
			}
};

} // namespace VolProc

// From ValSeries:
#if defined __lux32__
#define mChunkSize	0x20000000
#elif defined __win32__
#define mChunkSize	0x20000000
#else
#define mChunkSize	0x800000000
#endif


bool VolProc::ChainExecutor::needSplit( od_int64 memusage,
					od_int64& freemem, int& nrexecs ) const
{
    od_int64 totmem;
    OD::getSystemMemory( totmem, freemem );
    if ( memusage >= freemem )
    {
	nrexecs = freemem <= 0 ? 0
			       : mNINT32( Math::Ceil( mCast(double,memusage) /
					  mCast(double,freemem) ) );
	return true;
    }

    //Ensure all datapack arrays have contiguous memory
    for ( int idx=0; idx<tkzss_.size(); idx++ )
    {
	const od_int64 nrsamples = tkzss_.get(idx)->totalNr() * sizeof(float);
	if ( nrsamples < mChunkSize )
	    continue;

	nrexecs = mNINT32( Math::Ceil(	mCast(double,nrsamples) /
					mCast(double,mChunkSize) ) );
	return true;
    }

    nrexecs = 1;

    return false;
}

#undef mChunkSize


int VolProc::ChainExecutor::nrChunks( const TrcKeySampling& tks,
				      const ZSampling& zsamp,
				      od_int64& memusage )
{
    memusage = calculateMaximumMemoryUsage( tks, zsamp );
    const od_int64 nrbytes = mCast( od_int64, 1.01f * memusage );
    const bool cansplit = areSamplesIndependent() && !needsFullVolume();
    od_int64 freemem; int nrexecs;
    const bool needsplit = needSplit( nrbytes, freemem, nrexecs );
    if ( (needsplit && !cansplit) || freemem <=0 )
	return 0;
    else if ( !needsplit )
	return 1;

    int halfnrlinesperchunk = mNINT32( Math::Ceil( mCast(double,tks.nrLines()) /
						mCast(double,2 * nrexecs) ) );
    while ( halfnrlinesperchunk > 0 )
    {
	TrcKeySampling starthsamp( tks ), centerhsamp( tks ), stophsamp( tks );
	starthsamp.stop_.inl() = starthsamp.start_.inl() +2*halfnrlinesperchunk;
	centerhsamp.start_.inl() = tks.center().inl();
	centerhsamp.stop_.inl() = centerhsamp.start_.inl();
	centerhsamp.expand( halfnrlinesperchunk, 0 );
	stophsamp.start_.inl() = stophsamp.stop_.inl() - 2*halfnrlinesperchunk;
	starthsamp.limitTo( tks );
	centerhsamp.limitTo( tks );
	stophsamp.limitTo( tks );
	const od_int64 memstart =
			calculateMaximumMemoryUsage( starthsamp, zsamp );
	const od_int64 memcenter =
			calculateMaximumMemoryUsage( centerhsamp, zsamp );
	const od_int64 memstop =
			calculateMaximumMemoryUsage( stophsamp, zsamp );
	memusage = memcenter;
	if ( memstart > memusage )
	    memusage = memstart;

	if ( memstop > memusage )
	    memusage = memstop;

	if ( mCast(od_int64,1.01f*memusage) < freemem )
	    break;

	halfnrlinesperchunk--;
    }

    if ( halfnrlinesperchunk < 1 )
	return 0;

    return mNINT32( Math::Ceil( mCast(double,tks.nrLines() /
				mCast(double,2*halfnrlinesperchunk) ) ) );
}


od_int64 VolProc::ChainExecutor::calculateMaximumMemoryUsage(
						    const TrcKeySampling& tks,
						    const ZSampling& zsamp )
{
    updateScheduledStepsSampling( tks, zsamp );
    const Pos::GeomID geomid = tks.getGeomID();
    adjustStepsNrComponents( geomid );

    TypeSet<VolumeMemory> activevolumes;
    for ( int epochidx=0; epochidx<epochs_.size(); epochidx++ )
    {
	const ObjectSet<Step>& steps = epochs_[epochidx]->getSteps();
	TypeSet<Step::ID> inputsids;
	for ( int stepidx=0; stepidx<steps.size(); stepidx++ )
	{
	    od_int64 epochmemsize = 0;
	    const Step* step = steps[stepidx];
	    const int outputidx = 0;
	    //No actual support for multiple output datapacks yet!!
	    if ( !step->validOutputSlotID(step->getOutputSlotID(outputidx)))
		continue;

	    const int stepoutidx = chain_.indexOf( step );
	    const TrcKeyZSampling& outtkzs = *tkzss_[stepoutidx];
	    if ( step->hasPeakMemoryAllocatedOutput() )
	    {
		epochmemsize += step->getComponentMemory( outtkzs, false ) *
				step->getNrOutComponents( outputidx, geomid );
	    }
	    epochmemsize += step->extraMemoryUsage( outputidx, outtkzs );

	    const int nrinputs = step->getNrInputs();
	    if ( nrinputs > 0 && !step->canInputAndOutputBeSame() )
	    {
		TypeSet<Chain::Connection> conns;
		web_.getConnections( step->getID(), true, conns );
		for ( int idx=0; idx<nrinputs; idx++ )
		{
		    const Step::ID inpstepid = conns[idx].outputstepid_;
		    if ( inputsids.isPresent(inpstepid) ||
			 !step->hasPeakMemoryAllocatedInput() )
			continue;

		    inputsids += inpstepid;
		    const Step* inpstep = chain_.getStepFromID( inpstepid );
		    const int stepinpidx = chain_.indexOf( inpstep );
		    const TrcKeyZSampling& inptkzs = *tkzss_[stepinpidx];
		    epochmemsize += inpstep->getComponentMemory(inptkzs,false) *
			step->getNrInputComponents( conns[idx].inputslotid_,
						    geomid );
		}
	    }

	    VolumeMemory volmem(step->getID(),outputidx,epochmemsize,epochidx);
	    activevolumes += volmem;
	}
    }

    od_int64 res = 0;
    for ( int epochidx=0; epochidx<epochs_.size(); epochidx++ )
    {
	od_int64 memneeded = 0;

	for ( int idx=0; idx<activevolumes.size(); idx++ )
	{
	    if ( epochidx != activevolumes[idx].epoch_ )
		continue;

	    memneeded += activevolumes[idx].nrbytes_;
	}
	if ( memneeded > res )
	    res = memneeded;
    }

    return res;
}


bool VolProc::ChainExecutor::areSamplesIndependent() const
{
    for ( int epochidx=0; epochidx<epochs_.size(); epochidx++ )
    {
	const ObjectSet<Step>& steps = epochs_[epochidx]->getSteps();
	for ( int stepidx=0; stepidx<steps.size(); stepidx++ )
	    if ( !steps[stepidx]->areSamplesIndependent() )
		return false;
    }
    return true;
}


bool VolProc::ChainExecutor::needsFullVolume() const
{
    for ( int epochidx=0; epochidx<epochs_.size(); epochidx++ )
    {
	const ObjectSet<Step>& steps = epochs_[epochidx]->getSteps();
	for ( int stepidx=0; stepidx<steps.size(); stepidx++ )
	    if ( steps[stepidx]->needsFullVolume() )
		return true;
    }
    return false;
}


VolProc::ChainExecutor::Epoch::Epoch( const ChainExecutor& ce )
    : taskgroup_(*new TaskGroup)
    , chainexec_(ce)
{
    taskgroup_.setName( ce.name() );
}


bool VolProc::ChainExecutor::Epoch::needsStepOutput( Step::ID stepid ) const
{
    for ( int idx=0; idx<steps_.size(); idx++ )
    {
	const Step* currentstep = steps_[idx];
	if ( currentstep->getID() == stepid )
	    return true;

	TypeSet<Chain::Connection> inputconnections;
	chainexec_.web_.getConnections( currentstep->getID(), true,
					inputconnections );
	for ( int idy=0; idy<inputconnections.size(); idy++ )
	    if ( inputconnections[idy].outputstepid_==stepid )
		return true;
    }

    return false;
}


VolProc::Step::ID VolProc::ChainExecutor::getChainOutputStepID() const
{
    return chain_.outputstepid_;
}


VolProc::Step::OutputSlotID VolProc::ChainExecutor::getChainOutputSlotID() const
{
    return chain_.outputslotid_;
}


bool VolProc::ChainExecutor::Epoch::updateInputs()
{
    for ( int idx=0; idx<steps_.size(); idx++ )
    {
	const Step* finishedstep = steps_[idx];
	const RegularSeisDataPack* input = finishedstep->getOutput();
	if ( !input )
	{
	    pErrMsg("Output is not available");
	    return false;
	}

	TypeSet<Chain::Connection> connections;
	chainexec_.web_.getConnections( finishedstep->getID(), false,
					connections );
	for ( int idy=0; idy<connections.size(); idy++ )
	{
	    Step* futurestep = chainexec_.chain_.getStepFromID(
				     connections[idy].inputstepid_ );
	    if ( !futurestep )
		continue;

	    const Step::OutputSlotID finishedslot =
					connections[idy].outputslotid_;
	    const Step::InputSlotID futureslot = connections[idy].inputslotid_;
	    if ( !finishedstep->validOutputSlotID(finishedslot) ||
		 !futurestep->validInputSlotID(futureslot) )
	    {
		pErrMsg("Output is not available");
		return false;
	    }

	    futurestep->setInput( futureslot, input );
	}
    }

    return true;
}


bool VolProc::ChainExecutor::Epoch::doPrepare()
{
    for ( int idx=0; idx<steps_.size(); idx++ )
    {
	Step* currentstep = steps_[idx];
	PosInfo::CubeData cubedata;
	for ( int idy=0; idy<currentstep->getNrInputs(); idy++ )
	{
	    const Step::InputSlotID inputslot =
				    currentstep->getInputSlotID( idy );
	    if ( !currentstep->validInputSlotID(inputslot) )
	    {
		pErrMsg("This should not happen");
		return false;
	    }

	    const RegularSeisDataPack* input = currentstep->getInput(inputslot);
	    const auto* inpcd = !input ? nullptr : input->tracePositions();
	    if ( inpcd )
		cubedata.merge( *inpcd, true );
	}

	TrcKeyZSampling csamp;
	if ( !chainexec_.getCalculationScope(currentstep->getID(),csamp) ||
	     !csamp.isDefined() )
	{
	    BufferString msg( "This should not happen" );
	    errmsg_ = toUiString( msg );
	    pErrMsg( msg.str() );
	    return false;
	}

	const Pos::GeomID geomid = csamp.hsamp_.getGeomID();
	const RegularSeisDataPack* outfrominp =
		    currentstep->canInputAndOutputBeSame() &&
		    currentstep->validInputSlotID(0) ?
		    currentstep->getInput( currentstep->getInputSlotID(0) ) : 0;

	RefMan<RegularSeisDataPack> outcube;
	if ( outfrominp )
	    outcube = const_cast<RegularSeisDataPack*>( outfrominp );
	else
	{
	    outcube = new RegularSeisDataPack(
			VolumeDataPack::categoryStr(true,csamp.hsamp_.is2D()) );
	    DPM( DataPackMgr::SeisID() ).add( outcube );
	    outcube->setName( "New VolProc DP" );
	    outcube->setSampling( csamp );
	    CubeHorSubSel chss( csamp.hsamp_ );
	    if ( cubedata.totalSizeInside( chss ) > 0 )
	    {
		cubedata.limitTo( chss );
		if ( !cubedata.isFullyRegular() )
		    outcube->setTracePositions( new PosInfo::SortedCubeData(
							cubedata) );
	    }

	    for ( int icomp=0; icomp<currentstep->getNrOutComponents(0,geomid);
			       icomp++ )
	    {
		if ( !outcube->addComponent(0,false) )
		{
		    errmsg_ = uiStrings::phrCannotAllocateMemory();
		    outcube = 0;
		    return false;
		}
	    }
	}

	Step::OutputSlotID outputslotid = 0; // TODO: get correct slotid
	currentstep->setOutput( outputslotid, outcube );

	//The step should have reffed it, so we can forget it now.
	outcube = 0;

	TypeSet<Chain::Connection> outputconnections;
	chainexec_.web_.getConnections( currentstep->getID(),
					false, outputconnections );

	for ( int idy=0; idy<outputconnections.size(); idy++ )
	     currentstep->enableOutput( outputconnections[idy].outputslotid_ );

	if ( currentstep->getID()==chainexec_.getChainOutputStepID() )
	    currentstep->enableOutput( chainexec_.getChainOutputSlotID() );

	ReportingTask* newtask = currentstep->createTask();
	if ( !newtask )
	{
	    errmsg_ = tr("Could not create task: %1\n")
			.arg( currentstep->errMsg() );
	    return false;
	}

	taskgroup_.addTask( newtask );
    }

    return true;
}


void VolProc::ChainExecutor::Epoch::releaseData()
{
    for ( int idx=0; idx<steps_.size(); idx++ )
	steps_[idx]->releaseData();
}


RegularSeisDataPack* VolProc::ChainExecutor::Epoch::getOutput() const
{
    return steps_[steps_.size()-1]
        ? const_cast<RegularSeisDataPack*>(
                                steps_[steps_.size()-1]->getOutput().ptr())
        : 0;
}


const VolProc::Step::CVolRef VolProc::ChainExecutor::getOutput() const
{
    return outputdp_;
}

VolProc::Step::VolRef VolProc::ChainExecutor::getOutput()
{
    return outputdp_;
}

#define mCleanUpAndRet( prepare ) \
{ \
    uiStringSet errors; \
    const ObjectSet<Step>& cursteps = curepoch_->getSteps(); \
    for ( int istep=0; istep<cursteps.size(); istep++ ) \
    { \
	if ( !cursteps[istep] || cursteps[istep]->errMsg().isEmpty() ) \
	    continue; \
\
	errors.add( cursteps[istep]->errMsg() ); \
    } \
    if ( prepare && !curepoch_->errMsg().isEmpty() ) \
        errors.add( curepoch_->errMsg() ); \
    else if ( !prepare && !curepoch_->getTask().message().isEmpty() ) \
	errors.add( curepoch_->getTask().message() ); \
    \
    if ( !errors.isEmpty() ) \
	errmsg_ = errors.cat(); \
    deleteAndZeroPtr( curepoch_ ); \
    return ErrorOccurred(); \
}

int VolProc::ChainExecutor::nextStep()
{
    if ( !isok_ )
	return ErrorOccurred();
    if ( epochs_.isEmpty() )
	return Finished();

    curepoch_ = epochs_.pop();

    if ( !curepoch_->doPrepare() )
	mCleanUpAndRet( true )

    Task& curtask = curepoch_->getTask();
    mDynamicCastGet(ReportingTask*,curreptask,&curtask)
    curreptask->getProgress( *this );
    curtask.enableWorkControl( true );
    if ( !curtask.execute() )
	mCleanUpAndRet( false )

    const bool finished = epochs_.isEmpty();
    if ( finished )		//we just executed the last one
	outputdp_ = curepoch_->getOutput();

    //Give output volumes to all steps that need them
    if ( !finished && !curepoch_->updateInputs() )
	return false;

    //Everyone who wants my data has it. I can release it.
    curepoch_->releaseData();
    deleteAndZeroPtr( curepoch_ );

    return finished ? Finished() : MoreToDo();
}


void VolProc::ChainExecutor::releaseMemory()
{
    ObjectSet<Step> stepstorelease = scheduledsteps_;
    for ( int idx=stepstorelease.size()-1; idx>=0; idx-- )
    {
	const Step::ID curid = stepstorelease[idx]->getID();
	for ( int idy=0; idy<epochs_.size(); idy++ )
	{
	    if ( epochs_[idy]->needsStepOutput( curid ) )
	    {
		stepstorelease.removeSingle( idx );
		break;
	    }
	}
    }

    for ( int idx=stepstorelease.size()-1; idx>=0; idx-- )
	stepstorelease[idx]->releaseData();
}


void VolProc::ChainExecutor::controlWork( Task::Control ctrl )
{
    Task::controlWork( ctrl );
    if ( curepoch_ )
	curepoch_->getTask().controlWork( ctrl );
}


od_int64 VolProc::ChainExecutor::nrDone() const
{
    if ( totalnrepochs_ < 1 )
	return 0;

    const float percentperepoch = 100.f / totalnrepochs_;
    const int epochsdone = totalnrepochs_ - epochs_.size() - 1;
    float percentagedone = percentperepoch * epochsdone;

    if ( curepoch_ )
    {
	Task& curtask = curepoch_->getTask();

	const od_int64 nrdone = curtask.nrDone();
	const od_int64 totalnr = curtask.totalNr();
	if ( nrdone>=0 && totalnr>0 )
	{
	    const float curtaskpercentage = percentperepoch * nrdone / totalnr;
	    percentagedone += curtaskpercentage;
	}
    }

    return mNINT64( percentagedone );
}


uiString VolProc::ChainExecutor::nrDoneText() const
{
    return uiStrings::sPercentageDone();
}


od_int64 VolProc::ChainExecutor::totalNr() const
{
    return 100;
}


uiString VolProc::ChainExecutor::message() const
{
    if ( !errmsg_.isEmpty() )
	return errmsg_;

    if ( curepoch_ )
	return curepoch_->getTask().message();

    return uiString::empty();
}
