/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "prestackprocessor.h"

#include "iopar.h"
#include "keystrs.h"
#include "prestackgather.h"
#include "separstr.h"

namespace PreStack
{


mImplFactory( Processor, Processor::factory );


Processor::Processor( const char* nm )
    : ParallelTask( nm )
    , outputstepout_( 0, 0 )
{
    inputs_.allowNull( true ); 
    outputs_.allowNull( true );
    reset();
}


Processor::~Processor()
{
    freeArray( inputs_ );
    freeArray( outputs_ );
}


const BinID& Processor::getInputStepout() const
{ return outputstepout_; }


const BinID& Processor::getOutputStepout() const
{ return outputstepout_; }


bool Processor::reset()
{
    outputstepout_ = BinID(0,0);

    freeArray( inputs_ );
    freeArray( outputs_ );
    outputinterest_.erase();

    outputs_ += 0;
    outputinterest_ += false;
    inputs_ += 0;

    return true;
}


bool Processor::wantsInput( const BinID& bid ) const
{ 
    const int offset=getRelBidOffset( bid, outputstepout_ );
    return outputinterest_[offset]; 
}

void Processor::setInput( const BinID& relbid, DataPack::ID id )
{
    Gather* input = 0;
    mObtainDataPack( input, Gather*, DataPackMgr::FlatID(), id );

    const BinID inputstepout = getInputStepout();
    const int offset = getRelBidOffset( relbid, inputstepout );
    if ( offset>=inputs_.size() )
    {
	if ( input ) DPM( DataPackMgr::FlatID() ).release( input->id() );
	return;
    }

    if ( inputs_[offset] )
    {
	DPM( DataPackMgr::FlatID() ).release( inputs_[offset]->id() );
    }

    inputs_.replace( offset, input );
}


bool Processor::setOutputInterest( const BinID& relbid, bool yn )
{
    const BinID needestepout( abs(relbid.inl),abs(relbid.crl) );

    if ( needestepout.inl>outputstepout_.inl ||
	 needestepout.crl>outputstepout_.crl )
    {
	const BinID newstepout( mMAX(outputstepout_.inl,needestepout.inl),
		                mMAX(outputstepout_.crl,needestepout.crl) );
	mPSProcAddStepoutStep( outputs_, ObjectSet<Gather>,
			       outputstepout_, newstepout );
	mPSProcAddStepoutStep( inputs_, ObjectSet<Gather>,
			       outputstepout_, newstepout );
	mPSProcAddStepoutStep( outputinterest_, BoolTypeSet,
			       outputstepout_, newstepout );
	outputstepout_ = newstepout;
    }

    const int offset=getRelBidOffset( relbid,outputstepout_ );
    outputinterest_[offset] = yn;
    
    return true;
}


DataPack::ID Processor::getOutput( const BinID& relbid ) const
{
    const Gather* res = outputs_[getRelBidOffset(relbid,outputstepout_)];
    return res ? res->id() : DataPack::cNoID();
}


bool Processor::prepareWork()
{
    bool found = false;
    for ( int idx=inputs_.size()-1; idx>=0; idx-- )
	if ( inputs_[idx] ) { found = true; break; }

    if ( !found )
	return false;

    freeArray( outputs_ );

    BinID inputstepout = getInputStepout();
    for ( int idx=-outputstepout_.inl; idx<=outputstepout_.inl; idx++ )
    {
	for ( int idy=-outputstepout_.crl; idy<=outputstepout_.crl; idy++ )
	{
	    const BinID curpos( idx, idy );

	    const int outputoffset = getRelBidOffset(curpos,outputstepout_);

	    if ( outputinterest_[outputoffset] )
	    {
		const int inputoffset = getRelBidOffset(curpos,inputstepout);
		if ( !inputs_[inputoffset] )
		{
		    outputs_ += 0;
		    continue;
		}

		Gather* output = createOutputArray(*(inputs_[inputoffset]) );
		outputs_ += output;
		DPM( DataPackMgr::FlatID() ).addAndObtain( output );
	    }
	    else
		outputs_ += 0;
	}
    }

    return true;
}


Gather* Processor::createOutputArray( const Gather& input ) const
{ return new Gather(input); }



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
    return diff.inl*(stepout.crl*2+1)+diff.crl;
}


