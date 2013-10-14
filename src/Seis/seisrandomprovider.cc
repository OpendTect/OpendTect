/*+
 ________________________________________________________________________
 
 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer
 Date:          June 2012
 ________________________________________________________________________
 
 -*/
static const char* rcsID mUsedVar = "$Id$";

#include "seisrandomprovider.h"

#include "ioman.h"
#include "seisread.h"
#include "threadwork.h"
#include "seistrctr.h"

SeisRandomProvider::SeisRandomProvider( const MultiID& mid )
    : reader_( 0 )
    , translator_( 0 )
    , wantedbids_( 0, false )
    , isreading_( false )
    , traceAvailable( this )
{
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
	return;
    
    reader_ = new SeisTrcReader( ioobj );
    translator_ = reader_->seisTranslator();
}


SeisRandomProvider::~SeisRandomProvider()
{
    Threads::WorkManager::twm().removeWork(
       mWMT( this, SeisRandomProvider, readTraces) );

    Threads::MutexLocker lock( lock_ );
    while ( isreading_ )
    {
	lock_.wait();
    }
    
    delete reader_;
}



void SeisRandomProvider::requestTrace( const BinID& bid )
{
    Threads::MutexLocker lock( lock_ );
    const BinIDValueSet::SPos pos = wantedbids_.find( bid );
    if ( pos.isValid() )
	return;
    
    wantedbids_.add( bid );
    
    lock.unLock();
    
    triggerWork();
}



bool SeisRandomProvider::readTraces()
{
    Threads::MutexLocker lock( lock_ );
    
    isreading_ = true;

    BinIDValueSet::SPos pos;
    if ( !wantedbids_.next( pos ) )
    {
	isreading_ = false;
	return true;
    }
    
    
    bool docontinue = true;
    while ( docontinue )
    {
	const BinID curbid = wantedbids_.getBinID( pos );

	lock.unLock();
	
	if ( translator_->goTo( curbid ) && reader_->get( curtrace_ ) )
	    traceAvailable.trigger();
	
	lock.lock();
	
	const BinIDValueSet::SPos oldpos = wantedbids_.find( curbid );
	if ( !oldpos.isValid() )
	{
	    pErrMsg( "Trace removed by someone. Should never happen");
	    isreading_ = false;
	    return false;
	}
	
	pos = oldpos;
	docontinue = wantedbids_.next( pos );
	
	wantedbids_.remove( oldpos );
    }
    
    isreading_ = false;
    lock_.signal( true );
    return true;
}


void SeisRandomProvider::readFinished( CallBacker* )
{
    triggerWork();
}


void SeisRandomProvider::triggerWork()
{
    Threads::MutexLocker lock( lock_ );

    if ( isreading_ )
	return;
    
    if ( wantedbids_.isEmpty() )
	return;

    lock.unLock();
    
    CallBack cb( mCB( this, SeisRandomProvider, readFinished ) );
    
    Threads::WorkManager::twm().addWork(
			    mWMT( this, SeisRandomProvider, readTraces),
			    &cb, -1, false, true );
}

