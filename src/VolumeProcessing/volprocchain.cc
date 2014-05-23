/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : October 2006
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "volprocchain.h"

#include "threadwork.h"

#include "bufstring.h"
#include "iopar.h"
#include "simpnumer.h"
#include "keystrs.h"
#include "survinfo.h"
#include "paralleltask.h"

namespace VolProc
{

mImplFactory( Step, Step::factory );


class BinIDWiseTask : public ParallelTask
{
public:
		BinIDWiseTask( Step& ro )
		    : step_( ro ), totalnr_( -1 )	{}

    uiStringCopy	uiMessage() const	{ return errmsg_; }
    uiStringCopy	uiNrDoneText() const	{ return "Positions done"; }

protected:
    bool	doWork(od_int64 start, od_int64 stop, int threadid )
		{
		    const HorSampling hrg( step_.output_->cubeSampling().hrg );
		    BinID curbid = hrg.start;

		    const int nrinls = mCast( int, start/hrg.nrCrl() );
		    const int nrcrls = mCast( int, start - nrinls*hrg.nrCrl() );
		    curbid.inl() += nrinls*hrg.step.inl();
		    curbid.crl() += nrcrls*hrg.step.crl();

		    for ( int idx=mCast(int,start); idx<=stop; idx++ )
		    {
			if ( !step_.computeBinID( curbid, threadid ) )
			    return false;

			addToNrDone( 1 );

			if ( idx>=stop )
			    break;

			if ( !shouldContinue() )
			    return false;

			curbid.crl() += hrg.step.crl();
			if ( curbid.crl()>hrg.stop.crl() )
			{
			    curbid.crl() = hrg.start.crl();
			    curbid.inl() += hrg.step.inl();
			    if ( curbid.inl()>hrg.stop.inl() )
			    {
				pErrMsg("Going outside range");
				return false;
			    }
			}
		    }

		    return true;
		}

    od_int64	nrIterations() const
		{
		    if ( totalnr_==-1 )
		    {
			const HorSampling hrg(
				step_.output_->cubeSampling().hrg );
			totalnr_ = hrg.nrInl() * hrg.nrCrl();
		    }

		    return totalnr_;
		}

    bool	doPrepare( int nrthreads )
		{
		    const bool res = step_.prepareComp( nrthreads );
		    if ( !res ) errmsg_ = step_.errMsg();
		    return res;
		}

