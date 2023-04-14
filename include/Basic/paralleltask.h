#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "task.h"
#include "atomic.h"

class ParallelTaskRunner;


/*!\brief Generalization of a task that can be run in parallel.

  Any task that has a fixed number of computations that are independent
  (i.e. they don't need to be done in a certain order) can inherit
  ParallelTask and be executed in parallel by calling the
  ParallelTask::execute().

  Example of usage:

  \code
    float result[N];
    for ( int idx=0; idx<N; idx++ )
	result[idx] = input1[idx]* function( idx, other, variables );
  \endcode

  Could be made parallel by adding the class:

  \code

  class CalcClass : public ParallelTask
  {
  public:
	od_int64	nrIterations() const { return N; }
	int		doWork( od_int64 start, od_int64 stop, int threadid )
			{
			for ( int idx=start;idx<=stop &&shouldContinue();idx++ )
			{
				result[idx] = input1[idx] *
				function( idx, other, variables );
				addToNrDone( 1 );
			}

			return true;
			}
			};

  \endcode
  and in use that instead of the for-loop:
  \code
      CalcClass myclass( N, my, parameters );
      myclass.exectute();
  \endcode
*/

mExpClass(Basic) ParallelTask : public ReportingTask
{ mODTextTranslationClass(ParallelTask);
public:
    virtual		~ParallelTask();

    void		doParallel(bool yn)	{ parallel_ = yn; }

    bool		execute() override
			{ return executeParallel(parallel_); }
			/*!<Runs the process the desired number of times. \note
			    that the function has static threads (normally the
			    same number as there are processors on the machine),
			    and these static threads will be shared by all
			    instances of ParallelTask::execute. */
    virtual bool	executeParallel(bool parallel);
			/*!<Runs the process the desired number of times. \note
			    that the function has static threads (normally the
			    same number as there are processors on the machine),
			    and these static threads will be shared by all
			    instances of ParallelTask::execute. */

    od_int64		nrDone() const override;
			//!<May be -1, i.e. class does not report nrdone.

    od_int64		totalNr() const override { return nrIterations(); }
    static uiString	sPosFinished()	{ return tr("Positions finished"); }
    static uiString	sTrcFinished()	{ return tr("Traces finished"); }

protected:

    virtual od_int64	nrIterations() const				= 0;
			/*!<\returns the number of times the process should be
			    run. */
    virtual int		maxNrThreads() const;
    virtual int		minThreadSize() const	{ return 1; }
			/*!<\returns the minimum number of computations that
			     effectively can be run in a separate thread.
			     A small number will give a large overhead for when
			     each step is quick and nrIterations is not big. */
    virtual bool	stopAllOnFailure() const	{ return true; }
			/*!<If one thread fails, should an attempt be made to
			    stop the others? If true, enableWorkControl will
			    be enabled, and threads should call shouldContinue()
			    regularly. */

			ParallelTask(const char* nm=0);
    od_int64		calculateThreadSize(od_int64 totalnr,int nrthreads,
					    int thread) const;

    void		addToNrDone(od_int64 increment);
			/*!<Call this from within your thread to say
			    that you have done something.
			    Do NOT call in very fast loops at every idx since
			    it can impact negatively performance */
    void		quickAddToNrDone(od_int64 loopidx);
			/*!<Call this from within your thread to say
			    that you have done something, but not very often */

    void		resetNrDone();

private:
    virtual bool	doWork(od_int64 start,od_int64 stop,int threadidx) = 0;
			/*!<The functions that does the job. The function
			    will be called with all intervals from 0 to
			    ParallelTask::nrIterations()-1. The function must
			    be designed to be able to run in parallel.
			    \param start first index
			    \param stop last index
			    \param threadidx gives an identifier (between 0 and
				   nr of threads -1) that is unique to each call
				   to doWork. */
    virtual bool	doPrepare(int nrthreads)	{ return true; }
			/*!<Called once, before any doWork is called. */
    virtual bool	doFinish(bool success)		{ return success; }
			/*!<Called after all doWork have finished.
			    \param success indicates whether all doWork returned
				   true. */

    friend class			ParallelTaskRunner;
    Threads::Atomic<od_int64>		nrdone_;

private:

    od_int64				totalnrcache_;
    od_int64				nrdonebigchunksz_;
    bool				parallel_ = true;

};


