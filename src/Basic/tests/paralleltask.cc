/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "paralleltask.h"
#include "thread.h"
#include "threadlock.h"
#include "typeset.h"


/*!\brief Records the (start,stop,threadidx) slices handed out by
  ParallelTask::executeParallel.

  Regression test for the ParallelTaskRunner dispatch wiring in
  src/Basic/task.cc (runners[nrtasks] must be paired with
  tasks[nrtasks]). Chunks must partition [0,niter) contiguously and each
  chunk's threadidx must equal its position when ordered by start, i.e.
  0..nrchunks-1 ascending. The mis-wired version hands out threadidx in
  reverse (nrchunks-1..0), which breaks any doWork()/doPrepare() that
  sizes per-thread resources by the number of chunks.
*/

class ChunkRecordTask : public ParallelTask
{
public:
			ChunkRecordTask(od_int64 niter)
			    : niter_(niter)
			{}

    od_int64		nrIterations() const override	{ return niter_; }

    bool		doWork(od_int64 start,od_int64 stop,
			       int threadidx) override
			{
			    Threads::Locker locker( lock_ );
			    starts_ += start;
			    stops_ += stop;
			    threadids_ += threadidx;
			    return true;
			}

    int			nrChunks() const	{ return starts_.size(); }

    bool		verify(BufferString& errmsg) const
			{
			    const int nrchunks = starts_.size();
			    if ( nrchunks != stops_.size() ||
				 nrchunks != threadids_.size() )
			    {
				errmsg = "Chunk record size mismatch";
				return false;
			    }

			    if ( nrchunks < 2 )
			    {
				errmsg.set( "Expected parallel dispatch, got " )
				      .add( nrchunks ).add( " chunk(s)" );
				return false;
			    }

			    TypeSet<int> order;
			    for ( int idx=0; idx<nrchunks; idx++ )
				order += idx;

			    for ( int idx=0; idx<nrchunks; idx++ )
			    {
				int best = idx;
				for ( int jdx=idx+1; jdx<nrchunks; jdx++ )
				{
				    if ( starts_[order[jdx]] <
					 starts_[order[best]] )
					best = jdx;
				}

				if ( best != idx )
				    order.swap( idx, best );
			    }

			    if ( starts_[order[0]] != 0 )
			    {
				errmsg = "First chunk does not start at 0";
				return false;
			    }

			    if ( stops_[order[nrchunks-1]] != niter_-1 )
			    {
				errmsg = "Last chunk does not end at niter-1";
				return false;
			    }

			    for ( int idx=1; idx<nrchunks; idx++ )
			    {
				const od_int64 prevstop =
				    stops_[order[idx-1]];
				const od_int64 curstart =
				    starts_[order[idx]];
				if ( curstart != prevstop+1 )
				{
				    errmsg.set( "Chunks overlap or leave a gap"
						" between " ).add( prevstop )
					  .add( " and " ).add( curstart );
				    return false;
				}
			    }

			    TypeSet<int> seenthreadids;
			    for ( int idx=0; idx<nrchunks; idx++ )
			    {
				const int tid = threadids_[order[idx]];
				if ( tid < 0 || tid >= nrchunks )
				{
				    errmsg.set( "threadidx out of range: " )
					  .add( tid );
				    return false;
				}

				if ( seenthreadids.isPresent( tid ) )
				{
				    errmsg.set( "Duplicate threadidx: " )
					  .add( tid );
				    return false;
				}

				seenthreadids += tid;
				if ( tid != idx )
				{
				    errmsg.set( "threadidx does not match"
						" chunk order: expected " )
					  .add( idx ).add( ", got " )
					  .add( tid );
				    return false;
				}
			    }

			    return true;
			}

protected:

    od_int64		niter_;
    Threads::Lock	lock_;
    TypeSet<od_int64>	starts_;
    TypeSet<od_int64>	stops_;
    TypeSet<int>	threadids_;

};


class ParallelFillTask : public ParallelTask
{
public:
			ParallelFillTask(float* ptr,od_int64 niter)
			    : ptr_(ptr)
			    , niter_(niter)
			{}

    od_int64		nrIterations() const override	{ return niter_; }

    bool		doWork(od_int64 start,od_int64 stop,
			       int /*threadidx*/) override
			{
			    for ( od_int64 idx=start; idx<=stop; idx++ )
				ptr_[idx] = (float)idx;

			    return true;
			}

protected:

    float*		ptr_;
    od_int64		niter_;

};


static bool testChunkMapping()
{
    const od_int64 niter = 10000;
    ChunkRecordTask task( niter );
    mRunStandardTest( task.execute(), "ChunkRecordTask executes" );

    BufferString errmsg;
    if ( !task.verify( errmsg ) )
    {
	errStream() << "Chunk mapping: " << errmsg << od_endl;
	return false;
    }

    logStream() << "[OK] Parallel dispatch chunk mapping" << od_endl;
    return true;
}


static bool testParallelFill()
{
    const od_int64 niter = 10000;
    float* ptr = new float[niter];
    for ( od_int64 idx=0; idx<niter; idx++ )
	ptr[idx] = -1.f;

    ParallelFillTask task( ptr, niter );
    const bool res = task.execute();
    mRunStandardTest( res, "ParallelFillTask executes" );

    for ( od_int64 idx=0; idx<niter; idx++ )
    {
	if ( ptr[idx] != (float)idx )
	{
	    errStream() << "Parallel fill mismatch at " << idx
			<< ": expected " << (float)idx
			<< ", got " << ptr[idx] << od_endl;
	    delete [] ptr;
	    return false;
	}
    }

    delete [] ptr;
    logStream() << "[OK] Parallel fill covers every iteration once"
		<< od_endl;
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( Threads::getNrProcessors() < 2 )
    {
	logStream() << "Single-processor machine, parallel dispatch"
		    << " degrades to one chunk, skipping" << od_endl;
	return 0;
    }

    if ( !testChunkMapping() || !testParallelFill() )
	return 1;

    return 0;
}
