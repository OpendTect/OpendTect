/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : October 2006
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "volprocchain.h"

#include "iopar.h"
#include "keystrs.h"
#include "survinfo.h"

namespace VolProc
{

mImplFactory( Step, Step::factory );


class BinIDWiseTask : public ParallelTask
{
public:
    		BinIDWiseTask( Step& ro )
		    : step_( ro ), totalnr_( -1 )			{}

protected:
    bool	doWork(od_int64 start, od_int64 stop, int threadid )
		{
		    const HorSampling hrg( step_.output_->cubeSampling().hrg );
		    BinID curbid = hrg.start;

		    const int nrinls = mCast( int, start/hrg.nrCrl() );
		    const int nrcrls = mCast( int, start - nrinls*hrg.nrCrl() );
		    curbid.inl += nrinls*hrg.step.inl;
		    curbid.crl += nrcrls*hrg.step.crl;

		    for ( int idx=mCast(int,start); idx<=stop; idx++ )
		    {
			if ( !step_.computeBinID( curbid, threadid ) )
			    return false;

			addToNrDone( 1 );

			if ( idx>=stop )
			    break;

			if ( !shouldContinue() )
			    return false;

			curbid.crl += hrg.step.crl;
			if ( curbid.crl>hrg.stop.crl )
			{
			    curbid.crl = hrg.start.crl;
			    curbid.inl += hrg.step.inl;
			    if ( curbid.inl>hrg.stop.inl )
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

    bool	doPrepare(int nrthreads)
		{
		    return step_.prepareComp( nrthreads );
		}

    Step&		step_;
    mutable int		totalnr_;
};


ChainExecutor::ChainExecutor( Chain& vr )
    : Executor( "Volume processing" )
    , chain_( vr )
    , isok_( true )
    , curinput_( 0 )
    , curoutput_( 0 )
    , zrg_( 0, 0, 0 )
    , curtask_( 0 )
{
    for ( int idx=0; idx<chain_.nrSteps(); idx++ )
    {
	Step* step = chain_.getStep( idx );
	if ( !step->enabled() )
	    continue;

	steps_ += step;
    } 
}


ChainExecutor::~ChainExecutor()
{
    delete curtask_;
}


const char* ChainExecutor::errMsg() const
{ return errmsg_.str(); }


bool ChainExecutor::setCalculationScope( const CubeSampling& cs )
{
    hrg_ = cs.hrg;

    zrg_.start = mNINT32( cs.zrg.start/cs.zrg.step );
    zrg_.stop = mNINT32( cs.zrg.stop/cs.zrg.step );
    zrg_.step = 1;

    chain_.setZStep( cs.zrg.step, SI().zIsTime() );

    currentstep_ = 0;
    return true;
}


const Attrib::DataCubes* ChainExecutor::getOutput() const
{ return curoutput_; }


int ChainExecutor::nextStep()
{
    if ( !isok_ )
	return ErrorOccurred();

    if ( !prepareNewStep() )
	return errmsg_.isEmpty() ? Finished() : ErrorOccurred();

    if ( !curtask_->execute() )
	return ErrorOccurred();

    currentstep_++;
    if ( curtask_ ) delete curtask_;	//Avoids screwing up progressmeters
    curtask_ = 0;
    return currentstep_<steps_.size() ? MoreToDo() : Finished();
}


bool ChainExecutor::prepareNewStep()
{
    errmsg_.setEmpty();
    if ( currentstep_>=steps_.size() )
	return false;

    const HorSampling survhrg = SI().sampling( false ).hrg;
    const float zstep = chain_.getZStep();
    const Interval<int> survzrg( mNINT32(SI().zRange(false).start/zstep),
	    			 mNINT32(SI().zRange(false).stop/zstep) );

    HorSampling hrg( hrg_ ); 
    StepInterval<int> zrg = zrg_;
    for ( int idx=steps_.size()-1; idx>currentstep_; idx-- )
    {
	hrg = chain_.getStep( idx )->getInputHRg( hrg );
	zrg = chain_.getStep( idx )->getInputZRg( zrg );

	hrg.limitTo( survhrg );
	zrg.limitTo( survzrg );
    }

    RefMan<Attrib::DataCubes> tmpres = curinput_;
    if ( !currentstep_ )
    {
	curinput_ = 0;
    }
    else
    {
	curinput_ = curoutput_;
	curoutput_ = 0;

	chain_.getStep( currentstep_-1 )->releaseData();
    }

    const StepInterval<int> inlrg = hrg.inlRange();
    const StepInterval<int> crlrg = hrg.crlRange();

    const int nrinl = inlrg.nrSteps()+1;
    const int nrcrl = crlrg.nrSteps()+1;
    const int nrz = zrg.nrSteps()+1;

    bool initoutput = false;

    if ( curinput_ && steps_[currentstep_]->canInputAndOutputBeSame() )
	curoutput_ = curinput_;
    else if ( tmpres && tmpres!=curinput_ )
    {
	initoutput = true;
	curoutput_ = tmpres;
	tmpres = 0;
    }
    else if ( !curoutput_ )
    {
	initoutput = true;
	curoutput_ = new Attrib::DataCubes();
    }


    if ( initoutput )
    {
	curoutput_->inlsampling_.start = inlrg.start;
	curoutput_->inlsampling_.step = inlrg.step;
	curoutput_->crlsampling_.start = crlrg.start;
	curoutput_->crlsampling_.step = crlrg.step;
	curoutput_->crlsampling_.step = crlrg.step;
	curoutput_->z0_ = zrg.start;
	curoutput_->zstep_ = chain_.getZStep();

	if ( curoutput_->getInlSz()!=nrinl || curoutput_->getCrlSz()!=nrcrl ||
	     curoutput_->getZSz()!=nrz )
	    curoutput_->setSize( nrinl, nrcrl, nrz );
    }

    if ( !curoutput_->nrCubes() && !curoutput_->addCube(mUdf(float),0) )
    {
	errmsg_ = "Cannot allocate enough memory.";
	return false;
    }

    steps_[currentstep_]->setOutput( curoutput_, inlrg, crlrg, zrg );
    if ( steps_[currentstep_]->setInput( curinput_ ) )
	curinput_ = 0;

    Threads::MutexLocker lock( curtasklock_ );
    if ( curtask_ ) delete curtask_;
    curtask_ = steps_[currentstep_]->createTask();
    if ( !curtask_ )
    {
	const char* steperr = steps_[currentstep_]->errMsg();
	if ( steperr )
	{
	    errmsg_ = steps_[currentstep_]->userName();
	    errmsg_ += ": ";
	    errmsg_ += steperr;
	}
	else
	    errmsg_ = "Cannot create task.";

	return false;
    }

    curtask_->setProgressMeter( progressmeter_ );
    curtask_->enableNrDoneCounting( true );
    curtask_->enableWorkControl( true );

    return true;
}


void ChainExecutor::controlWork( Task::Control ctrl )
{
    Task::controlWork( ctrl );

    Threads::MutexLocker lock( curtasklock_ );
    if ( curtask_ )
	curtask_->controlWork( ctrl );
}


od_int64 ChainExecutor::nrDone() const
{
    Threads::MutexLocker lock( curtasklock_ );
    if ( curtask_ )
	return curtask_->nrDone();

    return -1;
}


od_int64 ChainExecutor::totalNr() const
{
    Threads::MutexLocker lock( curtasklock_ );
    if ( curtask_ )
	return curtask_->totalNr();

    return -1;
}


const char* ChainExecutor::message() const
{
    if ( !errmsg_.isEmpty() )
	return errmsg_;

    Threads::MutexLocker lock( curtasklock_ );
    if ( curtask_ )
	return curtask_->message();

    return 0;
}


Chain::Chain()
    : zstep_( SI().zRange(true).step )
    , zit_( SI().zIsTime() )
{}


Chain::~Chain()
{ deepErase( steps_ ); }


int Chain::nrSteps() const
{ return steps_.size(); }


Step* Chain::getStep(int idx)
{ return steps_[idx]; }


int Chain::indexOf(const Step* r) const
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
    {
	if ( steps_[idx]->getVelDesc() )
	    return steps_[idx]->getVelDesc();
    }

    return 0;
}


bool Chain::areSamplesIndependent() const
{
    for ( int idx=steps_.size()-1; idx>=0; idx-- )
    {
	if ( !steps_[idx]->areSamplesIndependent() )
	    return false;
    }

    return true;
}


void Chain::fillPar(IOPar& par) const
{
    par.set( sKeyNrSteps(), steps_.size() );
    for ( int idx=0; idx<steps_.size(); idx++ )
    {
	IOPar oppar;
	oppar.set( sKeyStepType(), steps_[idx]->factoryKeyword() );
	steps_[idx]->fillPar( oppar );

	par.mergeComp( oppar, toString(idx) );
    }
}


bool Chain::usePar( const IOPar& par )
{
    deepErase( steps_ );

    const char* parseerror = "Parsing error";

    int nrsteps;
    if ( !par.get( sKeyNrSteps(), nrsteps ) )
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
	    errmsg_ = "Cannot create Volume processing step ";
	    errmsg_ += type.buf();
	    errmsg_ += ". Perhaps all plugins are not loaded?";
	    return false;
	}

	if ( !step->usePar( *steppar ) )
	{
	    errmsg_ = "Cannot parse Volume Processing's parameters: ";
	    errmsg_ += step->errMsg();
	    delete step;
	    return false;
	}

	addStep( step );
    }

