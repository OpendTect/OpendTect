/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Oct 2004
 RCS:           $Id: jobrunner.cc,v 1.13 2004-11-11 16:04:07 arend Exp $
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
#include <iostream>

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
    	, maxhostfailures_(2)
    	, maxjobfailures_(2)
    	, rshcomm_("rsh")
	, niceval_(19)
	, firstport_(19636)
    	, prog_(cmd)
	, timeout_( atoi( getenv("DTECT_JOBMAN_TIMEOUT")
			? getenv("DTECT_JOBMAN_TIMEOUT") : "120000") )
	, jobStarted(this)
	, jobFailed(this)
    	, notifyji(0)
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


JobHostInfo* JobRunner::jobHostInfoFor( const HostData& hd ) const
{
    const JobHostInfo* jhi = 0;

    for ( int idx=0; idx<hostinfo_.size(); idx++ )
	if ( &hostinfo_[idx]->hostdata_ == &hd )
	    { jhi = hostinfo_[idx]; break; }

    return const_cast<JobHostInfo*>( jhi );
}


bool JobRunner::addHost( const HostData& hd )
{
    JobHostInfo* jhi = jobHostInfoFor( hd );
    const bool isnew = !jhi;
    if ( isnew )
	jhi = new JobHostInfo( hd );
    else if ( isFailed(jhi) )
	jhi->nrfailures_ = 0;
    else
	return true; // host already in use

    bool res = assignJob( *jhi );
    if ( isnew )
    {
	if ( res )
	    hostinfo_ += jhi;
	else
	    delete jhi;
    }

    return res;
}


bool JobRunner::assignJob( JobHostInfo& jhi )
{
    // First go for new jobs, then try failed
    for ( int tryfailed=0; tryfailed<2; tryfailed++ )
    {
	for ( int ijob=0; ijob<jobinfos_.size(); ijob++ )
	{
	    JobInfo& ji = *jobinfos_[ijob];
	    bool isnew = ji.state_ == JobInfo::ToDo;
	    bool isfailed = ji.state_ == JobInfo::Failed;
	    if ( isfailed )
		{ if ( !tryfailed ) continue; }
	    else if ( !isnew )
		continue;

	    StartRes res = startJob( ji, jhi );
	    if ( res == HostBad )
		return false;
	    else if ( res == Started )
		return true;
	}
    }

    return false;
}


