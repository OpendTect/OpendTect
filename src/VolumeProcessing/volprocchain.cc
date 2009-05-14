/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : October 2006
-*/

static const char* rcsID = "$Id: volprocchain.cc,v 1.10 2009-05-14 19:58:42 cvskris Exp $";

#include "volprocchain.h"

#include "iopar.h"
#include "keystrs.h"
#include "survinfo.h"

namespace VolProc
{

mImplFactory1Param( Step, Chain&, PS );


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

		    const int nrinls = start/hrg.nrCrl();
		    const int nrcrls = start - nrinls*hrg.nrCrl();
		    curbid.inl += nrinls*hrg.step.inl;
		    curbid.crl += nrcrls*hrg.step.crl;

		    for ( int idx=start; idx<=stop; idx++ )
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

    Step&	step_;
    mutable int		totalnr_;
};


ChainExecutor::ChainExecutor( Chain& vr )
    : Executor( "Volume processing" )
    , chain_( vr )
    , cubeoutput_( 0 )
    , isok_( true )
    , tmpres_( 0 )
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
    if ( cubeoutput_ ) cubeoutput_->unRef();
    delete curtask_;
}


const char* ChainExecutor::errMsg() const
{ return errmsg_.isEmpty() ? 0 : errmsg_.buf(); }


bool ChainExecutor::setCalculationScope( 
	Attrib::DataCubes* output )
{
    if ( !output ) return false;

    hrg_.start.inl = output->inlsampling.start;
    hrg_.start.crl = output->crlsampling.start;
    hrg_.step.inl = output->inlsampling.step;
    hrg_.step.crl = output->crlsampling.step;
    hrg_.stop.inl = output->inlsampling.atIndex( output->getInlSz()-1 );
    hrg_.stop.crl = output->crlsampling.atIndex( output->getCrlSz()-1 );

    zrg_.start = output->z0;
    zrg_.step = 1;
    zrg_.stop = output->z0 + output->getZSz()-1;
    chain_.setZSampling(
	    SamplingData<float>( output->z0*output->zstep, output->zstep ),
	    SI().zIsTime() );

    if ( cubeoutput_ ) 
	cubeoutput_->unRef();

    cubeoutput_ = output;
    cubeoutput_->ref();

    currentstep_ = 0;
    return true;
}


int ChainExecutor::nextStep()
{
    if ( !isok_ )
	return ErrorOccurred();

    if ( !prepareNewStep() )
	return errmsg_.isEmpty() ? Finished() : ErrorOccurred();

    if ( !curtask_->execute() )
	return ErrorOccurred();

    currentstep_++;
    return currentstep_<steps_.size() ? MoreToDo() : Finished();
}