/*!Macros to define a class to exectute your loop in parallel.

  The loop index is 'idx'.

Example:

    The original loop was:

for ( int isamp=0; isamp<outnrsamples; isamp++ )
    trc.set( isamp, storinterp_->get(blockbuf_,isamp), curcomp );

    There are 4 parameters (trc, curcomp, blockbuf_ and storinterp_) to
    pass to the executing object. You also need to pass a uiString giving
    the uiMessage if the execution is done with a taskrunner, thus:


mDefParallelCalc4Pars( SEGYSampleInterpreter,
	   od_static_tr("SEGYSampleInterpreter","Copying trace information"),
	   SeisTrc&,trc, int,curcomp, unsigned char*,blockbuf,
	   const TraceDataInterpreter*,storinterp)
mDefParallelCalcBody( \* No initializations *\,
	    trc_.set( idx, storinterp_->get(blockbuf_,idx), curcomp_ );
		    , \* No post-operations *\)

SEGYSampleInterpreter interp( trc.size(), trc, curcomp, blockbuf_, storinterp_);
interp.execute();

 */
#define mDeclareParallelCalcStd(uimsg) \
    od_int64	sz_; \
    bool	reportprogress_; \
    void	setReport(bool yn)		{ reportprogress_ = yn; } \
    od_int64	nrIterations() const override	{ return sz_; } \
    uiString	uiMessage() const override	{ return uimsg; } \
    uiString	uiNrDoneText() const override	{ return sPosFinished(); } \

