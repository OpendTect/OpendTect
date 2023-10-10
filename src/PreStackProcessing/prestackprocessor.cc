/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackprocessor.h"

#include "iopar.h"

namespace PreStack
{

mImplFactory( Processor, Processor::factory );

Processor::Processor( const char* nm )
    : ParallelTask(nm)
    , outputstepout_(0,0)
{
    inputs_.setNullAllowed();
    outputs_.setNullAllowed();
    reset();
}


Processor::~Processor()
{}


const BinID& Processor::getInputStepout() const
{ return outputstepout_; }


const BinID& Processor::getOutputStepout() const
{ return outputstepout_; }


bool Processor::reset( bool force )
{
    outputstepout_ = BinID::noStepout();

    freeArray( inputs_ );
    freeArray( outputs_ );
    outputinterest_.erase();

    outputs_ += nullptr;
    outputinterest_ += false;
    inputs_ += nullptr;

    return true;
}


bool Processor::wantsInput( const BinID& bid ) const
{
    const int offset=getRelBidOffset( bid, outputstepout_ );
    return outputinterest_[offset];
}


void Processor::setInput( const BinID& relbid, DataPackID id )
{
    auto input = DPM(DataPackMgr::FlatID()).get<Gather>( id );
    setInput( relbid, input );
}


void Processor::setInput( const BinID& relbid, Gather* input )
{
    const BinID inputstepout = getInputStepout();
    const int offset = getRelBidOffset( relbid, inputstepout );
    if ( offset>=inputs_.size() )
	return;

    inputs_.replace( offset, input );
}


void Processor::setInput( const BinID& relbid, const Gather* input )
{
    setInput( relbid, const_cast<Gather*>(input) );
}


bool Processor::setOutputInterest( const BinID& relbid, bool yn )
{
    const BinID needestepout( abs(relbid.inl()),abs(relbid.crl()) );

    if ( needestepout.inl()>outputstepout_.inl() ||
	 needestepout.crl()>outputstepout_.crl() )
    {
	const BinID newstepout( mMAX(outputstepout_.inl(),needestepout.inl()),
		                mMAX(outputstepout_.crl(),needestepout.crl()) );
	mPSProcAddStepoutStep( outputs_, ObjectSet<Gather>,
			       outputstepout_, newstepout );
	mPSProcAddStepoutStep( inputs_, ObjectSet<Gather>,
			       outputstepout_, newstepout );
	mPSProcAddStepoutStep( outputinterest_, BoolTypeSet,
			       outputstepout_, newstepout );
	outputstepout_ = newstepout;
    }

    const int offset = getRelBidOffset( relbid,outputstepout_ );
    outputinterest_[offset] = yn;

    return true;
}


DataPackID Processor::getOutputID( const BinID& relbid ) const
{
    const Gather* res = getOutput( relbid );
    return res ? res->id() : DataPack::cNoID();
}


ConstRefMan<Gather> Processor::getOutput( const BinID& relbid ) const
{
    return outputs_[getRelBidOffset(relbid,outputstepout_)];
}


bool Processor::prepareWork()
{
    bool found = false;
    for ( int idx=inputs_.size()-1; idx>=0; idx-- )
    {
	if ( inputs_[idx] )
	{
	    found = true;
	    break;
	}
    }

    if ( !found && usesPreStackInput() )
	return false;

    freeArray( outputs_ );

    BinID inputstepout = getInputStepout();
    for ( int idx=-outputstepout_.inl(); idx<=outputstepout_.inl(); idx++ )
    {
	for ( int idy=-outputstepout_.crl(); idy<=outputstepout_.crl(); idy++ )
	{
	    const BinID curpos( idx, idy );

	    const int outputoffset = getRelBidOffset(curpos,outputstepout_);

	    if ( outputinterest_[outputoffset] )
	    {
		const int inputoffset = getRelBidOffset(curpos,inputstepout);
		if ( !inputs_[inputoffset] )
		{
		    outputs_ += nullptr;
		    continue;
		}

		Gather* output = createOutputArray(*(inputs_[inputoffset]) );
		outputs_ += output;
		DPM( DataPackMgr::FlatID() ).add( output );
	    }
	    else
		outputs_ += nullptr;
	}
    }

    return true;
}


Gather* Processor::createOutputArray( const Gather& input ) const
{
    return new Gather( input );
}


int Processor::nrOffsets() const
{
    int max = 0;
    for ( int idx=inputs_.size()-1; idx>=0; idx-- )
    {
	if ( !inputs_[idx] )
	    continue;

	const int nroffsets = inputs_[idx]->size( Gather::offsetDim()==0 );

	max = mMAX(max,nroffsets);
    }

    return max;
}


int Processor::getRelBidOffset( const BinID& relbid, const BinID& stepout )
{
    const BinID start = -stepout;
    const BinID diff = relbid - start;
    return diff.inl()*(stepout.crl()*2+1)+diff.crl();
}


ProcessManager::ProcessManager()
    : setupChange( this )
{}


ProcessManager::~ProcessManager()
{
    deepErase( processors_ );
}



bool ProcessManager::reset( bool force )
{
    for ( int idx=0; idx<processors_.size(); idx++ )
    {
	if ( !processors_[idx]->reset(force) )
	    return false;
    }

    if ( processors_.isEmpty() )
	return true;

    BinID outputstepout( 0, 0 );
    return processors_.last()->setOutputInterest( outputstepout, true );

}


bool ProcessManager::needsPreStackInput() const
{
    return !processors_.isEmpty() ? processors_[0]->usesPreStackInput() : false;
}


BinID ProcessManager::getInputStepout() const
{
    return !processors_.isEmpty() ? processors_[0]->getInputStepout()
				  : BinID::noStepout();
}


bool ProcessManager::wantsInput( const BinID& relbid ) const
{
    return !processors_.isEmpty() ? processors_[0]->wantsInput(relbid) : false;
}


void ProcessManager::setInput( const BinID& relbid, DataPackID id )
{
    if ( !processors_.isEmpty() )
	processors_[0]->setInput( relbid, id );
}


void ProcessManager::setInput( const BinID& relbid, Gather* input )
{
    if ( !processors_.isEmpty() )
	processors_[0]->setInput( relbid, input );
}


void ProcessManager::setInput( const BinID& relbid, const Gather* input )
{
    if ( !processors_.isEmpty() )
	processors_[0]->setInput( relbid, input );
}


bool ProcessManager::prepareWork()
{
    for ( int proc=processors_.size()-1; proc>0; proc-- )
    {
	const BinID inputstepout = processors_[proc]->getInputStepout();

	for ( int idz=-inputstepout.inl(); idz<=inputstepout.inl(); idz++ )
	{
	    for ( int idu=-inputstepout.crl();idu<=inputstepout.crl();idu++)
	    {
		const BinID relinput(idz,idu);
		if ( !processors_[proc]->wantsInput( relinput ) )
		    continue;

		if ( !processors_[proc-1]->setOutputInterest( relinput, true ) )
		    return false;
	    }
	}
    }

    return true;
}


bool ProcessManager::process()
{
    for ( int proc=0; proc<processors_.size(); proc++ )
    {
	if ( proc )
	{
	    const BinID stepout = processors_[proc]->getInputStepout();
	    for ( int idx=-stepout.inl(); idx<=stepout.inl(); idx++ )
	    {
		for ( int idy=-stepout.crl(); idy<=stepout.crl(); idy++ )
		{
		    const BinID relbid( idx,idy );
		    processors_[proc]->setInput( relbid,
			    processors_[proc-1]->getOutput( relbid ));

		}
	    }
	}

	if ( !processors_[proc]->prepareWork() || !processors_[proc]->execute())
	    return false;
    }

    return true;
}


DataPackID ProcessManager::getOutputID() const
{
    if ( processors_.isEmpty() )
	return DataPack::cNoID();

    return processors_.last()->getOutputID( BinID::noStepout() );
}


ConstRefMan<Gather> ProcessManager::getOutput() const
{
    if ( processors_.isEmpty() )
	return nullptr;

    return processors_.last()->getOutput( BinID::noStepout() );
}


void ProcessManager::addProcessor( Processor* sgp )
{
    processors_ += sgp;
    setupChange.trigger();
}


int ProcessManager::nrProcessors() const
{
    return processors_.size();
}


void ProcessManager::removeProcessor( int idx )
{
    delete processors_.removeSingle( idx );
    setupChange.trigger();
}


void ProcessManager::swapProcessors( int i0, int i1 )
{
    processors_.swap( i0, i1 );
    setupChange.trigger();
}


void ProcessManager::removeAllProcessors()
{
    deepErase( processors_ );
    setupChange.trigger();
}


int ProcessManager::indexOf( const Processor* proc ) const
{
    return proc ? processors_.indexOf( proc ) : -1;
}


Processor* ProcessManager::getProcessor( int idx )
{
    return processors_[idx];
}


const Processor* ProcessManager::getProcessor( int idx ) const
{
    return processors_[idx];
}


void ProcessManager::fillPar( IOPar& par ) const
{
    par.set( sKeyNrProcessors(), processors_.size() );
    for ( int idx=0; idx<processors_.size(); idx++ )
    {
	IOPar procpar;
	procpar.set( sKey::Name(), processors_[idx]->name() );
	processors_[idx]->fillPar( procpar );

	const BufferString idxstr( "", idx );
	par.mergeComp( procpar, idxstr.buf() );
    }
}


bool ProcessManager::usePar( const IOPar& par )
{
    NotifyStopper stopper( setupChange );

    removeAllProcessors();

    int nrprocessors;
    if ( !par.get(sKeyNrProcessors(),nrprocessors) )
	return false;

    for ( int idx=0; idx<nrprocessors; idx++ )
    {
	const BufferString idxstr( "", idx );
	BufferString name;
	PtrMan<IOPar> steppar = par.subselect( idxstr.buf() );
	if ( !steppar || !steppar->get( sKey::Name(), name ) )
	{
	    errmsg_ = tr( "Could not find name for processing step %1." )
			.arg(toString(idx));
	    return false;
	}

	Processor* proc = Processor::factory().create( name.buf() );
	if ( !proc || proc->errMsg().isSet() || !proc->usePar( *steppar ) )
	{
	    errmsg_ = tr("Could not parse processing step %1.").arg(name);
	    if ( !proc || proc->errMsg().isSet() )
		errmsg_.append( tr( "\nAre all plugins loaded?" ) );
	    else
		errmsg_.append( proc->errMsg() );

	    delete proc;
	    return false;
	}

	addProcessor( proc );
    }

    stopper.enableNotification();
    setupChange.trigger();

    return true;
}


void Processor::freeArray( RefObjectSet<Gather>& arr )
{
    arr.erase();
}


void Processor::freeArray( ObjectSet<Gather>& arr )
{
    arr.erase();
}

} // namespace PreStack
