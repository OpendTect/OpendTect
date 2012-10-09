/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 14-6-1996
-*/

static const char* rcsID = "$Id$";

#include "executor.h"

#include "errh.h"
#include "oddirs.h"
#include "progressmeter.h"
#include "thread.h"
#include <iostream>


bool Executor::execute( std::ostream* strm, bool isfirst, bool islast,
		        int delaybetwnsteps )
{
    if ( !strm )
    {
	if ( !delaybetwnsteps ) return SequentialTask::execute();

	int rv = MoreToDo();
	while ( rv )
	{
	    rv = doStep();
	    if ( rv < 0 )
	    {
		const char* msg = message();
		if ( msg && *msg ) ErrMsg( msg );
		return false;
	    }
	    if ( delaybetwnsteps )
		Threads::sleep( delaybetwnsteps*0.001 );
	}
	return true;
    }

    std::ostream& stream = *strm;
    if ( isfirst )
	stream << GetProjectVersionName() << "\n\n";

    TextStreamProgressMeter progressmeter( *strm );
    setProgressMeter( &progressmeter );

    bool res = SequentialTask::execute();
    if ( !res )
	stream << "Error: " << message() << std::endl;

    setProgressMeter( 0 );

    if ( islast )
	stream << "\n\nEnd of process: '" << name() << "'" << std::endl;

    return res;
}


int Executor::doStep()
{
    prestep.trigger();
    const int res = SequentialTask::doStep();
    if ( res > 0 ) poststep.trigger();
    return res;
}


ExecutorGroup::ExecutorGroup( const char* nm, bool p, bool ownsexecs )
	: Executor( nm )
	, executors_( *new ObjectSet<Executor> )
	, currentexec_( 0 )
    	, parallel_( p )
    	, sumstart_( 0 )
        , ownsexecs_(ownsexecs)
{
}


ExecutorGroup::~ExecutorGroup()
{
    if ( ownsexecs_ )
	deepErase( executors_ );

    delete &executors_;
}


void ExecutorGroup::findNextSumStop()
{
    if ( !parallel_ )
    {
	for ( int idx=currentexec_+1; idx<executors_.size(); idx++ )
	{
	    BufferString nrdonetxt = executors_[idx]->nrDoneText();
	    BufferString msgtxt = executors_[idx]->message();
	    if ( nrdonetxt != executors_[idx-1]->nrDoneText() ||
		 msgtxt != executors_[idx-1]->message() )
	    {
		sumstop_ = idx-1;
		return;
	    }
	}
    }

    sumstop_ = executors_.size()-1;
}


void ExecutorGroup::add( Executor* n )
{
    executors_ += n;
    executorres_ += 1;

    findNextSumStop();
}


int ExecutorGroup::nextStep()
{
    const int nrexecs = executors_.size();
    if ( !nrexecs ) return Finished();

    int res = executorres_[currentexec_] = executors_[currentexec_]->doStep();
    if ( res == ErrorOccurred() )
	return ErrorOccurred();
    else if ( parallel_ || res==Finished() )
	res = goToNextExecutor() ? MoreToDo() : Finished();

    return res;
}


bool ExecutorGroup::goToNextExecutor()
{
    const int nrexecs = executors_.size();
    if ( !nrexecs ) return false;

    for ( int idx=0; idx<nrexecs; idx++ )
    {
	currentexec_++;
	if ( currentexec_==nrexecs )
	    currentexec_ = 0;

	if ( executorres_[currentexec_]>Finished() )
	{
	    if ( currentexec_>sumstop_ )
	    {
		sumstart_ = currentexec_;
		findNextSumStop();
	    }

	    return true;
	}
    }

    return false;
}


const char* ExecutorGroup::message() const
{
    return executors_.size() ? executors_[currentexec_]->message()
			    : Executor::message();
}


od_int64 ExecutorGroup::totalNr() const
{
    const int nrexecs = executors_.size();
    if ( !nrexecs ) return -1;

    od_int64 res = 0;
    for ( int idx=sumstart_; idx<=sumstop_; idx++ )
    {
	const od_int64 nr = executors_[idx]->totalNr();
	if ( nr<0 )
	    return -1;

	res += nr;
    }

    return res;
}


od_int64 ExecutorGroup::nrDone() const
{
    const int nrexecs = executors_.size();
    if ( nrexecs < 1 )
	return -1;

    od_int64 res = 0;
    for ( int idx=sumstart_; idx<=sumstop_; idx++ )
	res += executors_[idx]->nrDone();

    return res;
}


const char* ExecutorGroup::nrDoneText() const
{
    const char* txt = (const char*)nrdonetext_;
    if ( *txt ) return txt;

    if ( executors_.isEmpty() )
	return Executor::nrDoneText();

    return executors_[currentexec_]->nrDoneText();
}
