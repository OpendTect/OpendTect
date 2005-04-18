/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Oct 2004
 RCS:           $Id: jobrunner.cc,v 1.22 2005-04-18 14:09:44 cvsarend Exp $
________________________________________________________________________

-*/

#include "jobrunner.h"
#include "jobinfo.h"
#include "jobiomgr.h"
#include "jobdescprov.h"
#include "hostdata.h"
#include "filepath.h"
#include "filegen.h"
#include "iopar.h"
#include "jobiomgr.h"
#include "queue.h"
#include "mmdefs.h"
#include "timefun.h"
#include "debugmasks.h"
#include "msgh.h"
#include <iostream>

#define mDebugOn        (DBG::isOn(DBG_MM))

#define mAddDebugMsg( ji ) \
    msg += "\n Job failed "; msg += ji.jobfailures_; \
    msg += " times due to job related problems."; \
    msg += "\n and "; msg += ji.hstfailures_; \
    msg += " times due to host related problems."; \
    if ( ji.hostdata_ ) \
	{ msg += "\n Execut[ed/ing] on host: "; msg += ji.hostdata_->name(); } \
    msg += "\n Status: "; msg += ji.statusmsg_; \
    msg += "\n Info: "; msg += ji.infomsg_;


static BufferString tmpfnm_base;

static int mkTmpFileNr()
{
    tmpfnm_base = HostData::localHostName();
    tmpfnm_base += "_";
    tmpfnm_base += getPID();
    return 1;
}
                                                                                
static int tmpfile_nr = mkTmpFileNr();


JobRunner::JobRunner( JobDescProv* p, const char* cmd )
	: Executor("Running jobs")
	, iomgr__(0)
	, descprov_(p)
    	, rshcomm_("rsh")
	, niceval_(19)
	, firstport_(19636)
    	, prog_(cmd)
	, starttimeout_(	atoi( getenv("DTECT_START_TIMEOUT")
				    ? getenv("DTECT_START_TIMEOUT"): "45000") )
	, failtimeout_(		atoi( getenv("DTECT_FAIL_TIMEOUT")
				    ? getenv("DTECT_FAIL_TIMEOUT"): "300000") )
	, hosttimeout_(		atoi( getenv("DTECT_HOST_TIMEOUT")
				    ? getenv("DTECT_HOST_TIMEOUT"): "600000") )
    	, maxhostfailures_(	atoi( getenv("DTECT_MAX_HOSTFAIL")
				    ? getenv("DTECT_MAX_HOSTFAIL") : "2") )
    	, maxjobfailures_(	atoi( getenv("DTECT_MAX_JOBFAIL")
				    ? getenv("DTECT_MAX_JOBFAIL") : "2") )
    	, maxjobhstfails_(	atoi( getenv("DTECT_MAX_JOBHSTF")
				    ? getenv("DTECT_MAX_JOBHSTF") : "7") )
	, preJobStart(this)
	, postJobStart(this)
	, jobFailed(this)
	, msgAvail(this)
    	, curjobiop_(*new IOPar)
    	, curjobfp_(*new FilePath)
    	, curjobinfo_(0)
{
    FilePath fp( GetDataDir() );
    fp.add( "Proc" ).add( tmpfnm_base );
    procdir_ = fp.fullPath();
    procdir_ += "_"; procdir_ += tmpfile_nr;
    tmpfile_nr++;

    if ( File_exists(procdir_) && !File_isDirectory(procdir_) )
	File_remove(procdir_,NO);
    if ( !File_exists(procdir_) )
	File_createDir(procdir_,0);

    if ( Values::isUdf(descprov_->nrJobs()) || !descprov_->nrJobs() )
    {
	UsrMsg( "No jobs to process!", MsgClass::Error );
	return;
    }
	
    for ( int idx=0; idx<descprov_->nrJobs(); idx++ )
	jobinfos_ += new JobInfo( idx );
}


JobRunner::~JobRunner()
{
    deepErase( jobinfos_ );
    deepErase( hostinfo_ );
    deepErase( failedjobs_ );
    delete iomgr__;
}


HostNFailInfo* JobRunner::hostNFailInfoFor( const HostData* hd ) const
{
    if ( !hd ) return 0;

    HostNFailInfo* hfi = 0;
    for ( int idx=0; idx<hostinfo_.size(); idx++ )
	if ( &hostinfo_[idx]->hostdata_ == hd )
	    { hfi = hostinfo_[idx]; break; }

    return hfi;
}


