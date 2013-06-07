/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Oct 2004
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "jobrunner.h"

#include "hiddenparam.h"
#include "jobinfo.h"
#include "jobiomgr.h"
#include "jobdescprov.h"
#include "hostdata.h"
#include "filepath.h"
#include "file.h"
#include "iopar.h"
#include "jobiomgr.h"
#include "queue.h"
#include "mmdefs.h"
#include "envvars.h"
#include "timefun.h"
#include "debugmasks.h"
#include "oddirs.h"
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
    msg += "\n Info: "; msg += ji.infomsg_; \
    msg += "\n Last received update: "; \
    msg += Time::passedSince(ji.recvtime_)/1000; \
    msg += " seconds ago.";


const BufferString& getTempBaseNm()
{
    static BufferString tmpfnm_base;
    if ( tmpfnm_base.isEmpty() )
    {
	tmpfnm_base = HostData::localHostName();
	tmpfnm_base += "_";
	tmpfnm_base += GetPID();
    }
    return tmpfnm_base;
}


int mkTmpFileNr()
{
    return 1;
}
 

mGlobal int& MMJob_getTempFileNr(); // keep compiler happy
int& MMJob_getTempFileNr()
{
    static int tmpfile_nr = 1;
    return tmpfile_nr;
}

HiddenParam<JobRunner,BufferString> errmsg_("");
JobRunner::JobRunner( JobDescProv* p, const char* cmd )
	: Executor("Running jobs")
	, iomgr__(0)
	, descprov_(p)
    	, rshcomm_("rsh")
	, niceval_(19)
	, firstport_(19636)
    	, prog_(cmd)
	, starttimeout_( 1000 * GetEnvVarIVal("DTECT_MM_START_TO",   45 ) )
	, failtimeout_(  1000 * GetEnvVarIVal("DTECT_MM_FAIL_TO",    450 ) )
	, wrapuptimeout_(1000 *	GetEnvVarIVal("DTECT_MM_WRAPUP_TO",  1800 ) )
	, hosttimeout_(  1000 * GetEnvVarIVal("DTECT_MM_HOST_TO",    600 ) )
    	, maxhostfailures_(	GetEnvVarIVal("DTECT_MM_MX_HSTFAIL", 5 ) )
    	, maxjobfailures_(	GetEnvVarIVal("DTECT_MM_MX_JOBFAIL", 2 ) )
    	, maxjobhstfails_(	GetEnvVarIVal("DTECT_MM_MX_JOBHSTF", 7 ) )
	, startwaittime_ (	GetEnvVarIVal("DTECT_MM_START_WAIT", 1000 ) )
	, preJobStart(this)
	, postJobStart(this)
	, jobFailed(this)
	, msgAvail(this)
    	, curjobiop_(*new IOPar)
    	, curjobfp_(*new FilePath)
    	, curjobinfo_(0)
{
    procdir_ = GetProcFileName( getTempBaseNm() );
    procdir_ += "_"; procdir_ += MMJob_getTempFileNr();
    MMJob_getTempFileNr()++;
    errmsg_.setParam( this, "" );

    if ( File::exists(procdir_) && !File::isDirectory(procdir_) )
	File::remove(procdir_);
    if ( !File::exists(procdir_) )
	File::createDir(procdir_);

    if ( mIsUdf(descprov_->nrJobs()) || !descprov_->nrJobs() )
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
	    { hfi = const_cast<HostNFailInfo*>(hostinfo_[idx]); break; }

    return hfi;
}


