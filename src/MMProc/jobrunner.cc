/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Oct 2004
 RCS:           $Id: jobrunner.cc,v 1.2 2004-10-25 11:58:59 bert Exp $
________________________________________________________________________

-*/

#include "jobrunner.h"
#include "jobinfo.h"
#include "jobdescprov.h"
#include "hostdata.h"
#include "filepath.h"
#include "filegen.h"
#include "iopar.h"
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

#define mMkFname(ext) \
    fname = ""; \
    fname += machine ? machine->name() : ""; \
    fname += "_"; fname += jobid_; fname += ext; \
    fname = FilePath( basename ).add( fname ).fullPath()


JobRunner::JobRunner( JobDescProv* p, const char* cmd )
	: Executor("Running jobs")
	, descprov_(p)
    	, maxhostfailures_(2)
    	, maxjobfailures_(2)
    	, rshcomm_("rsh")
	, niceval_(19)
	, firstport_(19163)
    	, prog_(cmd)
{
    FilePath fp( GetDataDir() );
    fp.add( "Proc" ).add( tmpfnm_base );
    BufferString procdir_ = fp.fullPath();
    procdir_ += "_"; procdir_ += tmpfile_nr;
    tmpfile_nr++;

    if ( File_exists(procdir_) && !File_isDirectory(procdir_) )
	File_remove(procdir_,NO);
    if ( !File_exists(procdir_) )
	File_createDir(procdir_,0);

    for ( int idx=0; idx<descprov_->nrJobs(); idx++ )
	jobinfos_ += new JobInfo( idx );
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


bool JobRunner::runJob( JobInfo& ji, const HostData& hd )
{
    IOPar iop; descprov_->getJob( ji.descnr_, iop );
    ji.osprocid_ = startProg( hd, iop, ji.descnr_, ji.curmsg_ );
    if ( ji.osprocid_ < 0 )
    {
	ji.state_ = JobInfo::Failed;
	return false;
    }

    ji.state_ = JobInfo::Working;
    ji.hostdata_ = &hd;
    return true;
}


bool JobRunner::startProg( const HostData& hd, IOPar& iop, int jid,
			   BufferString& msg )
{
    FilePath fp( procdir_ ); fp.add( hd.name() );
    BufferString basenm = fp.fullPath();
    basenm += "_"; basenm += jid;
    //TODO add .par and .log and do remote stuff. Then do it.

    return true;
}


JobInfo* JobRunner::currentJob( const JobHostInfo* jhi ) const
{
    for ( int idx=0; idx<jobinfos_.size(); idx++ )
    {
	const JobInfo* ji = jobinfos_[idx];
	if ( ji->hostdata_ == &jhi->hostdata_ && isAssigned(idx) )
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
    //TODO kill remote process

    delete jhi;
}


void JobRunner::pauseHost( int hnr, bool yn )
{
    if ( hnr < 0 || hnr >= hostinfo_.size() )
	return;
    JobInfo* ji = currentJob( hostinfo_[hnr] );
    if ( !ji ) return;

    if ( ji->state_ == JobInfo::Working && yn )
	; //TODO pause
    else if ( ji->state_ == JobInfo::Paused && !yn )
	; //TODO un-pause
}


bool JobRunner::isPaused( int hnr ) const
{
    if ( hnr < 0 || hnr >= hostinfo_.size() )
	return false;
    const JobInfo* ji = currentJob( hostinfo_[hnr] );
    return ji ? ji->state_ == JobInfo::Paused : false;
}


bool JobRunner::isAssigned( int hnr ) const
{
    if ( hnr < 0 || hnr >= hostinfo_.size() )
	return false;
    const JobInfo* ji = currentJob( hostinfo_[hnr] );
    return !ji ? false
	 : ji->state_ == JobInfo::Paused || ji->state_ == JobInfo::Working;
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


bool JobRunner::haveIncomplete() const
{
    for ( int ijob=0; ijob<jobinfos_.size(); ijob++ )
	if ( jobinfos_[ijob]->state_ != JobInfo::Completed )
	    return true;
    return false;
}


int JobRunner::doCycle()
{
    //TODO
    // getNewJobInfo();

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