ProcessManager::ProcessManager()
    : setupChange( this )
{}


ProcessManager::~ProcessManager()
{
    deepErase( processors_ );
}



bool ProcessManager::reset()
{
    for ( int idx=0; idx<processors_.size(); idx++ )
	if ( !processors_[idx]->reset() )
	    return false;

    if ( !processors_.size() )
 	return true;

    BinID outputstepout( 0, 0 );
    return processors_[processors_.size()-1]->setOutputInterest(
	    						outputstepout, true );

}


BinID ProcessManager::getInputStepout() const
{
    if ( processors_.size() ) return processors_[0]->getInputStepout();
    return BinID( 0, 0 );
}


bool ProcessManager::wantsInput( const BinID& relbid ) const
{
    return processors_.size()
	? processors_[0]->wantsInput( relbid ) 
	: false;
}


void ProcessManager::setInput( const BinID& relbid, DataPack::ID id )
{
    if ( processors_.size() )
	processors_[0]->setInput( relbid, id );
}


bool ProcessManager::prepareWork()
{
    for ( int proc=processors_.size()-1; proc>0; proc-- )
    {
	const BinID inputstepout = processors_[proc]->getInputStepout();

	for ( int idz=-inputstepout.inl; idz<=inputstepout.inl; idz++ )
	{
	    for ( int idu=-inputstepout.crl;idu<=inputstepout.crl;idu++)
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
	    for ( int idx=-stepout.inl; idx<=stepout.inl; idx++ )
	    {
		for ( int idy=-stepout.crl; idy<=stepout.crl; idy++ )
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


DataPack::ID ProcessManager::getOutput() const
{
    return processors_.size()
	? processors_[processors_.size()-1]->getOutput(BinID(0,0))
	: DataPack::cNoID();
}


void ProcessManager::addProcessor( Processor* sgp )
{
    processors_ += sgp;
    setupChange.trigger();
}


int ProcessManager::nrProcessors() const
{ return processors_.size(); }


void ProcessManager::removeProcessor( int idx )
{
    delete processors_.remove( idx );
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


Processor* ProcessManager::getProcessor( int idx )
{ return processors_[idx]; }


const Processor*
ProcessManager::getProcessor( int idx ) const
{ return processors_[idx]; }


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
    if ( !par.get( sKeyNrProcessors(), nrprocessors ) )
	return false;

    for ( int idx=0; idx<nrprocessors; idx++ )
    {
	const BufferString idxstr( "", idx );
	BufferString name;
	PtrMan<IOPar> steppar = par.subselect( idxstr.buf() );
	if ( !steppar || !steppar->get( sKey::Name(), name ) )
	{
	    errmsg_ = "Could not find name for processing step ";
	    errmsg_ += idx;
	    errmsg_ += ".";
	    return false;
	}

	Processor* proc = Processor::factory().create( name.buf() );
	if ( !proc || proc->errMsg() || !proc->usePar( *steppar ) )
	{
	    errmsg_ = "Could not parse processing step ";
	    errmsg_ += name.buf();
	    errmsg_ += ".";
	    if ( proc && proc->errMsg() )
	    {
		errmsg_ += FileMultiString::separatorStr();
		errmsg_ += proc->errMsg();
	    }
	    else
	    {
		errmsg_ += " Perhaps all plugins are not loaded?";
	    }

	    delete proc;
	    return false;
	}

	addProcessor( proc );
    }

    stopper.disable();
    setupChange.trigger();

    return true;
}


void Processor::freeArray( ObjectSet<Gather>& arr )
{
    for ( int idx=0; idx<arr.size(); idx++ )
	DPM( DataPackMgr::FlatID() ).release( arr[idx] );

    arr.erase();
}


}; //namespace