bool JobRunner::addHost( const HostData& hd )
{

    HostNFailInfo* hfi = hostNFailInfoFor( &hd );
    const bool isnew = !hfi;
    if ( isnew )
    {
	hfi = new HostNFailInfo( hd );
	hfi->starttime_ = Time_getMilliSeconds();
    }
    else if ( hostStatus(hfi) == HostFailed )
    {
	hfi->nrfailures_ = 0;
	hfi->starttime_ = Time_getMilliSeconds();

	// Clear failed jobs' hostdata for this host.
	for ( int ijob=0; ijob<jobinfos_.size(); ijob++ )
	{
	    JobInfo& ji = *jobinfos_[ijob];
	    bool isfailed = ( ji.state_ == JobInfo::JobFailed
		           || ji.state_ == JobInfo::HostFailed);

	    if ( isfailed && ji.hostdata_ == &hfi->hostdata_ )
		ji.hostdata_=0;
	}
    }
    else
	return true; // host already in use

    AssignStat res = assignJob( *hfi );

    if ( isnew )
    {
	if ( res == BadHost )
	    delete hfi;
	else
	    hostinfo_ += hfi;
    }

    return !(res == BadHost);
}


JobRunner::AssignStat JobRunner::assignJob( HostNFailInfo& hfi )
{
    static int waittime( atoi( getenv("DTECT_WAIT_TIME")
			     ? getenv("DTECT_WAIT_TIME") : "1000") )
    static int timestamp = -1;

    int elapsed = timestamp > 0 ? Time_getMilliSeconds() - timestamp : -1;
    if ( elapsed < 0 ) timestamp = Time_getMilliSeconds();

    if ( elapsed < waittime ) return NotReady; 

    timestamp = Time_getMilliSeconds();
    
    // First go for new jobs, then try failed
    for ( int tryfailed=0; tryfailed<2; tryfailed++ )
    {
	for ( int ijob=0; ijob<jobinfos_.size(); ijob++ )
	{
	    JobInfo& ji = *jobinfos_[ijob];
	    bool isnew = ji.state_ == JobInfo::ToDo;
	    bool isfailed = ( ji.state_ == JobInfo::JobFailed
		           || ji.state_ == JobInfo::HostFailed);
	    if ( isfailed )
	    {
		if ( !tryfailed ) continue;
		if ( ji.hostdata_ && ji.hostdata_ == &hfi.hostdata_ ) continue;
	    }
	    else if ( !isnew )
		continue;

	    StartRes res = startJob( ji, hfi );
	    if ( res == HostBad )
		return BadHost;
	    else if ( res == Started )
		return JobStarted;
	}
    }

    return NoJobs;
}


JobRunner::StartRes JobRunner::startJob( JobInfo& ji, HostNFailInfo& hfi )
{
    if ( hostStatus(&hfi) == HostFailed ) return HostBad;

    if ( ji.jobfailures_ > maxjobfailures_ || ji.hstfailures_ > maxjobhstfails_)
    {
	ji.infomsg_ = " Too many failures -- Job Bad"; 
	curjobinfo_ = &ji;
	msgAvail.trigger();
    
	if ( mDebugOn )
	{
	    BufferString msg("----\nJobRunner::startJob : job ");
	    msg += ji.descnr_;	msg += " is bad.";
	    mAddDebugMsg( ji )
	    DBG::message(msg);
	}
   
	jobinfos_ -= &ji; failedjobs_ += &ji;
	return JobBad;
    }

    if ( !runJob(ji,hfi.hostdata_) )
    {
	if ( mDebugOn )
	{
	    BufferString msg("----\nJobRunner::startJob: could not start job ");
	    msg += ji.descnr_; msg += " on host "; msg += ji.hostdata_->name();
	    mAddDebugMsg( ji )
	    DBG::message(msg);
	}

	return hostStatus(&hfi) == HostFailed ? HostBad : NotStarted;
    }

    if ( mDebugOn )
    {
	BufferString msg("----\nJobRunner::startJob : started job ");
	msg += ji.descnr_; msg += " on host "; msg += ji.hostdata_->name();
	mAddDebugMsg( ji )
	DBG::message(msg);
    }

    return Started;
}


JobIOMgr& JobRunner::iomgr()
{
    if ( !iomgr__ ) iomgr__ = new JobIOMgr(firstport_, niceval_);
    return *iomgr__;
}


const FilePath& JobRunner::getBaseFilePath( JobInfo& ji, const HostData& hd  )
{
    static FilePath basefp;

    BufferString basenm( hd.name() );
    basenm += "_"; basenm += ji.descnr_;

    basefp = procdir_;
    basefp.add( basenm );

    return basefp;
}
   