bool ChainExecutor::prepareNewStep()
{
    errmsg_.setEmpty();
    if ( currentstep_>=steps_.size() )
	return false;

    HorSampling hrg( hrg_ ); 
    StepInterval<int> zrg = zrg_;
    for ( int idx=steps_.size()-1; idx>currentstep_; idx-- )
    {
	hrg = chain_.getStep( idx )->getInputHRg( hrg );
	zrg = chain_.getStep( idx )->getInputZRg( zrg );
    }

    tmpres_ = curinput_;
    if ( !currentstep_ ) { curinput_ = 0; }
    else
    {
	curinput_ = curoutput_;
	curoutput_ = 0;

	if ( tmpres_ )
	{
	    curoutput_ = tmpres_;
	    tmpres_ = 0;
	}
    }

    if ( currentstep_==steps_.size()-1 )
	curoutput_ = cubeoutput_;
    else if ( !curoutput_ )
	curoutput_ = new Attrib::DataCubes();

    const StepInterval<int> inlrg = hrg.inlRange();
    const StepInterval<int> crlrg = hrg.crlRange();

    const int nrinl = inlrg.nrSteps()+1;
    const int nrcrl = crlrg.nrSteps()+1;
    const int nrz = zrg.nrSteps()+1;

    if ( curoutput_->getInlSz()!=nrinl || curoutput_->getCrlSz()!=nrcrl ||
	 curoutput_->getZSz()!=nrz )
	curoutput_->setSize( nrinl, nrcrl, nrz );

    curoutput_->inlsampling.start = inlrg.start;
    curoutput_->inlsampling.step = inlrg.step;
    curoutput_->crlsampling.start = crlrg.start;
    curoutput_->crlsampling.step = crlrg.step;
    curoutput_->crlsampling.step = crlrg.step;
    curoutput_->z0 = zrg.start;
    curoutput_->zstep = chain_.getZSampling().step;

    if ( !curoutput_->nrCubes() && !curoutput_->addCube( mUdf(float), false ) )
    {
	errmsg_ = "Cannot allocate enough memory.";
	return false;
    }
    else
	curoutput_->setValue( 0, mUdf(float) );

    steps_[currentstep_]->setOutput( curoutput_ );
    if ( steps_[currentstep_]->setInput( curinput_ ) )
	curinput_ = 0;

    Threads::MutexLocker lock( curtasklock_ );
    if ( curtask_ ) delete curtask_;
    curtask_ = steps_[currentstep_]->createTask();
    if ( !curtask_ )
    {
	errmsg_ = "Cannot create task";
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
    Threads::MutexLocker lock( curtasklock_ );
    if ( curtask_ )
	return curtask_->message();

    return 0;
}


Chain::Chain()
    : zsampling_( SI().zRange(true).start, SI().zRange(true).step )
{}


Chain::~Chain()
{ deepErase( steps_ ); }


void Chain::setZSampling( const SamplingData<float>& nsd, bool zit )
{
    zsampling_ = nsd;
    zit_ = zit;
}


const SamplingData<float>& Chain::getZSampling() const
{ return zsampling_; }


int Chain::nrSteps() const
{ return steps_.size(); }


Step* Chain::getStep(int idx)
{ return steps_[idx]; }


int Chain::indexOf(const Step* r) const
{ return steps_.indexOf( r ); }


void Chain::addStep( Step* r )
{ steps_ += r; }


void Chain::insertStep( int idx, Step* r )
{ steps_.insertAt( r, idx ); }


void Chain::swapSteps( int o1, int o2 )
{
    steps_.swap( o1, o2 );
}


void Chain::removeStep( int idx )
{
    delete steps_.remove( idx );
}


const VelocityDesc* Chain::getVelDesc() const
{
    for ( int idx=steps_.size(); idx>=0; idx-- )
    {
	if ( steps_[idx]->getVelDesc() )
	    return steps_[idx]->getVelDesc();
    }

    return 0;
}


void Chain::fillPar(IOPar& par) const
{
    par.set( sKeyNrSteps(), steps_.size() );
    for ( int idx=0; idx<steps_.size(); idx++ )
    {
	IOPar oppar;
	oppar.set( sKeyStepType(), steps_[idx]->type() );
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

	Step* step = PS().create( type.buf(), *this );
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
{ return errmsg_.isEmpty() ? 0 : errmsg_.buf(); }


Step::Step( Chain& chain )
    : chain_( chain )
    , output_( 0 )
    , input_( 0 )
    , enabled_( true )
{}


Step::~Step()
{
    if ( output_ ) output_->unRef();
    if ( input_ ) input_->unRef();
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


void Step::setOutput( Attrib::DataCubes* ni )
{
    if ( output_ ) output_->unRef();
    output_ = ni;
    if ( output_ ) output_->ref();
}


void Step::fillPar( IOPar& par ) const
{
    par.setYN( sKeyEnabled(), enabled_ );
    if ( !username_.isEmpty() )
	par.set( sKey::Name, username_.buf() );
}


bool Step::usePar( const IOPar& par )
{
    par.getYN( sKeyEnabled(), enabled_ );
    username_.empty();
    par.get( sKey::Name, username_ );
    return true;
}


Task* Step::createTask()
{
    if ( areSamplesIndependent() && prefersBinIDWise() )
	return new BinIDWiseTask( *this );

    return 0;
}


}; //namespace
