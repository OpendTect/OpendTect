/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Oct 2004
 RCS:           $Id: jobrunner.cc,v 1.19 2005-03-30 11:19:23 cvsarend Exp $
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
#include <iostream>

#define mDebugOn        (DBG::isOn(DBG_MM))


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
	, jobtimeout_(		atoi( getenv("DTECT_JOB_TIMEOUT")
				    ? getenv("DTECT_JOB_TIMEOUT"): "120000") )
	, hosttimeout_(		atoi( getenv("DTECT_HOST_TIMEOUT")
				    ? getenv("DTECT_HOST_TIMEOUT"): "600000") )
    	, maxhostfailures_(	atoi( getenv("DTECT_MAX_HOSTFAIL")
				    ? getenv("DTECT_MAX_HOSTFAIL") : "2") )
    	, maxjobfailures_(	atoi( getenv("DTECT_MAX_JOBFAIL")
				    ? getenv("DTECT_MAX_JOBFAIL") : "2") )
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
    }
    else
	return true; // host already in use

    bool res = assignJob( *hfi );
    if ( isnew )
    {
	if ( res )
	    hostinfo_ += hfi;
	else
	    delete hfi;
    }

    return res;
}


bool JobRunner::assignJob( HostNFailInfo& hfi )
{
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
		{ if ( !tryfailed ) continue; }
	    else if ( !isnew )
		continue;

	    StartRes res = startJob( ji, hfi );
	    if ( res == HostBad )
		return false;
	    else if ( res == Started )
		return true;
	}
    }

    return false;
}


JobRunner::StartRes JobRunner::startJob( JobInfo& ji, HostNFailInfo& hfi )
{
    if ( hostStatus(&hfi) == HostFailed ) return HostBad;

    if ( ji.failures_ > maxjobfailures_ )
    {
	jobinfos_ -= &ji; failedjobs_ += &ji;
	return JobBad;
    }

    if ( !runJob(ji,hfi.hostdata_) )
    {
	return hostStatus(&hfi) == HostFailed ? HostBad : NotStarted;
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

    if ( ji.hostdata_ ) 
    {
	if ( reason == JobInfo::JobFailed ) ji.failures_++;

	HostNFailInfo* hfi = hostNFailInfoFor( ji.hostdata_ );
	if ( hfi ) hfi->nrfailures_++;
    }

    if ( mDebugOn )
    {
	BufferString msg("JobRunner::failedJob : ");
	if ( ji.state_ == JobInfo::HostFailed ) msg += "host failed. ";
	if ( ji.state_ == JobInfo::JobFailed ) msg += "job failed. ";
	msg += "\n Job failed "; msg += ji.failures_; msg += " times ";
	msg += "\n Failed on host: "; msg += ji.hostdata_->name();
	msg += "\n Status: "; msg += ji.statusmsg_;
	msg += "\n Info: "; msg += ji.infomsg_;

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
    ji.timestamp_ = 0;
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

    if ( !hfi->nrfailures_ ) return OK;

    if ( hfi->nrfailures_ <= maxhostfailures_ ) return SomeFailed;

    int totltim = hfi->starttime_ ? Time_getMilliSeconds() - hfi->starttime_
				  : -1;

    if ( totltim < 0 ) { pErrMsg( "huh?" ); return SomeFailed; }

    if ( totltim <= hosttimeout_ ) // default: 10 mins.
	return SomeFailed;

    int lastsuctim = hfi->lastsuccess_ ?
			    Time_getMilliSeconds() - hfi->lastsuccess_ : -1; 

    if ( lastsuctim < -1 ) { pErrMsg( "huh?" ); return HostFailed; }
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
	    if ( !ji.timestamp_ ) ji.timestamp_ = Time_getMilliSeconds();

	    int elapsed = Time_getMilliSeconds() - ji.timestamp_; 

	    if ( elapsed > jobtimeout_ )
	    {
		ji.statusmsg_ = "Timed out.";
		failedJob( ji, JobInfo::HostFailed );
		
		if ( ji.hostdata_ )
		    iomgr().removeJob( ji.hostdata_->name(), ji.descnr_ );
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
    ji->timestamp_ = si.timestamp;
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
	msgAvail.trigger();
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