    Step&		step_;
    mutable int		totalnr_;
    uiString		errmsg_;
};



ChainExecutor::ChainExecutor( Chain& vr )
    : Executor( "Volume processing" )
    , chain_( vr )
    , isok_( true )
    , outputzrg_( 0, 0, 0 )
    , outputvolume_( 0 )
    , totalnrepochs_( 1 )
    , curepoch_( 0 )
{
    web_ = chain_.getWeb();
    //Todo: Optimize connections, check for indentical steps using same inputs
}


ChainExecutor::~ChainExecutor()
{
    deepErase( epochs_ );
    if ( curepoch_ )
	delete curepoch_;
}


uiStringCopy ChainExecutor::errMsg() const
{ return errmsg_; }


#define mGetStep( var, id, errret ) \
Step* var = chain_.getStepFromID( id ); \
if ( !var ) \
{ \
    errmsg_ = uiString("Cannot find output step with id %1") \
		  .arg( toString(id) ); \
    return errret; \
}


bool ChainExecutor::scheduleWork()
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


int ChainExecutor::computeLatestEpoch( Step::ID stepid ) const
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


void ChainExecutor::computeComputationScope( Step::ID stepid,
				HorSampling& stepoutputhrg,
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
    stepoutputzrg = StepInterval<int>::udf();

    for ( int idx=0; idx<outputconnections.size(); idx++ )
    {
	const Step* nextstep = chain_.getStepFromID(
			    outputconnections[idx].inputstepid_ );

	HorSampling nextstephrg;
	StepInterval<int> nextstepzrg;
	computeComputationScope( nextstep->getID(), nextstephrg, nextstepzrg );

	const HorSampling requiredhrg = nextstep->getInputHRg( nextstephrg );
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


bool ChainExecutor::setCalculationScope( const HorSampling& hrg,
					 const StepInterval<int>& zrg )
{
    outputhrg_ = hrg;
    outputzrg_ = zrg;

    return scheduleWork();
}


bool ChainExecutor::Epoch::needsStepOutput( Step::ID stepid ) const
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


bool ChainExecutor::Epoch::doPrepare()
{
    for ( int idx=0; idx<steps_.size(); idx++ )
    {
	Step* currentstep = steps_[idx];
	TypeSet<Chain::Connection> inputconnections;
	chainexec_.web_.getConnections( currentstep->getID(), true,
					inputconnections );
	for ( int idy=0; idy<inputconnections.size(); idy++ )
	{
	    const Step* outputstep = chainexec_.chain_.getStepFromID(
					inputconnections[idy].outputstepid_ );
	    if ( !outputstep )
	    {
		pErrMsg("This should not happen");
		return false;
	    }

	    RefMan<const Attrib::DataCubes> input = outputstep->getOutput();
	    const Step::OutputSlotID outputslot =
					inputconnections[idy].outputslotid_;
	    const int outputidx = outputstep->getOutputIdx( outputslot );

	    if ( !input->validCubeIdx(outputidx) )
	    {
		pErrMsg("Output is not available");
		return false;
	    }

	    currentstep->setInput( inputconnections[idy].inputslotid_, input );
	}

	HorSampling stepoutputhrg;
	StepInterval<int> stepoutputzrg;

	chainexec_.computeComputationScope( currentstep->getID(), stepoutputhrg,
					    stepoutputzrg );

	CubeSampling csamp;
	csamp.hrg = stepoutputhrg;
	Attrib::DataCubes* outcube = new Attrib::DataCubes;
	outcube->ref();
	outcube->setSizeAndPos( csamp );
	if ( !outcube->addCube(mUdf(float),0) )
	{
	    errmsg_ = "Cannot allocate enough memory.";
	    outcube->unRef();
	    return false;
	}

	Step::OutputSlotID outputslotid = 0; // TODO: get correct slotid
	currentstep->setOutput( outputslotid, outcube,
				stepoutputhrg, stepoutputzrg );

	TypeSet<Chain::Connection> outputconnections;
	chainexec_.web_.getConnections( currentstep->getID(),
					false, outputconnections );

	for ( int idy=0; idy<outputconnections.size(); idy++ )
	     currentstep->enableOutput( outputconnections[idy].outputslotid_ );

	if ( currentstep->getID()==chainexec_.chain_.outputstepid_ )
	    currentstep->enableOutput( chainexec_.chain_.outputslotid_ );

	Task* newtask = currentstep->createTask();
	if ( !newtask )
	{
	    pErrMsg("Could not create task");
	    return false;
	}

	taskgroup_.addTask( newtask );
    }

    return true;
}


const Attrib::DataCubes* ChainExecutor::Epoch::getOutput() const
{
    return steps_[steps_.size()-1] ? steps_[steps_.size()-1]->getOutput() : 0;
}


const Attrib::DataCubes* ChainExecutor::getOutput() const
{ return outputvolume_; }


#define mCleanUpAndRet( ret ) \
{ \
    delete curepoch_; \
    curepoch_ = 0; \
    return ret; \
}

int ChainExecutor::nextStep()
{

    if ( curepoch_ )
    {
	delete curepoch_;
	curepoch_ = 0;
    }

    if ( !isok_ )
	return ErrorOccurred();

    if ( epochs_.isEmpty() )
	return Finished();

    releaseMemory();

    //curtasklock_.lock();
    curepoch_ = epochs_.pop();

    if ( !curepoch_->doPrepare() )
	mCleanUpAndRet( ErrorOccurred() )

    Task& curtask = curepoch_->getTask();
    curtask.setProgressMeter( progressmeter_ );
    curtask.enableWorkControl( true );

    //curtasklock_.unLock();

    if ( !curtask.execute() )
	mCleanUpAndRet( ErrorOccurred() )

    if ( epochs_.isEmpty() )		//we just executed the last one
	outputvolume_ = curepoch_->getOutput();

    return epochs_.isEmpty() ? Finished() : MoreToDo();
}


void ChainExecutor::releaseMemory()
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


void ChainExecutor::controlWork( Task::Control ctrl )
{
    Task::controlWork( ctrl );
    //Threads::Locker lckr( curtasklock_ );
    if ( curepoch_ )
	curepoch_->getTask().controlWork( ctrl );
}


od_int64 ChainExecutor::nrDone() const
{
    //Threads::Locker lckr( curtasklock_ );
    const float percentperepoch = 100.f/totalnrepochs_;
    const int epochsdone =
	epochs_.isEmpty() ? 0 : totalnrepochs_-(epochs_.size()-1);
    float percentagedone = percentperepoch*epochsdone;

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

    return mNINT32(percentagedone);
}


uiStringCopy ChainExecutor::uiNrDoneText() const
{ return curepoch_ ? curepoch_->getTask().uiNrDoneText() : "Positions done"; }


od_int64 ChainExecutor::totalNr() const
{ return 100; }


uiStringCopy ChainExecutor::uiMessage() const
{
    if ( !errmsg_.isEmpty() )
	return errmsg_;

    //Threads::Locker lckr( curtasklock_ );
    if ( curepoch_ )
	return curepoch_->getTask().uiMessage();
    return 0;
}


Chain::Connection::Connection( Step::ID outpstepid,
			       Step::OutputSlotID outpslotid,
			       Step::ID inpstepid,
			       Step::InputSlotID inpslotid )
    : outputstepid_( outpstepid )
    , outputslotid_( outpslotid )
    , inputstepid_( inpstepid )
    , inputslotid_( inpslotid )
{

}


bool Chain::Connection::isUdf() const
{
    return  mIsUdf( outputstepid_ ) ||
	    mIsUdf( outputslotid_) ||
	    mIsUdf( inputstepid_) ||
	    mIsUdf( inputslotid_ );
}


bool Chain::Connection::operator==( const VolProc::Chain::Connection& b ) const
{
    return outputstepid_==b.outputstepid_ &&
	    outputslotid_==b.outputslotid_ &&
	    inputstepid_==b.inputstepid_ &&
	    inputslotid_==b.inputslotid_;
}


void Chain::Connection::fillPar( IOPar& iopar, const char* key ) const
{
    iopar.set( key, outputstepid_, outputslotid_,
	       inputstepid_, inputslotid_ );
}


bool Chain::Connection::usePar( const IOPar& iopar, const char* key )
{
    return iopar.get( key, outputstepid_, outputslotid_,
		      inputstepid_, inputslotid_ );
}


bool Chain::Connection::operator!=( const VolProc::Chain::Connection& b ) const
{
    return !((*this)==b);
}


Chain::Chain()
    : zstep_( SI().zRange(true).step )
    , zist_( SI().zIsTime() )
    , freeid_( 0 )
    , outputstepid_( Step::cUndefID() )
    , outputslotid_( Step::cUndefSlotID() )
{}


Chain::~Chain()
{ deepErase( steps_ ); }


bool Chain::addConnection(const VolProc::Chain::Connection& c )
{
    if ( !validConnection(c) )
	return false;

    web_.getConnections().addIfNew( c );

    return true;
}



void Chain::removeConnection(const VolProc::Chain::Connection& c )
{
    web_.getConnections() -= c;
}


bool Chain::validConnection( const VolProc::Chain::Connection& c ) const
{
    if ( c.isUdf() )
	return false;

    const Step* outputstep = getStepFromID( c.outputstepid_ );
    if ( !outputstep || !outputstep->validOutputSlotID(c.outputslotid_) )
	return false;

    const Step* inputstep = getStepFromID( c.inputstepid_ );
    if ( !inputstep || !inputstep->validInputSlotID(c.inputslotid_) )
	return false;

    return true;
}


int Chain::nrSteps() const
{ return steps_.size(); }


Step* Chain::getStep( int idx )
{ return steps_[idx]; }


Step* Chain::getStepFromID( Step::ID id )
{
    for ( int idx=0; idx<steps_.size(); idx++ )
	if ( steps_[idx]->getID()==id )
	    return steps_[idx];

    return 0;
}


const Step* Chain::getStepFromID( Step::ID id ) const
{ return const_cast<Chain*>( this )->getStepFromID( id ); }


Step* Chain::getStepFromName( const char* nm )
{
    for ( int idx=0; idx<steps_.size(); idx++ )
    {
	const FixedString usrnm = steps_[idx]->userName();
	if ( usrnm == nm )
	    return steps_[idx];
    }

    return 0;
}


const Step* Chain::getStepFromName( const char* nm ) const
{ return const_cast<Chain*>(this)->getStepFromName( nm ); }


int Chain::indexOf( const Step* r ) const
{ return steps_.indexOf( r ); }


void Chain::addStep( Step* r )
{
    r->setChain( *this );
    steps_ += r;
}


void Chain::insertStep( int idx, Step* r )
{ steps_.insertAt( r, idx ); }


void Chain::swapSteps( int o1, int o2 )
{
    steps_.swap( o1, o2 );
}


void Chain::removeStep( int idx )
{
    delete steps_.removeSingle( idx );
}


const VelocityDesc* Chain::getVelDesc() const
{
    for ( int idx=steps_.size()-1; idx>=0; idx-- )
	if ( steps_[idx]->getVelDesc() )
	    return steps_[idx]->getVelDesc();

    return 0;
}


bool Chain::areSamplesIndependent() const
{
    for ( int idx=steps_.size()-1; idx>=0; idx-- )
	if ( !steps_[idx]->areSamplesIndependent() )
	    return false;

    return true;
}


bool Chain::needsFullVolume() const
{
    for ( int idx=steps_.size()-1; idx>=0; idx-- )
	if ( steps_[idx]->needsFullVolume() )
	    return true;

    return false;
}


void Chain::fillPar( IOPar& par ) const
{
    par.set( sKeyNrSteps(), steps_.size() );
    for ( int idx=0; idx<steps_.size(); idx++ )
    {
	IOPar oppar;
	oppar.set( sKeyStepType(), steps_[idx]->factoryKeyword() );
	steps_[idx]->fillPar( oppar );

	par.mergeComp( oppar, toString(idx) );
    }

    BufferString key;
    const TypeSet<Chain::Connection>& conns = web_.getConnections();
    par.set( sKeyNrConnections(), conns.size() );
    for ( int idx=0; idx<conns.size(); idx++ )
	conns[idx].fillPar( par, sKeyConnection(idx,key)  );

    par.set( sKey::Output(), outputstepid_, outputslotid_ );
}


const char* Chain::sKeyConnection( int idx, BufferString& str )
{
    str = "Connection ";
    str += idx;
    return str.str();
}


bool Chain::usePar( const IOPar& par )
{
    deepErase( steps_ );
    web_.getConnections().erase();

    const char* parseerror = "Parsing error";

    int nrsteps;
    if ( !par.get(sKeyNrSteps(),nrsteps) )
    {
	errmsg_ = parseerror;
	return false;
    }

    for ( int idx=0; idx<nrsteps; idx++ )
    {
	PtrMan<IOPar> steppar = par.subselect( toString(idx) );
	if ( !steppar )
	{
	    errmsg_ = parseerror;
	    return false;
	}

	BufferString type;
	if ( !steppar->get( sKeyStepType(), type ) )
	{
	    errmsg_ = parseerror;
	    return false;
	}

	Step* step = Step::factory().create( type.buf() );
	if ( !step )
	{
	    errmsg_ = tr( "Cannot create Volume processing step %1. "
			  "Perhaps all plugins are not loaded?")
			  .arg( type.buf() );

	    return false;
	}

	if ( !step->usePar( *steppar ) )
	{
	    errmsg_ = tr("Cannot parse Volume Processing's parameters: %1")
			.arg( step->errMsg() );
	    delete step;
	    return false;
	}

	addStep( step );
    }

    int nrconns;
    if ( par.get(sKeyNrConnections(),nrconns) )
    {
	Step::ID outputstepid;
	Step::OutputSlotID outputslotid;
	if ( !par.get(sKey::Output(),outputstepid,outputslotid) ||
	     !setOutputSlot(outputstepid,outputslotid) )
	{
	    errmsg_ = "Could not parse or set output slot.";
	    return false;
	}

	BufferString key;
	for ( int idx=0; idx<nrconns; idx++ )
	{
	    Connection newconn;
	    if ( !newconn.usePar( par, sKeyConnection(idx,key) ) )
	    {
		errmsg_ = tr("Cannot parse Connection %1").arg( toString(idx) );
		return false;
	    }

	    if ( !addConnection(newconn) )
	    {
		errmsg_ = tr( "Cannot add connection ").arg( toString(idx) );
		return false;
	    }
	}
    }
    else if ( steps_.size() ) //Old format, all connections implicit
    {
	for ( int idx=1; idx<steps_.size(); idx++ )
	{
	    Connection conn;
	    conn.inputstepid_ = steps_[idx]->getID();
	    conn.inputslotid_ = steps_[idx]->getInputSlotID( 0 );
	    conn.outputstepid_ = steps_[idx-1]->getID();
	    conn.outputslotid_ = steps_[idx-1]->getOutputSlotID( 0 );

	    if ( !addConnection( conn ) )
	    {
		pErrMsg("Should never happen");
		return false;
	    }
	}

	if ( !setOutputSlot( steps_.last()->getID(),
			     steps_.last()->getOutputSlotID( 0 ) ) )
	{
	    pErrMsg("Should never happen");
	    return false;
	}
    }

    return true;
}


void Chain::setStorageID( const MultiID& mid )
{ storageid_ = mid; }


bool Chain::setOutputSlot( Step::ID stepid, Step::OutputSlotID slotid )
{
    if ( steps_.size() > 1 )
    {
	const Step* step = getStepFromID( stepid );
	if ( !step || !step->validOutputSlotID(slotid) )
	    return false;
    }

    outputstepid_ = stepid;
    outputslotid_ = slotid;

    return true;
}


uiStringCopy Chain::errMsg() const
{ return errmsg_; }


void Chain::Web::getConnections( Step::ID stepid, bool isinput,
			    TypeSet<VolProc::Chain::Connection>& res ) const
{
    for ( int idx=0; idx<connections_.size(); idx++ )
	if ( (isinput && connections_[idx].inputstepid_==stepid) ||
	     (!isinput && connections_[idx].outputstepid_==stepid))
		res += connections_[idx];
}


// Step
Step::Step()
    : chain_( 0 )
    , output_( 0 )
    , id_( cUndefID() )
{
    inputs_.allowNull();
}


Step::~Step()
{
    deepUnRef( inputs_ );
    if ( output_ ) output_->unRef();
}


void Step::resetInput()
{
    deepUnRef( inputs_ );
    inputslotids_.erase();
    for ( int idx=0; idx<getNrInputs(); idx++ )
    {
	inputs_ += 0;
	inputslotids_ += idx;
    }
}


void Step::releaseData()
{
    if ( output_ ) output_->unRef();
    output_ = 0;

    resetInput();
}


void Step::setChain( VolProc::Chain& c )
{
    if ( chain_ )
    {
	pErrMsg("Can only add to chain once");
	return;
    }

    chain_ = &c;
    if ( mIsUdf(id_) )
	id_ = c.getNewStepID();
}


const char* Step::userName() const
{ return username_.isEmpty() ? 0 : username_.buf(); }


void Step::setUserName( const char* nm )
{ username_ = nm; }

int Step::getNrInputs() const
{ return needsInput() ? 1 : 0; }


int Step::getInputSlotID( int idx ) const
{
    if ( !needsInput() )
	return Step::cUndefSlotID();

    if ( idx<0 || idx>=getNrInputs() )
    {
	pErrMsg("Invalid input slot");
	return Step::cUndefSlotID();
    }

    return inputslotids_.validIdx(idx) ? inputslotids_[idx] : idx;
}


void Step::getInputSlotName( InputSlotID slotid, BufferString& res ) const
{ res = "Input "; res.add( slotid ); }


int Step::getOutputSlotID( int idx ) const
{
    if ( idx<0 || idx>=getNrOutputs() )
    {
	pErrMsg("Invalid output slot");
	return Step::cUndefSlotID();
    }

    return idx;
}


bool Step::validInputSlotID( InputSlotID slotid ) const
{
    for ( int idx=0; idx<getNrInputs(); idx++ )
    {
	if ( getInputSlotID(idx)==slotid )
	    return true;
    }

    return false;
}


bool Step::validOutputSlotID( OutputSlotID slotid ) const
{
    for ( int idx=0; idx<getNrOutputs(); idx++ )
    {
	if ( getOutputSlotID(idx)==slotid )
	    return true;
    }

    return false;
}


HorSampling Step::getInputHRg( const HorSampling& hr ) const
{ return hr; }


StepInterval<int>
    Step::getInputZRg( const StepInterval<int>& si ) const
{ return si; }


void Step::setInput( InputSlotID slotid, const Attrib::DataCubes* dc )
{
    if ( inputs_.isEmpty() )
	resetInput();

    const int idx = inputslotids_.indexOf( slotid );
    if ( !inputs_.validIdx(idx) )
	return;

    if ( inputs_[idx] )	inputs_[idx]->unRef();
    inputs_.replace( idx, dc );
    if ( inputs_[idx] ) inputs_[idx]->ref();
}


const Attrib::DataCubes* Step::getInput( InputSlotID slotid ) const
{
    const int idx = inputslotids_.indexOf( slotid );
    return inputs_.validIdx(idx) ? inputs_[idx] : 0;
}


void Step::setOutput( OutputSlotID slotid, Attrib::DataCubes* dc,
		      const HorSampling& hrg,
		      const StepInterval<int>& zrg )
{
    if ( output_ ) output_->unRef();
    output_ = dc;
    if ( output_ ) output_->ref();

    hrg_ = hrg;
    zrg_ = zrg;
}


Attrib::DataCubes* Step::getOutput( OutputSlotID slotid )
{
    // TODO: implement using slotid
    return output_;
}


const Attrib::DataCubes* Step::getOutput( OutputSlotID slotid ) const
{ return const_cast<Step*>(this)->getOutput( slotid ); }


void Step::enableOutput( OutputSlotID slotid )
{
    outputslotids_.addIfNew( slotid );
}


int Step::getOutputIdx( OutputSlotID slotid ) const
{
    return outputslotids_.indexOf( slotid );
}


void Step::fillPar( IOPar& par ) const
{
    if ( !username_.isEmpty() )
	par.set( sKey::Name(), username_.buf() );

    par.set( sKey::ID(), id_ );
}


bool Step::usePar( const IOPar& par )
{
    username_.empty();
    par.get( sKey::Name(), username_ );
    if ( !par.get(sKey::ID(),id_) && chain_ )
	id_ = chain_->getNewStepID();

    return true;
}


Task* Step::createTask()
{
    if ( areSamplesIndependent() && prefersBinIDWise() )
	return new BinIDWiseTask( *this );

    return 0;
}

} // namespace Volproc

