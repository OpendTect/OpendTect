/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: prestackprocessor.cc,v 1.3 2007-04-10 21:57:03 cvskris Exp $";

#include "prestackprocessor.h"

#include "prestackgather.h"

namespace PreStack
{


mImplFactory( Processor, PF );


Processor::Processor()
    : output_( 0 )
    , input_( 0 )
{}


Processor::~Processor()
{
    if ( output_ ) output_->unRef();
    if ( input_ ) input_->unRef();
}


void Processor::setInput( const Gather* isg )
{
    if ( input_ ) input_->unRef();
    input_ = isg;
    if ( input_ ) input_->ref();
}


const Gather* Processor::getOutput() const
{ return output_; }


bool Processor::prepareWork()
{
    if ( !input_ ) return false;

    if ( output_ ) //TODO Make smarter and only set auxdata
    {
	output_->unRef();
	output_ = 0;
    }

    output_ = new Gather(*input_);
    output_->ref();

    return true;
}



int Processor::nrOffsets() const
{
    return input_->data().info().getSize( Gather::offsetDim() );
}


ProcessManager::ProcessManager()
    : input_( 0 )
    , output_( 0 )
{}


ProcessManager::~ProcessManager()
{
    deepErase( processors_ );
    if ( input_ ) input_->unRef();
    if ( output_ ) output_->unRef();
}


void ProcessManager::setInput( const Gather* isg )
{
    if ( input_ ) input_->unRef();
    input_ = isg;
    if ( input_ ) input_->ref();
}


bool ProcessManager::process(bool forceall)
{
    const Gather* curinput = input_;
    for ( int idx=0; idx<processors_.size(); idx++ )
    {
	processors_[idx]->setInput( curinput );
	if ( !processors_[idx]->prepareWork() || !processors_[idx]->execute() )
	    return false;

	curinput = processors_[idx]->getOutput();
    }

    if ( output_ ) output_->unRef();
    output_ = curinput;
    if ( output_ ) output_->ref();

    return true;
}


const Gather* ProcessManager::getOutput() const
{ return output_; }


void ProcessManager::addProcessor( Processor* sgp )
{ processors_ += sgp; }


int ProcessManager::nrProcessors() const
{ return processors_.size(); }


void ProcessManager::removeProcessor( int idx )
{ delete processors_.remove( idx ); }


void ProcessManager::swapProcessors( int i0, int i1 )
{ processors_.swap( i0, i1 ); }


Processor* ProcessManager::getProcessor( int idx )
{ return processors_[idx]; }


const Processor*
ProcessManager::getProcessor( int idx ) const
{ return processors_[idx]; }


}; //namespace
