#ifndef jobman_h
#define jobman_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Oct 2004
 RCS:		$Id: jobrunner.h,v 1.2 2004-10-25 11:59:24 bert Exp $
________________________________________________________________________

-*/

#include "executor.h"
class IOPar;
class HostData;
class JobInfo;
class JobHostInfo;
class JobDescProv;

class JobHostInfo
{
public:
    			JobHostInfo( const HostData& hd )
			    : hostdata_(hd)
			    , nrfailures_(0)	{}

    const HostData&	hostdata_;
    int			nrfailures_; //!< Reset to 0 at every success
};


/*!\brief Runs all jobs defined by JobDescProv. */

class JobRunner : public Executor
{
public:

    				JobRunner(JobDescProv*,const char* cmd);
				//!< prov becomes mine

    const ObjectSet<JobHostInfo>& hostInfo() const { return hostinfo_; }
    bool			addHost(const HostData&);

    void			removeHost(int);
    void			pauseHost(int,bool);
    bool			isFailed(int) const;
    bool			isPaused(int) const;
    bool			isAssigned(int) const;

    int				jobsDone() const;
    int				jobsInProgress() const;
    int				totalJobs() const
				{ return jobinfos_.size()+failedjobs_.size(); }
    int				jobsFailed() const
    				{ return failedjobs_.size(); }
    JobInfo*			currentJob(const JobHostInfo*) const;

    int				nextStep()	{ return doCycle(); }
    int				nrDone() const	{ return jobsDone(); }
    int				totalNr() const	{ return totalJobs(); }
    const char*			message() const;
    const char*			nrDoneMessage() const;

    				// Set these before first step
    void			setFirstPort( int n )	{ firstport_ = n; }
    void			setRshComm( const char* s ) { rshcomm_ = s; }
    void			setProg( const char* s ) { prog_ = s; }
    				// Set this anytime
    void			setNiceNess( int n )	{ niceval_ = n; }

protected:

    JobDescProv*		descprov_;
    ObjectSet<JobInfo>		jobinfos_;
    ObjectSet<JobHostInfo>	hostinfo_;
    ObjectSet<JobInfo>		failedjobs_;
    BufferString		prog_;
    BufferString		procdir_;

    int				niceval_;
    int				firstport_;
    BufferString		rshcomm_;
    int				maxhostfailures_;
    int				maxjobfailures_;

    int				doCycle();
    JobHostInfo*		jobHostInfoFor(const HostData&) const;

    enum StartRes		{ Started, NotStarted, JobBad, HostBad };
    StartRes			startJob(JobInfo&,JobHostInfo&);
    bool			runJob(JobInfo&,const HostData&);
    bool			assignJob(JobHostInfo&);
    bool			startProg(const HostData&,IOPar&,int,
	    				  BufferString&);
    bool			haveIncomplete() const;
    bool			isFailed(const JobHostInfo*) const;

};

#endif
