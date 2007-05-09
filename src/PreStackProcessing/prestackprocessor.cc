/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: prestackprocessor.cc,v 1.4 2007-05-09 16:45:03 cvskris Exp $";

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
    if ( output_ ) DPM( DataPackMgr::FlatID ).release( output_->id() );
    if ( input_ ) DPM( DataPackMgr::FlatID ).release( input_->id() );
}


void Processor::setInput( DataPack::ID id )
{
    if ( input_ ) DPM( DataPackMgr::FlatID ).release( input_->id() );
    DataPack* dp = DPM( DataPackMgr::FlatID ).obtain( id, false );
    mDynamicCastGet( Gather*, gather, dp );
    if ( gather ) input_ = gather;
    else if ( dp ) DPM( DataPackMgr::FlatID ).release( dp->id() );
}


DataPack::ID Processor::getOutput() const
{ return output_ ? output_->id() : -1; }


bool Processor::prepareWork()
{
    if ( !input_ ) return false;

    if ( output_ )
    {
	DPM( DataPackMgr::FlatID ).release( output_->id() );
	output_ = 0;
    }

    output_ = new Gather(*input_);
    DPM( DataPackMgr::FlatID ).add( output_ );

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
    if ( output_ ) DPM( DataPackMgr::FlatID ).release( output_->id() );
    if ( input_ ) DPM( DataPackMgr::FlatID ).release( input_->id() );
}


void ProcessManager::setInput( DataPack::ID id )
{
    if ( input_ ) DPM( DataPackMgr::FlatID ).release( input_->id() );
    input_ = 0;
    DataPack* dp = DPM( DataPackMgr::FlatID ).obtain( id, false );
    mDynamicCastGet( Gather*, gather, dp );
    if ( gather ) input_ = gather;
    else if ( dp ) DPM( DataPackMgr::FlatID ).release( dp->id() );
}


bool ProcessManager::process(bool forceall)
{
    DataPack::ID curinput = input_->id();
    for ( int idx=0; idx<processors_.size(); idx++ )
    {
	processors_[idx]->setInput( curinput );
	if ( !processors_[idx]->prepareWork() || !processors_[idx]->execute() )
	    return false;

	curinput = processors_[idx]->getOutput();
    }

    if ( output_ ) DPM( DataPackMgr::FlatID ).release( output_->id() );
    output_ = 0;
    DataPack* dp = DPM( DataPackMgr::FlatID ).obtain( curinput, false );
    mDynamicCastGet( Gather*, gather, dp );
    if ( gather ) output_ = gather;
    else if ( dp ) DPM( DataPackMgr::FlatID ).release( dp->id() );

    return true;
}


DataPack::ID ProcessManager::getOutput() const
{ return output_ ? output_->id() : -1; }


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