JobRunner::StartRes JobRunner::startJob( JobInfo& ji, JobHostInfo& jhi )
{
    ji.attempts_++;
    if ( ji.attempts_ > maxjobfailures_ )
    {
	jobinfos_ -= &ji; failedjobs_ += &ji;
	return JobBad;
    }

    if ( !runJob(ji,jhi.hostdata_) )
    {
	jhi.nrfailures_++;
	return jhi.nrfailures_ > maxhostfailures_ ? HostBad : NotStarted;
    }

    jhi.nrfailures_ = 0;
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
    

bool JobRunner::runJob( JobInfo& ji, const HostData& hd )
{
    IOPar iop; descprov_->getJob( ji.descnr_, iop );

    FilePath basefp( getBaseFilePath(ji,hd) );

    notifyji = &ji;
    ji.hostdata_ = &hd;
    if ( !iomgr().startProg( prog_, iop, basefp, ji, rshcomm_ ) )
    {
	ji.state_ = JobInfo::Failed;
	iomgr().fetchMsg(ji.curmsg_);
	jobFailed.trigger();
	return false;
    }

    ji.state_ = JobInfo::Working;
    jobStarted.trigger();
    return true;
}


JobInfo* JobRunner::currentJob( const JobHostInfo* jhi ) const
{
    for ( int idx=0; idx<jobinfos_.size(); idx++ )
    {
	const JobInfo* ji = jobinfos_[idx];
	if ( ji->hostdata_ == &jhi->hostdata_ && isAssigned(*ji) )
	    return const_cast<JobInfo*>( ji );
    }
    return 0;
}


void JobRunner::removeHost( int hnr )
{
    if ( hnr < 0 || hnr >= hostinfo_.size() )
	return;
    JobHostInfo* jhi = hostinfo_[hnr];
    hostinfo_ -= jhi;

    JobInfo* ji = currentJob( jhi );

    if ( ji )
    {
	notifyji = ji;
	ji->hostdata_ = &jhi->hostdata_;
	ji->state_ = JobInfo::Failed;
	jobFailed.trigger();

	iomgr().reqModeForJob( *ji, JobIOMgr::Stop );

	if ( ji->hostdata_ )
	    iomgr().removeJob( ji->hostdata_->name(), ji->descnr_ );
    }

    delete jhi;
}


void JobRunner::pauseHost( int hnr, bool yn )
{
    if ( hnr < 0 || hnr >= hostinfo_.size() )
	return;
    JobInfo* ji = currentJob( hostinfo_[hnr] );
    if ( !ji ) return;

    if ( ji->state_ == JobInfo::Working && yn )
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
    return ji.state_ == JobInfo::Paused || ji.state_ == JobInfo::Working;
}


bool JobRunner::isFailed( int hnr ) const
{
    return hnr < 0 || hnr >= hostinfo_.size() ? true
	 : isFailed( hostinfo_[hnr] );
}



bool JobRunner::isFailed( const JobHostInfo* jhi ) const
{
    return jhi ? jhi->nrfailures_ > maxhostfailures_ : true;
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
    //TODO get rid of all processes - set all statuses to Failed
    return true;
}


int JobRunner::doCycle()
{
    updateJobInfo();

    if ( !haveIncomplete() )
	return Finished;

    // Put idle hosts to work
    ObjectSet<JobHostInfo> hinf( hostinfo_ );
    for ( int ih=0; ih<hinf.size(); ih++ )
    {
	JobHostInfo* jhi = hinf[ih];
	if ( isFailed(jhi) )
	    continue;

	JobInfo* ji = currentJob( jhi );
	if ( !ji )
	    assignJob( *jhi );
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

	if ( !ji.timestamp_ ) ji.timestamp_ = Time_getMilliSeconds();

	int elapsed = Time_getMilliSeconds() - ji.timestamp_; 

	if ( ji.state_ == JobInfo::Working && elapsed > timeout_ )
	{
	    ji.state_ = JobInfo::Failed;
	    if ( ji.hostdata_ )
		iomgr().removeJob( ji.hostdata_->name(), ji.descnr_ );
	}
    }
}


void JobRunner::showMachStatus( BufferStringSet& res ) const
{
    for ( int ijob=0; ijob<jobinfos_.size(); ijob++ )
    {
	JobInfo& ji = *jobinfos_[ijob];

	bool active = ji.state_ == JobInfo::Working ||
			ji.state_ == JobInfo::Paused;
	    
	if ( active && ji.hostdata_ )
	{
	    BufferString* mch = new BufferString( ji.hostdata_->name() );

	    *mch += " -:- ";
	    *mch += ji.curmsg_;

	    res += mch;
	}
    }
}


void JobRunner::handleStatusInfo( StatusInfo& si )
{
    JobInfo* ji = gtJob( si.descnr );
    if ( !ji ) return;

    if ( si.msg.size() ) ji->curmsg_ = si.msg;
    ji->timestamp_ = si.timestamp;

    switch( si.tag )
    {
    case mPID_TAG :
	ji->osprocid_ = si.status;
    break;
    case mPROC_STATUS :
	ji->nrdone_ = si.status;

	ji->curmsg_ = " active; ";
	ji->curmsg_ += ji->nrdone_;
	ji->curmsg_ += " traces done.";
    break;
    case mCTRL_STATUS :
        switch( si.status )
	{
	case mSTAT_INITING:
	    ji->state_ = JobInfo::Working;
	    ji->curmsg_ = " inititalising";
	break;
	case mSTAT_WORKING:
	    ji->state_ = JobInfo::Working;
	    ji->curmsg_ = " active";
	break;
	case mSTAT_ERROR:
	    notifyji = ji;
	    ji->state_ = JobInfo::Failed;
	    jobFailed.trigger();
	break;
	case mSTAT_PAUSED:
	    ji->state_ = JobInfo::Paused;
	    ji->curmsg_ = " paused";
	break;
	case mSTAT_FINISHED:
	    ji->state_ = JobInfo::Completed;
	    ji->curmsg_ = " finished";
	break;
	}
    break;
    case mEXIT_STATUS :
        switch( si.status )
	{
	case mSTAT_DONE:
	    ji->state_ = JobInfo::Completed;
	    ji->curmsg_ = " finished";
	break;

	case mSTAT_ERROR:
	case mSTAT_KILLED: 
	    ji->state_ = JobInfo::Failed;
	break;

	default:
	    ji->curmsg_ = "Unknown exit status received.";
	    ji->state_ = JobInfo::Failed;
	break;
	}
	iomgr().removeJob( si.hostnm , si.descnr );
    break;
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