bool JobRunner::addHost( const HostData& hd )
{
    if ( !iomgr().isReady() )
    {
	delete iomgr__;
	iomgr__ = 0;
	BufferString errmsg;
	errmsg = BufferString( "Failed to listen to Port ", firstport_, " on ");
	errmsg += HostData::localHostName();
	errmsg_.setParam( this, errmsg );
	return false;
    }

    HostNFailInfo* hfi = hostNFailInfoFor( &hd );
    const bool isnew = !hfi;
    if ( isnew )
    {
	hfi = new HostNFailInfo( hd );
	hfi->starttime_ = Time::getMilliSeconds();
    }
    else if ( !hfi->inuse_ )
    {
	hfi->nrfailures_ = 0;
	hfi->starttime_ = Time::getMilliSeconds();
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
    if ( hfi.inuse_ ) return NotReady;
    
    static int timestamp = -1;

    int elapsed = Time::passedSince( timestamp );
    if ( elapsed < 0 ) timestamp = Time::getMilliSeconds();

    if ( elapsed < startwaittime_ ) return NotReady; 

    timestamp = Time::getMilliSeconds();
    
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
		if ( (ji.hostdata_ && ji.hostdata_ == &hfi.hostdata_)
		     && hostinfo_.size() > 1 ) continue;
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
    if ( hfi.inuse_ ) { pErrMsg("huh?"); return NotStarted; }

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

    hfi.inuse_ = true;
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
#ifdef __win__
    replaceCharacter( basenm.buf(), '.',  '_' );
#endif
    basenm += "_"; basenm += ji.descnr_;

    basefp = procdir_;
    basefp.add( basenm );

    return basefp;
}
   

void JobRunner::failedJob( JobInfo& ji, JobInfo::State reason )
{ 
    HostNFailInfo* hfi = 0;
    if ( ji.hostdata_ )
    {
	hfi = hostNFailInfoFor( ji.hostdata_ );
	if ( hfi ) hfi->inuse_ = false;

	iomgr().removeJob( ji.hostdata_->name(), ji.descnr_ );
    }

    if ( !isAssigned(ji) )
    {
	if ( mDebugOn )
	{
	    BufferString msg("----\nJobRunner::failedJob UNASSIGNED! : ");
	    if ( ji.state_ == JobInfo::HostFailed ) msg += "host failed. ";
	    if ( ji.state_ == JobInfo::JobFailed ) msg += "job failed. ";
	    mAddDebugMsg( ji )
	    DBG::message(msg);
	}
       	return;
    }

    curjobinfo_ = &ji;
    ji.state_ = reason;

    if ( reason == JobInfo::JobFailed ) ji.jobfailures_++;
    else
    {
	ji.hstfailures_++;
	if ( hfi ) hfi->nrfailures_++;
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
	    msg += Time::passedSince(hfi->starttime_)/1000;
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
    curjobiop_.setEmpty(); descprov_->getJob( ji.descnr_, curjobiop_ );
    curjobfp_ = getBaseFilePath(ji,hd);
    curjobinfo_ = &ji;
    ji.hostdata_ = &hd;
    ji.state_ = JobInfo::Scheduled;
    ji.starttime_ = Time::getMilliSeconds();
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
	const_cast<HostNFailInfo*>(hfi)->starttime_ = Time::getMilliSeconds();
    }

    if ( !hfi->nrfailures_ ) return OK;

    if ( hfi->nrfailures_ <= maxhostfailures_ ) return SomeFailed;

    int totltim = Time::passedSince( hfi->starttime_ );
	    
    if ( totltim > 0 && totltim <= hosttimeout_ ) // default: 10 mins.
	return SomeFailed;

    if ( totltim <= 0 && !hfi->nrsucces_ )
    {
	BufferString msg( "Time since start (" );
	msg += totltim; msg += ") <= 0 for ";

	msg += hfi->hostdata_.name();
	msg += "\n nrfail: "; msg += hfi->nrfailures_;
	msg += "\n nrsucc: "; msg += hfi->nrsucces_;
	msg += "\n startime: "; msg += hfi->starttime_;
	msg += "\n last succes time: "; msg += hfi->lastsuccess_;

	DBG::message(msg);
	return HostFailed; 
    }

    int lastsuctim = Time::passedSince( hfi->lastsuccess_ );

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
    while( !hostinfo_.isEmpty() )
	removeHost(0); 
	
    return true;
}


int JobRunner::doCycle()
{
    if ( !iomgr().isReady() )
	return ErrorOccurred();
    

    updateJobInfo();

    if ( !haveIncomplete() )
	return Finished();

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

    return haveIncomplete() ? MoreToDo() : Finished();
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
	    if ( !ji.starttime_ ) { pErrMsg("huh?"); Time::getMilliSeconds(); }

	    int since_lst_chk = Time::passedSince( ji.starttime_ ); 
	    if ( since_lst_chk > starttimeout_ )
	    {
		int since_lst_recv = ji.recvtime_ ?
					Time::passedSince(ji.recvtime_) : -1;
		int to = ji.state_ == JobInfo::WrappingUp ? wrapuptimeout_
							  : failtimeout_;

		if ( since_lst_recv < 0 || since_lst_recv > to )
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
	const JobInfo& ji = *jobinfos_[ijob];
	const bool active = isAssigned(ji);
	    
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
	ji->statusmsg_ = " initialising";
    break;
    case mPROC_STATUS :
	ji->nrdone_ = si.status;

	ji->statusmsg_ = " active; ";
	ji->statusmsg_ += ji->nrdone_;
	ji->statusmsg_ += " traces done.";

	if ( si.procid > 0 && ji->osprocid_ > 0  && si.procid != ji->osprocid_ )
	    failedJob( *ji, JobInfo::JobFailed );

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
	case mSTAT_WRAPUP:
	    ji->state_ = JobInfo::WrappingUp;
	    ji->statusmsg_ = " storing result";
	break;
	case mSTAT_FINISHED:
	    ji->state_ = JobInfo::WrappingUp;
	    ji->statusmsg_ = " closing up";
	break;
	}
    break;
    case mEXIT_STATUS :
        switch( si.status )
	{
	case mSTAT_ALLDONE:
	{
	    ji->state_ = JobInfo::Completed;
	    ji->statusmsg_ = " all done";
	    HostNFailInfo* hfi = hostNFailInfoFor( ji->hostdata_ );
	    if ( hfi )
	    {
		hfi->nrsucces_++;
		hfi->lastsuccess_ = Time::getMilliSeconds();
		hfi->inuse_ = false;
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


const char* JobRunner::errorMsg() const
{   return errmsg_.getParam(this).size() ? errmsg_.getParam(this).buf() : 0; } 