#define mDefParallelCalcNoPars(clss,uimsg) \
	class clss : public ParallelTask \
	{ mODTextTranslationClass(clss) \
	public: \
	    mDeclareParallelCalcStd(uimsg) \
	    clss( od_int64 _sz_ ) : sz_(_sz_), reportprogress_(true)	{} \

#define mDefParallelCalc1Par(clss,uimsg,T1,v1) \
	class clss : public ParallelTask \
	{ mODTextTranslationClass(clss) \
	public: \
	    mDeclareParallelCalcStd(uimsg) \
	    T1		v1##_; \
	    clss( od_int64 _sz_, T1 _##v1##_ ) \
		: sz_(_sz_), reportprogress_(true), v1##_(_##v1##_)	{} \

#define mDefParallelCalc2Pars(clss,uimsg,T1,v1,T2,v2) \
	class clss : public ParallelTask \
	{ mODTextTranslationClass(clss) \
	public: \
	    mDeclareParallelCalcStd(uimsg) \
	    T1 v1##_; T2 v2##_; \
	    clss( od_int64 _sz_, T1 _##v1##_, T2 _##v2##_ ) \
		: sz_(_sz_), reportprogress_(true) \
		, v1##_(_##v1##_), v2##_(_##v2##_)			{} \

#define mDefParallelCalc3Pars(clss,uimsg,T1,v1,T2,v2,T3,v3) \
	class clss : public ParallelTask \
	{ mODTextTranslationClass(clss) \
	public: \
	    mDeclareParallelCalcStd(uimsg) \
	    T1 v1##_; T2 v2##_; T3 v3##_; \
	    clss( od_int64 _sz_, \
		    T1 _##v1##_, T2 _##v2##_, T3 _##v3##_ ) \
		: sz_(_sz_), reportprogress_(true) \
		, v1##_(_##v1##_), v2##_(_##v2##_) , v3##_(_##v3##_)	{} \

#define mDefParallelCalc4Pars(clss,uimsg,T1,v1,T2,v2,T3,v3,T4,v4) \
	class clss : public ParallelTask \
	{ mODTextTranslationClass(clss) \
	public: \
	    mDeclareParallelCalcStd(uimsg) \
	    T1 v1##_; T2 v2##_; T3 v3##_; T4 v4##_; \
	    clss( od_int64 _sz_, \
		    T1 _##v1##_, T2 _##v2##_, T3 _##v3##_, T4 _##v4##_ ) \
		: sz_(_sz_), reportprogress_(true) \
		, v1##_(_##v1##_), v2##_(_##v2##_) \
		, v3##_(_##v3##_), v4##_(_##v4##_)			{} \


#define mDefParallelCalc5Pars(clss,uimsg,T1,v1,T2,v2,T3,v3,T4,v4,T5,v5) \
	class clss : public ParallelTask \
	{ mODTextTranslationClass(clss) \
	public: \
	    mDeclareParallelCalcStd(uimsg) \
	    T1 v1##_; T2 v2##_; T3 v3##_; T4 v4##_; T5 v5##_;\
	    clss( od_int64 _sz_, T1 _##v1##_, T2 _##v2##_, T3 _##v3##_, \
				 T4 _##v4##_, T5 _##v5##_ ) \
		: sz_(_sz_), reportprogress_(true) \
		, v1##_(_##v1##_), v2##_(_##v2##_) \
		, v3##_(_##v3##_), v4##_(_##v4##_), v5##_(_##v5##_)	{} \

#define mDefParallelCalc6Pars(clss,uimsg,T1,v1,T2,v2,T3,v3,T4,v4,T5,v5,T6,v6) \
	class clss : public ParallelTask \
	{ mODTextTranslationClass(clss) \
	public: \
	    mDeclareParallelCalcStd(uimsg) \
	    T1 v1##_; T2 v2##_; T3 v3##_; T4 v4##_; T5 v5##_; T6 v6##_;\
	    clss( od_int64 _sz_, T1 _##v1##_, T2 _##v2##_, T3 _##v3##_, \
				 T4 _##v4##_, T5 _##v5##_, T6 _##v6##_ ) \
		: sz_(_sz_), reportprogress_(true) \
		, v1##_(_##v1##_), v2##_(_##v2##_) \
		, v3##_(_##v3##_), v4##_(_##v4##_) \
		, v5##_(_##v5##_), v6##_(_##v6##_)			{} \

#define mDefParallelCalc7Pars(clss,uimsg,T1,v1,T2,v2,T3,v3,T4,v4,T5,v5, \
			      T6,v6,T7,v7) \
	class clss : public ParallelTask \
	{ mODTextTranslationClass(clss) \
	public: \
	    mDeclareParallelCalcStd(uimsg) \
	    T1 v1##_; T2 v2##_; T3 v3##_; T4 v4##_; \
	    T5 v5##_; T6 v6##_; T7 v7##_; \
	    clss( od_int64 _sz_, T1 _##v1##_, T2 _##v2##_, T3 _##v3##_, \
		    T4 _##v4##_, T5 _##v5##_, T6 _##v6##_, T7 _##v7##_ ) \
		: sz_(_sz_), reportprogress_(true) \
		, v1##_(_##v1##_), v2##_(_##v2##_) \
		, v3##_(_##v3##_), v4##_(_##v4##_) \
		, v5##_(_##v5##_), v6##_(_##v6##_), v7##_(_##v7##_)    {} \

#define mDefParallelCalcBody(preop,impl,postop) \
	    bool doWork( od_int64 start, od_int64 stop, int ) override \
	    { \
		preop; \
		for ( od_int64 idx=start; idx<=stop; idx++ ) \
		    { impl; if ( reportprogress_ ) quickAddToNrDone(idx); } \
		postop; \
		return true; \
	    } \
	};
