#ifndef jobrunner_h
#define jobrunner_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Oct 2004
 RCS:		$Id: jobrunner.h,v 1.23 2009/07/22 16:01:15 cvsbert Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "jobinfo.h"

class IOPar;
class HostData;
class JobDescProv;
class JobIOMgr;
class StatusInfo;
class BufferStringSet;
class FilePath;


mClass HostNFailInfo
{
public:
    			HostNFailInfo( const HostData& hd )
			    : hostdata_(hd)
			    , nrfailures_(0)
			    , nrsucces_(0)
			    , lastsuccess_(0)
			    , starttime_(0)
			    , inuse_(false) {}

    const HostData&	hostdata_;
    int			nrfailures_; //!< Reset to 0 at every success
    int			nrsucces_;
    int			starttime_;  //!< Set whenever host added.
    int			lastsuccess_; //!< timestamp
    bool		inuse_;
};


/*!\brief Runs all jobs defined by JobDescProv. */

mClass JobRunner : public Executor
{
public:

    				JobRunner(JobDescProv*,const char* cmd);
				//!< JobDescProv becomes mine. Never pass null.
				~JobRunner();

    const JobDescProv*		descProv() const	{ return descprov_; }

    const ObjectSet<HostNFailInfo>& hostInfo() const	{ return hostinfo_; }
    bool			addHost(const HostData&);
    void			removeHost(int);
    void			pauseHost(int,bool);
    bool			stopAll();
    bool			hostFailed(int) const;
    bool			isPaused(int) const;
    bool			isAssigned( const JobInfo& ji ) const;

    int				nrJobs( bool failed=false ) const
    				{ return (failed ? failedjobs_ : jobinfos_)
				    	 .size(); }
    const JobInfo&		jobInfo( int idx, bool failed=false ) const
    				{ return *(failed ? failedjobs_ : jobinfos_)
				    	 [idx]; }

    int				jobsDone() const;
    int				jobsInProgress() const;
    int				jobsLeft() const
				{ return jobinfos_.size() - jobsDone(); }
    int				totalJobs() const
				{ return jobinfos_.size()+failedjobs_.size(); }
    JobInfo*			currentJob(const HostNFailInfo*) const;

    int				nextStep()	{ return doCycle(); }
    od_int64			nrDone() const	{ return jobsDone(); }
    od_int64			totalNr() const	{ return totalJobs(); }
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

    Notifier<JobRunner>		preJobStart;
    Notifier<JobRunner>		postJobStart;
    Notifier<JobRunner>		jobFailed;
    Notifier<JobRunner>		msgAvail;

    const JobInfo&		curJobInfo() const	{ return *curjobinfo_; }
    IOPar&			curJobIOPar()		{ return curjobiop_; }
    const FilePath&		curJobFilePath()	{ return curjobfp_; }

    const char*			procDir() const	{ return procdir_.buf(); }
    				// processing directory on local machine

protected:

    JobDescProv*		descprov_;
    ObjectSet<JobInfo>		jobinfos_;
    ObjectSet<HostNFailInfo>	hostinfo_;
    ObjectSet<JobInfo>		failedjobs_;
    BufferString		prog_;
    BufferString		procdir_;
    FilePath&			curjobfp_;
    IOPar&			curjobiop_;
    JobInfo*			curjobinfo_;

    JobIOMgr&			iomgr();
    JobIOMgr*			iomgr__;

    int				niceval_;
    int				firstport_;
    BufferString		rshcomm_;
    int				maxhostfailures_; //!< host failrs B4 host bad
    int				maxjobfailures_;  //!< job related job failrs
    int				maxjobhstfails_;  //!< host related job failrs
    int				starttimeout_;
    int				failtimeout_;
    int				wrapuptimeout_;
    int				hosttimeout_; 
    int				startwaittime_;  //!< wait B4 next client start

    int				doCycle();
    HostNFailInfo*		hostNFailInfoFor(const HostData*) const;

    void			updateJobInfo();
    void 			handleStatusInfo( StatusInfo& );
    JobInfo* 			gtJob( int descnr );

    void 			failedJob( JobInfo&, JobInfo::State );

    enum StartRes		{ Started, NotStarted, JobBad, HostBad };
    StartRes			startJob( JobInfo& ji, HostNFailInfo& jhi );
    bool			runJob(JobInfo&,const HostData&);


    enum AssignStat		{ NotReady, BadHost, JobStarted, NoJobs };
    AssignStat			assignJob(HostNFailInfo&);
    bool			haveIncomplete() const;

    enum HostStat		{ OK = 0, SomeFailed = 1, HostFailed = 2 };
    HostStat			hostStatus(const HostNFailInfo*) const;

};

#endif