void JobRunner::failedJob( JobInfo& ji, JobInfo::State reason )
{ 
    if ( !isAssigned(ji) ) return;

    curjobinfo_ = &ji;
    ji.state_ = reason;

    HostNFailInfo* hfi = 0;
    if ( ji.hostdata_ ) 
    {
	hfi = hostNFailInfoFor( ji.hostdata_ );

	if ( reason == JobInfo::JobFailed ) ji.jobfailures_++;
	else
	{
	    ji.hstfailures_++;
	    if ( hfi ) hfi->nrfailures_++;
	}
    }

    if ( mDebugOn )
    {
	BufferString msg("----\nJobRunner::failedJob : ");
	if ( ji.state_ == JobInfo::HostFailed ) msg += "host failed. ";
	if ( ji.state_ == JobInfo::JobFailed ) msg += "job failed. ";
	if ( hfi )
	{
	    msg += "\n Host failed "; msg += hfi->nrfailures_; msg += " times ";
	    msg += "\n Host started ";
	    msg += (Time_getMilliSeconds() - hfi->starttime_)/1000;
	    msg += " seconds ago.";
	}
	mAddDebugMsg( ji )
	DBG::message(msg);
    }
    
    jobFailed.trigger();
    ji.infomsg_ = "";
} 


bool JobRunner::runJob( JobInfo& ji, const HostData& hd )
{
    curjobiop_.clear(); descprov_->getJob( ji.descnr_, curjobiop_ );
    curjobfp_ = getBaseFilePath(ji,hd);
    curjobinfo_ = &ji;
    ji.hostdata_ = &hd;
    ji.state_ = JobInfo::Scheduled;
    ji.starttime_ = Time_getMilliSeconds();
    ji.recvtime_ = 0;
    ji.statusmsg_ = " scheduled";
    ji.infomsg_ = "";
    ji.osprocid_ = -1;
    preJobStart.trigger();

    if ( !iomgr().startProg( prog_, curjobiop_, curjobfp_, ji, rshcomm_ ) )
    {
	if ( iomgr().peekMsg() ) iomgr().fetchMsg(ji.infomsg_);
	failedJob( ji, JobInfo::HostFailed );
	return false;
    }

    postJobStart.trigger();
    return true;
}


JobInfo* JobRunner::currentJob( const HostNFailInfo* hfi ) const
{
    for ( int idx=0; idx<jobinfos_.size(); idx++ )
    {
	const JobInfo* ji = jobinfos_[idx];
	if ( ji->hostdata_ == &hfi->hostdata_ && isAssigned(*ji) )
	    return const_cast<JobInfo*>( ji );
    }
    return 0;
}


void JobRunner::removeHost( int hnr )
{
    if ( hnr < 0 || hnr >= hostinfo_.size() )
	return;
    HostNFailInfo* hfi = hostinfo_[hnr];
    hostinfo_ -= hfi;

    JobInfo* ji = currentJob( hfi );

    if ( ji )
    {
	ji->hostdata_ = &hfi->hostdata_;
	failedJob( *ji, JobInfo::HostFailed );

	iomgr().reqModeForJob( *ji, JobIOMgr::Stop );

	if ( ji->hostdata_ )
	    iomgr().removeJob( ji->hostdata_->name(), ji->descnr_ );
    }

    delete hfi;
}


void JobRunner::pauseHost( int hnr, bool yn )
{
    if ( hnr < 0 || hnr >= hostinfo_.size() )
	return;
    JobInfo* ji = currentJob( hostinfo_[hnr] );
    if ( !ji ) return;

    if ( isAssigned(*ji) && ji->state_ != JobInfo::Paused && yn )
	iomgr().reqModeForJob( *ji, JobIOMgr::Pause );
    else if ( ji->state_ == JobInfo::Paused && !yn )
	iomgr().reqModeForJob( *ji, JobIOMgr::Work );
}


bool JobRunner::isPaused( int hnr ) const
{
    if ( hnr < 0 || hnr >= hostinfo_.size() )
	return false;
    const JobInfo* ji = currentJob( hostinfo_[hnr] );
    return ji ? ji->state_ == JobInfo::Paused : false;
}


bool JobRunner::isAssigned( const JobInfo& ji ) const
{
    return ji.state_ >= JobInfo::Scheduled && ji.state_ <= JobInfo::Paused;
}


bool JobRunner::hostFailed( int hnr ) const
{
    return hnr < 0 || hnr >= hostinfo_.size() ? true
	 : hostStatus( hostinfo_[hnr] ) == HostFailed;
}


