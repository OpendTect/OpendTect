#ifndef jobman_h
#define jobman_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Oct 2004
 RCS:		$Id: jobrunner.h,v 1.1 2004-10-25 07:26:20 bert Exp $
________________________________________________________________________

-*/

#include "executor.h"
class HostData;
class JobInfo;
class JobHostInfo;
class JobDescProv;

class JobHostInfo
{
    			JobHostInfo( const HostData& hd )
			    : hostdata_(hd)
			    , failures_(0)	{}

    const HostData&	hostdata_;
    int			failures_; //!< Reset to 0 at every success
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
    bool			isPaused(int) const;

    int				jobsDone() const;
    int				jobsInProgress() const;
    int				totalJobs() const
				{ return jobinfos_.size()+failedjobs_.size(); }
    int				jobsFailed() const;
    				{ return failedjobs_.size(); }
    JobInfo*			currentJob(JobHostInfo*) const;

    int				nextStep()	{ return doCycle(); }
    int				nrDone() const	{ return jobsDone(); }
    int				totalNr() const	{ return totalJobs(); }
    const char*			message() const;
    const char*			nrDoneMessage() const;

    void			setNiceNess( int n )	{ niceval_ = n; }
    void			setFirstPort( int n )	{ firstport_ = n; }
    void			setRshComm( const char* s ) { rshcomm = s; }
    void			setProg( const char* s ) { prog_ = s; }

protected:

    JobDescProv*		descprov_;
    ObjectSet<JobInfo>		jobinfos_;
    ObjectSet<JobHostInfo>	hostinfo_;
    BufferString		procdir_;
    BufferString		prog_;

    int				niceval_;
    int				firstport_;
    BufferString		rshcomm_;

    int				nrhostattempts_;
    int				nrjobattempts_;
    ObjectSet<JobInfo>		failedjobs_;
    ObjectSet<JobHostInfo>	failedhosts_;

    int				doCycle();
    JobHostInfo*		jobHostInfoFor(const HostData&) const;

    enum StartRes		{ Started, NotStarted, JobSucks, HostSucks };
    StartRes			startJob(JobInfo&,JobHostInfo&);
    bool			runJob(JobInfo&,const HostData&);
    bool			startProg(const HostData&,IOPar&,BufferString&);
    bool			haveIncomplete() const;

};

#endif
