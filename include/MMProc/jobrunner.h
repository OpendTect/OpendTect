#ifndef jobman_h
#define jobman_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Oct 2004
 RCS:		$Id: jobrunner.h,v 1.10 2004-11-11 11:35:57 bert Exp $
________________________________________________________________________

-*/

#include "executor.h"
class IOPar;
class HostData;
class JobInfo;
class JobHostInfo;
class JobDescProv;
class JobIOMgr;
class StatusInfo;
class BufferStringSet;
class FilePath;

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
				//!< JobDescProv becomes mine. Never pass null.
				~JobRunner();

    const JobDescProv*		descProv() const	{ return descprov_; }

    const ObjectSet<JobHostInfo>& hostInfo() const	{ return hostinfo_; }
    bool			addHost(const HostData&);
    void			removeHost(int);
    void			pauseHost(int,bool);
    bool			stopAll();
    bool			isFailed(int) const;
    bool			isPaused(int) const;
    bool			isAssigned( const JobInfo& ji ) const;

    int				nrJobs( bool failed=false ) const
    				{ return (failed ? jobinfos_ : failedjobs_)
				    	 .size(); }
    const JobInfo&		jobInfo( int idx, bool failed=false ) const
    				{ return *(failed ? jobinfos_ : failedjobs_)
				    	 [idx]; }

    int				jobsDone() const;
    int				jobsInProgress() const;
    int				jobsLeft() const
				{ return jobinfos_.size() - jobsDone(); }
    int				totalJobs() const
				{ return jobinfos_.size()+failedjobs_.size(); }
    JobInfo*			currentJob(const JobHostInfo*) const;

    int				nextStep()	{ return doCycle(); }
    int				nrDone() const	{ return jobsDone(); }
    int				totalNr() const	{ return totalJobs(); }
    const char*			message() const;
    const char*			nrDoneMessage() const;

    				// Set these before first step
    void			setFirstPort( int n )	    { firstport_ = n; }
    void			setRshComm( const char* s ) { rshcomm_ = s; }
    void			setProg( const char* s )    { prog_ = s; }
    				// Set this anytime
    void			setNiceNess( int n );

    void			showMachStatus( BufferStringSet& ) const;
    const FilePath&		getBaseFilePath(JobInfo&, const HostData&);

    Notifier<JobRunner>		jobStarted;
    Notifier<JobRunner>		jobFailed;
    const JobInfo&		notifyJob() const	{ return *notifyji; }

protected:

    JobDescProv*		descprov_;
    ObjectSet<JobInfo>		jobinfos_;
    ObjectSet<JobHostInfo>	hostinfo_;
    ObjectSet<JobInfo>		failedjobs_;
    BufferString		prog_;
    BufferString		procdir_;

    JobIOMgr&			iomgr();
    JobIOMgr*			iomgr__;

    int				niceval_;
    int				firstport_;
    BufferString		rshcomm_;
    int				maxhostfailures_;
    int				maxjobfailures_;
    int				timeout_;
    JobInfo*			notifyji;

    int				doCycle();
    JobHostInfo*		jobHostInfoFor(const HostData&) const;

    void			updateJobInfo();
    void 			handleStatusInfo( StatusInfo& );
    JobInfo* 			gtJob( int descnr );

    enum StartRes		{ Started, NotStarted, JobBad, HostBad };
    StartRes			startJob( JobInfo& ji, JobHostInfo& jhi );
    bool			runJob(JobInfo&,const HostData&);
    bool			assignJob(JobHostInfo&);
    bool			haveIncomplete() const;
    bool			isFailed(const JobHostInfo*) const;

};

#endif
