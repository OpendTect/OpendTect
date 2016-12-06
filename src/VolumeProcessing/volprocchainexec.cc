/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : October 2006
-*/


#include "volprocchainexec.h"

#include "seisdatapack.h"
#include "threadwork.h"
#include "simpnumer.h" // for getCommonStepInterval

uiString VolProc::ChainExecutor::sGetStepErrMsg()
{
    return uiStrings::phrCannotFind( tr("output step with id: %1") );
}


VolProc::ChainExecutor::ChainExecutor( Chain& vr )
    : ::Executor( "Volume processing" )
    , chain_( vr )
    , isok_( true )
    , outputzrg_( 0, 0, 0 )
    , outputdp_( 0 )
    , totalnrepochs_( 1 )
    , curepoch_( 0 )
{
    setName( vr.name().getFullString() );
    web_ = chain_.getWeb();
    //TODO Optimize connections, check for indentical steps using same inputs
}


VolProc::ChainExecutor::~ChainExecutor()
{
    deepErase( epochs_ );
    if ( curepoch_ )
	delete curepoch_;
}


uiString VolProc::ChainExecutor::errMsg() const
{ return errmsg_; }


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

	const int lateststepepoch = computeLatestEpoch( currentstep->getID() );
	if ( mIsUdf(lateststepepoch) )
	{
	    pErrMsg("Cannot compute latest epoch");
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
	    if ( computeLatestEpoch( scheduledsteps_[idy]->getID() )==idx )
		epoch->addStep( scheduledsteps_[idy] );

	epochs_ += epoch;
    }

    totalnrepochs_ = epochs_.size();

    return true;
}


int VolProc::ChainExecutor::computeLatestEpoch( Step::ID stepid ) const
{
    if ( stepid==chain_.outputstepid_ )
	return 0;

    TypeSet<Chain::Connection> outputconnections;
    web_.getConnections( stepid, false, outputconnections );

    int latestepoch = mUdf(int);
    for ( int idx=0; idx<outputconnections.size(); idx++ )
    {
	const int curepoch =
	    computeLatestEpoch( outputconnections[idx].inputstepid_ );
	if ( mIsUdf(curepoch) )
	    continue;

	const int newepoch = curepoch+1;
	if ( mIsUdf(latestepoch) || newepoch>latestepoch )
	    latestepoch = newepoch;
    }

    return latestepoch;
}


void VolProc::ChainExecutor::computeComputationScope( Step::ID stepid,
				TrcKeySampling& stepoutputhrg,
				StepInterval<int>& stepoutputzrg ) const
{
    if ( stepid==chain_.outputstepid_ )
    {
	stepoutputhrg = outputhrg_;
	stepoutputzrg = outputzrg_;
	return;
    }

    TypeSet<Chain::Connection> outputconnections;
    web_.getConnections( stepid, false, outputconnections );

    stepoutputhrg.init(false);
    stepoutputzrg.setUdf();

    for ( int idx=0; idx<outputconnections.size(); idx++ )
    {
	const Step* nextstep = chain_.getStepFromID(
			    outputconnections[idx].inputstepid_ );

	TrcKeySampling nextstephrg;
	StepInterval<int> nextstepzrg;
	computeComputationScope( nextstep->getID(), nextstephrg, nextstepzrg );

	const TrcKeySampling requiredhrg = nextstep->getInputHRg( nextstephrg );
	if ( stepoutputhrg.isDefined() )
	    stepoutputhrg.include( requiredhrg );
	else
	    stepoutputhrg = requiredhrg;

	const StepInterval<int> requiredzrg =
		nextstep->getInputZRg( nextstepzrg );
	if ( stepoutputzrg.isUdf() )
	    stepoutputzrg = requiredzrg;
	else
	    stepoutputzrg = getCommonStepInterval( stepoutputzrg, requiredzrg );

    }
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
			, epoch_(epoch)				{};

    bool		operator==( VolumeMemory vm ) const
			{
			    return creator_ == vm.creator_ &&
				   outputslot_ == vm.outputslot_ &&
				   nrbytes_ == vm.nrbytes_ &&
				   epoch_ == vm.epoch_;
			}
};

} // namespace VolProc


