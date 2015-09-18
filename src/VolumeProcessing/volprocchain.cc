/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : October 2006
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "volprocchain.h"

#include "threadwork.h"

#include "bufstring.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "simpnumer.h"
#include "keystrs.h"
#include "seisdatapack.h"
#include "survinfo.h"
#include "paralleltask.h"

namespace VolProc
{

mImplFactory( Step, Step::factory );


class BinIDWiseTask : public ParallelTask
{ mODTextTranslationClass(BinIDWiseTask);
public:
		BinIDWiseTask( Step& ro )
		    : step_( ro ), totalnr_( -1 ) { setName(ro.userName());  }

    uiString	uiMessage() const	{ return errmsg_; }
    uiString	uiNrDoneText() const	{ return tr("Positions done"); }

protected:
    bool	doWork(od_int64 start, od_int64 stop, int threadid )
		{
		    const TrcKeySampling hrg(
				step_.output_->sampling().hsamp_ );
		    BinID curbid = hrg.start_;

		    const int nrinls = mCast( int, start/hrg.nrCrl() );
		    const int nrcrls = mCast( int, start - nrinls*hrg.nrCrl() );
		    curbid.inl() += nrinls*hrg.step_.inl();
		    curbid.crl() += nrcrls*hrg.step_.crl();

		    for ( int idx=mCast(int,start); idx<=stop; idx++ )
		    {
			if ( !step_.computeBinID( curbid, threadid ) )
			    return false;

			addToNrDone( 1 );

			if ( idx>=stop )
			    break;

			if ( !shouldContinue() )
			    return false;

			curbid.crl() += hrg.step_.crl();
			if ( curbid.crl()>hrg.stop_.crl() )
			{
			    curbid.crl() = hrg.start_.crl();
			    curbid.inl() += hrg.step_.inl();
			    if ( curbid.inl()>hrg.stop_.inl() )
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
			const TrcKeySampling hrg(
				step_.output_->sampling().hsamp_ );
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
    setName( vr.name().getFullString() );
    web_ = chain_.getWeb();
    //Todo: Optimize connections, check for indentical steps using same inputs
}


ChainExecutor::~ChainExecutor()
{
    deepErase( epochs_ );
    if ( curepoch_ )
	delete curepoch_;
}


uiString ChainExecutor::errMsg() const
{ return errmsg_; }


#define mGetStep( var, id, errret ) \
Step* var = chain_.getStepFromID( id ); \
if ( !var ) \
{ \
    errmsg_ = mToUiStringTodo("Cannot find output step with id %1") \
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


bool ChainExecutor::setCalculationScope( const TrcKeySampling& hrg,
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

	    const RegularSeisDataPack* input = outputstep->getOutput();
	    const Step::OutputSlotID outputslot =
					inputconnections[idy].outputslotid_;
	    const int outputidx = outputstep->getOutputIdx( outputslot );

	    if ( !input->validComp(outputidx) )
	    {
		pErrMsg("Output is not available");
		return false;
	    }

	    currentstep->setInput( inputconnections[idy].inputslotid_, input );
	}

	TrcKeySampling stepoutputhrg;
	StepInterval<int> stepoutputzrg;

	chainexec_.computeComputationScope( currentstep->getID(), stepoutputhrg,
					    stepoutputzrg );

	TrcKeyZSampling csamp;
	csamp.hsamp_ = stepoutputhrg;
	csamp.zsamp_.start = SI().zStep()*stepoutputzrg.start;
	csamp.zsamp_.stop = SI().zStep()*stepoutputzrg.stop;
	RegularSeisDataPack* outcube = new RegularSeisDataPack( 0 );
	DPM( DataPackMgr::SeisID() ).addAndObtain( outcube );
	outcube->setSampling( csamp );
	if ( !outcube->addComponent( 0 ) )
	{
	    errmsg_ = "Cannot allocate enough memory.";
	    outcube->release();
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


const RegularSeisDataPack* ChainExecutor::Epoch::getOutput() const
{
    return steps_[steps_.size()-1] ? steps_[steps_.size()-1]->getOutput() : 0;
}


const RegularSeisDataPack* ChainExecutor::getOutput() const
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
    progressmeter_->skipProgress( false );
    curtask.setProgressMeter( progressmeter_ );
    curtask.enableWorkControl( true );

    //curtasklock_.unLock();

    if ( !curtask.execute() )
	mCleanUpAndRet( ErrorOccurred() )

    if ( epochs_.isEmpty() )		//we just executed the last one
	outputvolume_ = curepoch_->getOutput();

    //To prevent the overall chain progress display in between sub-tasks
    progressmeter_->skipProgress( true );

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
	epochs_.isEmpty() ? 0 : totalnrepochs_-epochs_.size();
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




uiString ChainExecutor::uiNrDoneText() const
{
    return curepoch_
	? curepoch_->getTask().uiNrDoneText()
	: tr("Positions done");
}


od_int64 ChainExecutor::totalNr() const
{ return 100; }


uiString ChainExecutor::uiMessage() const
{
    if ( !errmsg_.isEmpty() )
	return errmsg_;

    //Threads::Locker lckr( curtasklock_ );
    if ( curepoch_ )
	return curepoch_->getTask().uiMessage();

    return uiString::emptyString();
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


bool Chain::Connection::operator==( const Chain::Connection& b ) const
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


bool Chain::Connection::operator!=( const Chain::Connection& b ) const
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


bool Chain::addConnection( const Chain::Connection& c )
{
    if ( !validConnection(c) )
	return false;

    web_.getConnections().addIfNew( c );
    return true;
}


void Chain::removeConnection( const Chain::Connection& c )
{
    web_.getConnections() -= c;
}


void Chain::updateConnections()
{
    const Chain::Web oldweb = web_;
    web_.getConnections().erase();

    for ( int idx=1; idx<steps_.size(); idx++ )
    {
	Step* step = steps_[idx];
	if ( step->isInputPrevStep() )
	{
	    Step* prevstep = steps_[idx-1];
	    Chain::Connection connection( prevstep->getID(), 0,
		    step->getID(), step->getInputSlotID(0) );
	    addConnection( connection );
	}
	else
	{
	    TypeSet<Chain::Connection> conns;
	    oldweb.getConnections( step->getID(), true, conns );
	    for ( int cidx=0; cidx<conns.size(); cidx++ )
		addConnection( conns[cidx] );
	}
    }
}


bool Chain::validConnection( const Chain::Connection& c ) const
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
{ return steps_.validIdx(idx) ? steps_[idx] : 0; }


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
    updateConnections();
}


void Chain::removeStep( int sidx )
{
    if ( !steps_.validIdx(sidx) ) return;

    delete steps_.removeSingle( sidx );
    updateConnections();
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

    const uiString parseerror = tr("Parsing error");

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
	    errmsg_ = tr("Could not parse or set output slot.");
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
		errmsg_ = tr( "Cannot add connection %1").arg( toString(idx) );
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


uiString Chain::name() const
{
    PtrMan<IOObj>  ioobj = IOM().get( storageid_ );
    return !ioobj ? uiString::emptyString() :
	tr("Executing volume builder chain \'%1\'").arg(ioobj->name());
}


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


uiString Chain::errMsg() const
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
    for ( int idx=0; idx<inputs_.size(); idx++ )
	DPM( DataPackMgr::SeisID() ).release( inputs_[idx] );

    DPM( DataPackMgr::SeisID() ).release( output_ );
}


void Step::resetInput()
{
    for ( int idx=0; idx<inputs_.size(); idx++ )
	DPM( DataPackMgr::SeisID() ).release( inputs_[idx] );

    inputslotids_.erase();
    for ( int idx=0; idx<getNrInputs(); idx++ )
    {
	inputs_ += 0;
	inputslotids_ += idx;
    }
}


void Step::releaseData()
{
    DPM( DataPackMgr::SeisID() ).release( output_ );
    output_ = 0;

    resetInput();
}


Chain& Step::getChain()
{ return *chain_; }

const Chain& Step::getChain() const
{ return const_cast<Step*>(this)->getChain(); }


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
{ return isInputPrevStep() ? 1 : 0; }


int Step::getInputSlotID( int idx ) const
{
    if ( !isInputPrevStep() )
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


TrcKeySampling Step::getInputHRg( const TrcKeySampling& hr ) const
{ return hr; }


StepInterval<int>
    Step::getInputZRg( const StepInterval<int>& si ) const
{ return si; }


void Step::setInput( InputSlotID slotid, const RegularSeisDataPack* dc )
{
    if ( inputs_.isEmpty() )
	resetInput();

    const int idx = inputslotids_.indexOf( slotid );
    if ( !inputs_.validIdx(idx) )
	return;

    if ( inputs_[idx] )
	DPM( DataPackMgr::SeisID() ).release( inputs_[idx]->id() );
    inputs_.replace( idx, dc );
    if ( inputs_[idx] )
	DPM( DataPackMgr::SeisID() ).obtain( inputs_[idx]->id() );
}


const RegularSeisDataPack* Step::getInput( InputSlotID slotid ) const
{
    const int idx = inputslotids_.indexOf( slotid );
    return inputs_.validIdx(idx) ? inputs_[idx] : 0;
}


void Step::setOutput( OutputSlotID slotid, RegularSeisDataPack* dc,
		      const TrcKeySampling& hrg,
		      const StepInterval<int>& zrg )
{
    DPM( DataPackMgr::SeisID() ).release( output_ );
    output_ = dc;
    DPM( DataPackMgr::SeisID() ).addAndObtain( output_ );

    tks_ = hrg;
    zrg_ = zrg;
}


RegularSeisDataPack* Step::getOutput( OutputSlotID slotid )
{
    // TODO: implement using slotid
    return output_;
}


const RegularSeisDataPack* Step::getOutput( OutputSlotID slotid ) const
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