JobRunner::HostStat JobRunner::hostStatus( const HostNFailInfo* hfi ) const
{
    if ( !hfi ) return HostFailed;

    if ( hfi->starttime_ <= 0 )
    {
	pErrMsg( "starttime_ <= 0!!. Setting to current time..." );
	if ( mDebugOn )
	{
	    BufferString msg( "Start time (" );
	    msg += hfi->starttime_; msg += ") <= 0 for ";

	    msg += hfi->hostdata_.name();
	    msg += "\n nrfail: "; msg += hfi->nrfailures_;
	    msg += "\n nrsucc: "; msg += hfi->nrsucces_;
	    msg += "\n last succes time: "; msg += hfi->lastsuccess_;

	    DBG::message(msg);
	}
	const_cast<HostNFailInfo*>(hfi)->starttime_ = Time_getMilliSeconds();
    }

    if ( !hfi->nrfailures_ ) return OK;

    if ( hfi->nrfailures_ <= maxhostfailures_ ) return SomeFailed;

    int totltim = hfi->starttime_ ? Time_getMilliSeconds()-hfi->starttime_ : -1;
    if ( totltim < 0 && hfi->starttime_ ) totltim += 86486400;
	    
    if ( totltim > 0 && totltim <= hosttimeout_ ) // default: 10 mins.
	return SomeFailed;

    if ( totltim <= 0 && !hfi->nrsucces_ )
    {
	BufferString msg( "Time since start (" );
	msg += totltim; msg += ") < 0 for ";

	msg += hfi->hostdata_.name();
	msg += "\n nrfail: "; msg += hfi->nrfailures_;
	msg += "\n nrsucc: "; msg += hfi->nrsucces_;
	msg += "\n startime: "; msg += hfi->starttime_;
	msg += "\n last succes time: "; msg += hfi->lastsuccess_;

	DBG::message(msg);
	return HostFailed; 
    }

    int lastsuctim = hfi->lastsuccess_ ?
			    Time_getMilliSeconds() - hfi->lastsuccess_ : -1; 
    if ( lastsuctim < 0 && hfi->lastsuccess_ ) lastsuctim += 86486400;

    if ( lastsuctim < 0 )  return HostFailed;
    
    if ( hfi->nrsucces_ > 0 && lastsuctim < hosttimeout_ ) return SomeFailed;

    return HostFailed;
}


int JobRunner::jobsDone() const
{
    int nr = 0;
    for ( int ijob=0; ijob<jobinfos_.size(); ijob++ )
	if ( jobinfos_[ijob]->state_ == JobInfo::Completed )
	    nr++;
    return nr;
}


int JobRunner::jobsInProgress() const
{
    int nr = 0;
    for ( int ijob=0; ijob<jobinfos_.size(); ijob++ )
    {
	const JobInfo& ji = *jobinfos_[ijob];
	if ( ji.state_ != JobInfo::ToDo && ji.state_ != JobInfo::Completed )
	    nr++;
    }
    return nr;
}


const char* JobRunner::message() const
{
    return "Processing";
}


const char* JobRunner::nrDoneMessage() const
{
    return "Jobs completed";
}


void JobRunner::setNiceNess( int n )
{
    niceval_ = n;
    if ( iomgr__ ) iomgr__->setNiceNess(n);
}


bool JobRunner::haveIncomplete() const
{
    for ( int ijob=0; ijob<jobinfos_.size(); ijob++ )
	if ( jobinfos_[ijob]->state_ != JobInfo::Completed )
	    return true;
    return false;
}


bool JobRunner::stopAll()
{
    while( hostinfo_.size() > 0 )
	removeHost(0); 
	
    return true;
}


int JobRunner::doCycle()
{
    updateJobInfo();

    if ( !haveIncomplete() )
	return Finished;

    // Put idle hosts to work
    ObjectSet<HostNFailInfo> hinf( hostinfo_ );
    for ( int ih=0; ih<hinf.size(); ih++ )
    {
	HostNFailInfo* hfi = hinf[ih];
	if ( hostStatus(hfi) == HostFailed )
	    continue;

	JobInfo* ji = currentJob( hfi );
	if ( !ji )
	    assignJob( *hfi );
    }

    return haveIncomplete() ? MoreToDo : Finished;
}