od_int64 VolProc::ChainExecutor::computeMaximumMemoryUsage(
					const TrcKeySampling& hrg,
					const StepInterval<int>& zrg )
{
    if ( !scheduleWork() )
	return -1;

    TypeSet<VolumeMemory> activevolumes;
    const od_int64 basesize = Step::getBaseMemoryUsage( hrg, zrg );

    for ( int epochidx=0; epochidx<epochs_.size(); epochidx++ )
    {
	const ObjectSet<Step>& steps = epochs_[epochidx]->getSteps();
	for ( int stepidx=0; stepidx<steps.size(); stepidx++ )
	{
	    const Step* step = steps[stepidx];
	    for ( int outputidx=0; outputidx<step->getNrOutputs(); outputidx++ )
	    {
		if ( !step->validOutputSlotID(step->getOutputSlotID(outputidx)))
		    continue;

		const od_int64 extrasize = step->extraMemoryUsage( outputidx,
								    hrg, zrg );

		od_int64 outputsize = basesize + extrasize;
		if ( step->getNrInputs() > 0 &&
		     !step->canInputAndOutputBeSame() )
		    outputsize += basesize;

		VolumeMemory volmem( step->getID(), outputidx, outputsize,
				     epochidx );

		activevolumes += volmem;
	    }
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


bool VolProc::ChainExecutor::setCalculationScope( const TrcKeySampling& hrg,
					 const StepInterval<int>& zrg )
{
    outputhrg_ = hrg;
    outputzrg_ = zrg;

    return scheduleWork();
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
	    const int outputidx = finishedstep->getOutputIdx( finishedslot );
	    if ( !input->validComp(outputidx) )
	    {
		pErrMsg("Output is not available");
		return false;
	    }

	    futurestep->setInput( connections[idy].inputslotid_, input );
	}
    }

    return true;
}


bool VolProc::ChainExecutor::Epoch::doPrepare( ProgressMeter* progmeter )
{
    for ( int idx=0; idx<steps_.size(); idx++ )
    {
	Step* currentstep = steps_[idx];
	PosInfo::CubeData posdata;
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
	    const PosInfo::CubeData* inppd = !input ? 0 : input->trcsSampling();
	    if ( inppd )
		posdata.merge( *inppd, true );
	}

	TrcKeySampling stepoutputhrg;
	StepInterval<int> stepoutputzrg;

	chainexec_.computeComputationScope( currentstep->getID(), stepoutputhrg,
					    stepoutputzrg );

	TrcKeyZSampling csamp;
	csamp.hsamp_ = stepoutputhrg;
	const StepInterval<float> fullzrg = csamp.zsamp_;
	csamp.zsamp_.start = stepoutputzrg.start * fullzrg.step; //index -> real
	csamp.zsamp_.stop = stepoutputzrg.stop * fullzrg.step; //index -> real
	csamp.zsamp_.step = stepoutputzrg.step * fullzrg.step; //index -> real

	const RegularSeisDataPack* outfrominp =
		    currentstep->canInputAndOutputBeSame() &&
		    currentstep->validInputSlotID(0) ?
		    currentstep->getInput( currentstep->getInputSlotID(0) ) : 0;

	RefMan<RegularSeisDataPack> outcube = outfrominp
	    ? const_cast<RegularSeisDataPack*>( outfrominp )
	    : DPM( DataPackMgr::SeisID() ).add( new RegularSeisDataPack( 0 ) );
	if ( !outfrominp )
	{
	    outcube->setSampling( csamp );
	    if ( posdata.totalSizeInside( csamp.hsamp_ ) > 0 )
	    {
		posdata.limitTo( csamp.hsamp_ );
		if ( !posdata.isFullyRectAndReg() )
		    outcube->setTrcsSampling(
				    new PosInfo::SortedCubeData(posdata) );
	    }

	    if ( !outcube->addComponent( 0 ) )
	    { //TODO: allocate the step-required number of components
		errmsg_ = "Cannot allocate enough memory.";
		outcube = 0;
		return false;
	    }
	}

	Step::OutputSlotID outputslotid = 0; // TODO: get correct slotid
	currentstep->setOutput( outputslotid, outcube,
				stepoutputhrg, stepoutputzrg );

	//The step should have reffed it, so we can forget it now.
	outcube = 0;

	TypeSet<Chain::Connection> outputconnections;
	chainexec_.web_.getConnections( currentstep->getID(),
					false, outputconnections );

	for ( int idy=0; idy<outputconnections.size(); idy++ )
	     currentstep->enableOutput( outputconnections[idy].outputslotid_ );

	if ( currentstep->getID()==chainexec_.getChainOutputStepID() )
	    currentstep->enableOutput( chainexec_.getChainOutputSlotID() );

	Task* newtask = currentstep->needReportProgress()
			    ? currentstep->createTaskWithProgMeter(progmeter)
			    : currentstep->createTask();
	if ( !newtask )
	{
	    pErrMsg("Could not create task");
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


const RegularSeisDataPack* VolProc::ChainExecutor::getOutput() const
{
    return outputdp_;
}

RegularSeisDataPack* VolProc::ChainExecutor::getOutput()
{
    return outputdp_;
}



#define mCleanUpAndRet( ret ) \
{ \
    delete curepoch_; \
    curepoch_ = 0; \
    return ret; \
}

int VolProc::ChainExecutor::nextStep()
{
    if ( !isok_ )
	return ErrorOccurred();
    if ( epochs_.isEmpty() )
	return Finished();

    curepoch_ = epochs_.pop();

    if ( !curepoch_->doPrepare(progressmeter_) )
	mCleanUpAndRet( ErrorOccurred() )

    Task& curtask = curepoch_->getTask();
    curtask.setProgressMeter( progressmeter_ );
    curtask.enableWorkControl( true );
    if ( !curtask.execute() )
	mCleanUpAndRet( ErrorOccurred() )

    const bool finished = epochs_.isEmpty();
    if ( finished )		//we just executed the last one
	outputdp_ = curepoch_->getOutput();

    //Give output volumes to all steps that need them
    if ( !finished && !curepoch_->updateInputs() )
	return false;

    //Everyone who wants my data has it. I can release it.
    curepoch_->releaseData();
    deleteAndZeroPtr( curepoch_ );
    if ( finished )
	progressmeter_ = 0;

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

    return uiString::emptyString();
}