    return true;
}


void Chain::setStorageID( const MultiID& mid )
{ storageid_=mid; }


const char* Chain::errMsg() const
{ return errmsg_.str(); }


Step::Step()
    : chain_( 0 )
    , output_( 0 )
    , input_( 0 )
    , enabled_( true )
{}


Step::~Step()
{
    releaseData();
}


void Step::releaseData()
{
    if ( output_ ) output_->unRef();
    if ( input_ ) input_->unRef();
    output_ = 0;
    input_ = 0;
}


void Step::enable( bool yn )
{ enabled_ = yn; }


bool Step::enabled() const
{ return enabled_; }


const char* Step::userName() const
{ return username_.isEmpty() ? 0 : username_.buf(); }


void Step::setUserName( const char* nm )
{ username_ = nm; }


HorSampling Step::getInputHRg(const HorSampling& hr) const
{ return hr; }


StepInterval<int>
Step::getInputZRg(const StepInterval<int>& si) const
{ return si; }


bool Step::setInput( const Attrib::DataCubes* ni )
{
    if ( input_ ) input_->unRef();
    input_ = ni;
    if ( input_ ) input_->ref();

    return false;
}


 void Step::setOutput( Attrib::DataCubes* ni,
 	const StepInterval<int>& inlrg,
 	const StepInterval<int>& crlrg,
 	const StepInterval<int>& zrg) 
{
    if ( output_ ) output_->unRef();
    output_ = ni;
    if ( output_ ) output_->ref();

    hrg_.set( inlrg, crlrg );
    zrg_ = zrg;
}


void Step::fillPar( IOPar& par ) const
{
    par.setYN( sKeyEnabled(), enabled_ );
    if ( !username_.isEmpty() )
	par.set( sKey::Name(), username_.buf() );
}


bool Step::usePar( const IOPar& par )
{
    par.getYN( sKeyEnabled(), enabled_ );
    username_.empty();
    par.get( sKey::Name(), username_ );
    return true;
}


Task* Step::createTask()
{
    if ( areSamplesIndependent() && prefersBinIDWise() )
	return new BinIDWiseTask( *this );

    return 0;
}


}; //namespace