void JobRunner::updateJobInfo()
{
    ObjQueue<StatusInfo>& queue = iomgr().statusQueue();

    while( StatusInfo* si = queue.next() )
    {
	handleStatusInfo( *si );
	delete si;
    }

    for ( int ijob=0; ijob<jobinfos_.size(); ijob++ )
    {
	JobInfo& ji = *jobinfos_[ijob];

	if ( isAssigned(ji)  )
	{
	    int now = Time_getMilliSeconds();
	    if ( !ji.starttime_ ) { pErrMsg("huh?"); ji.starttime_ = now; }

	    int since_lst_chk = now - ji.starttime_; 
	    if ( since_lst_chk > starttimeout_ )
	    {
		int since_lst_recv = ji.recvtime_ ? now - ji.recvtime_ : -1;

		if ( since_lst_recv < 0 || since_lst_recv > failtimeout_ )
		{
		    ji.statusmsg_ = "Timed out.";
		    failedJob( ji, JobInfo::HostFailed );
		
		    if ( ji.hostdata_ )
			iomgr().removeJob( ji.hostdata_->name(), ji.descnr_ );
		}
	    }
	}
    }
}


void JobRunner::showMachStatus( BufferStringSet& res ) const
{
    for ( int ijob=0; ijob<jobinfos_.size(); ijob++ )
    {
	JobInfo& ji = *jobinfos_[ijob];

	bool active = isAssigned(ji);
	    
	if ( active && ji.hostdata_ )
	{
	    BufferString* mch = new BufferString( ji.hostdata_->name() );

	    *mch += " -:- ";
	    *mch += ji.statusmsg_;

	    res += mch;
	}
    }
}


void JobRunner::handleStatusInfo( StatusInfo& si )
{
    JobInfo* ji = gtJob( si.descnr );
    if ( !ji ) return;

    curjobinfo_ = ji;

    ji->infomsg_ = "";
    ji->recvtime_ = si.timestamp;
    if ( si.msg.size() ) ji->infomsg_ = si.msg;

    switch( si.tag )
    {
    case mPID_TAG :
	ji->osprocid_ = si.status;
	ji->state_ = JobInfo::Working;
	ji->statusmsg_ = " inititalising";
    break;
    case mPROC_STATUS :
	ji->nrdone_ = si.status;

	ji->statusmsg_ = " active; ";
	ji->statusmsg_ += ji->nrdone_;
	ji->statusmsg_ += " traces done.";
    break;
    case mCTRL_STATUS :
        switch( si.status )
	{
	case mSTAT_WORKING:
	    ji->state_ = JobInfo::Working;
	    ji->statusmsg_ = " active";
	break;
	case mSTAT_JOBERROR:
	    failedJob( *ji, JobInfo::JobFailed );
	break;
	case mSTAT_HSTERROR:
	    failedJob( *ji, JobInfo::HostFailed );
	break;
	case mSTAT_PAUSED:
	    ji->state_ = JobInfo::Paused;
	    ji->statusmsg_ = " paused";
	break;
	case mSTAT_FINISHED:
	    ji->state_ = JobInfo::Completed;
	    ji->statusmsg_ = " finished";
	break;
	}
    break;
    case mEXIT_STATUS :
        switch( si.status )
	{
	case mSTAT_DONE:
	{
	    ji->state_ = JobInfo::Completed;
	    ji->statusmsg_ = " finished";
	    HostNFailInfo* hfi = hostNFailInfoFor( ji->hostdata_ );
	    if ( hfi )
	    {
		hfi->nrsucces_++;
		hfi->lastsuccess_ = Time_getMilliSeconds();
	    }
	}
	break;

	case mSTAT_JOBERROR:
	    failedJob( *ji, JobInfo::JobFailed );
	break;

	case mSTAT_HSTERROR:
	case mSTAT_KILLED: 
	    failedJob( *ji, JobInfo::HostFailed );
	break;

	default:
	    ji->infomsg_ = "Unknown exit status received.";
	    failedJob( *ji, JobInfo::HostFailed );
	break;
	}
	iomgr().removeJob( si.hostnm , si.descnr );
    break;
    }

    if ( ji->infomsg_.size() ) // not already handled by failedJob()
    {
	if ( mDebugOn )
	{
	    BufferString msg("----\nJobRunner::handleStatusInfo: info for job");
	    msg += ji->descnr_;	msg += " : "; 
	    mAddDebugMsg( (*ji) )
	    DBG::message(msg);
	}
  
	msgAvail.trigger();
    }
}


JobInfo* JobRunner::gtJob( int descnr )
{
    JobInfo* ret =0;

    for ( int idx=0; idx<jobinfos_.size(); idx++ )
	if ( jobinfos_[idx]->descnr_ == descnr )
	    ret = jobinfos_[idx];

    if ( !ret )
    {
	for ( int idx=0; idx<failedjobs_.size(); idx++ )
	    if ( failedjobs_[idx]->descnr_ == descnr )
		ret = failedjobs_[idx];
    }
	

    return ret;
}

