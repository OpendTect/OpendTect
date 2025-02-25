/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "executor.h"

#include "genc.h"
#include "oddirs.h"
#include "progressmeter.h"
#include "thread.h"
#include "od_ostream.h"


bool Executor::goImpl( od_ostream* strm, bool isfirst, bool islast, int delay )
{
    if ( !strm || strm == &od_ostream::nullStream() )
    {
	if ( !delay ) return SequentialTask::execute();

	int rv = MoreToDo();
	while ( rv )
	{
	    rv = doStep();
	    if ( rv < 0 )
	    {
		const uiString msg = uiMessage();
		if ( !msg.isEmpty() ) ErrMsg( msg.getFullString() );
		return false;
	    }
	    if ( delay )
		Threads::sleep( delay*0.001 );
	}
	return true;
    }

    if ( isfirst )
	*strm << GetProjectVersionName() << "\n\n";

    PtrMan<ProgressMeter> pm;
    if ( useSimpleMeter() )
	pm = new SimpleTextStreamProgressMeter( *strm, simpleMeterStep() );
    else
	pm = new TextStreamProgressMeter( *strm );

    ((Task*)(this))->setProgressMeter( pm.ptr() );
    pm->setName( name() );

    bool res = SequentialTask::execute();
    if ( !res )
	*strm << "Error: " << uiMessage().getFullString() << od_newline;

    ((Task*)(this))->setProgressMeter( nullptr );

    if ( islast )
	*strm << "\n\nEnd of process: '" << name() << "'" << od_newline;

    strm->flush();
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
	    const Executor& exec = *executors_[idx];
	    const Executor& prevexec = *executors_[idx-1];
	    if ( exec.uiNrDoneText() != prevexec.uiNrDoneText() ||
		 exec.uiMessage() != prevexec.uiMessage() )
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


uiString ExecutorGroup::uiMessage() const
{
    if ( executors_.size() )
	return executors_[currentexec_]->uiMessage();

    return Executor::uiMessage();
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


uiString ExecutorGroup::uiNrDoneText() const
{
    if ( !nrdonetext_.isEmpty() )
    {
	return nrdonetext_;
    }

    if ( executors_.isEmpty() )
	return Executor::uiNrDoneText();

    return executors_[currentexec_]->uiNrDoneText();
}


bool TextTaskRunner::execute( Task& t )
{
    mDynamicCastGet(Executor*,exec,&t)
    if ( exec )
	execres_ = exec->go( strm_ );
    else
    {
	TextStreamProgressMeter progressmeter(strm_);
	t.setProgressMeter( &progressmeter );
	execres_ = t.execute();
    }

    return execres_;
}
