#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2004
________________________________________________________________________

-*/

#include "mmprocmod.h"
#include "executor.h"
#include "jobinfo.h"

class BufferStringSet;
class HostData;
class JobDescProv;
class JobIOMgr;
class StatusInfo;
namespace File { class Path; }

/*!
\brief Holds host-specific status information.
*/

mExpClass(MMProc) HostNFailInfo
{ mODTextTranslationClass(HostNFailInfo);
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


/*!
\brief Runs all jobs defined by JobDescProv.
*/

mExpClass(MMProc) JobRunner : public Executor
{ mODTextTranslationClass(JobRunner);
public:

				JobRunner(JobDescProv*,const char* cmd,
					  od_ostream* logstrm=0);
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
    uiString			message() const;
    uiString			nrDoneText() const;

				// Set these before first step
    void			setFirstPort( int n )	    { firstport_ = n; }
    void			setRshComm( const char* s ) { rshcomm_ = s; }
    void			setProg( const char* s )    { prog_ = s; }
				// Set this anytime
    void			setPriority(float);

    void			showMachStatus( BufferStringSet& ) const;
    const File::Path&		getBaseFilePath(JobInfo&, const HostData&);

    Notifier<JobRunner>		preJobStart;
    Notifier<JobRunner>		postJobStart;
    Notifier<JobRunner>		jobFailed;
    Notifier<JobRunner>		msgAvail;

    const JobInfo&		curJobInfo() const	{ return *curjobinfo_; }
    IOPar&			curJobIOPar()		{ return curjobiop_; }
    const File::Path&		curJobFilePath()	{ return curjobfp_; }
    int				getLastReceivedTime(JobInfo&);

    const char*			procDir() const	{ return procdir_.buf(); }
				// processing directory on local machine
    uiString			errorMsg() const;

protected:

    JobDescProv*		descprov_;
    ObjectSet<JobInfo>		jobinfos_;
    ObjectSet<HostNFailInfo>	hostinfo_;
    ObjectSet<JobInfo>		failedjobs_;
    BufferString		prog_;
    BufferString		procdir_;
    File::Path&			curjobfp_;
    IOPar&			curjobiop_;
    JobInfo*			curjobinfo_;
    od_ostream*			logstrm_;

    JobIOMgr&			iomgr();
    JobIOMgr*			iomgr_;

    float			prioritylevel_;
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
    uiString			errmsg_;

    int				doCycle();
    HostNFailInfo*		hostNFailInfoFor(const HostData*) const;

    void			updateJobInfo();
    void			handleStatusInfo( StatusInfo& );
    JobInfo*			gtJob( int descnr );

    void			failedJob( JobInfo&, JobInfo::State );

    enum StartRes		{ Started, NotStarted, JobBad, HostBad };
    StartRes			startJob( JobInfo& ji, HostNFailInfo& jhi );
    bool			runJob(JobInfo&,const HostData&);


    enum AssignStat		{ NotReady, BadHost, JobStarted, NoJobs };
    AssignStat			assignJob(HostNFailInfo&);
    bool			haveIncomplete() const;

    enum HostStat		{ OK = 0, SomeFailed = 1, HostFailed = 2 };
    HostStat			hostStatus(const HostNFailInfo*) const;
    void			handleExitStatus(JobInfo&);
};

mGlobal(MMProc) int& MMJob_getTempFileNr();
