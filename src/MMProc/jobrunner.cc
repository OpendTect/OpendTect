/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Oct 2004
 RCS:           $Id: jobrunner.cc,v 1.1 2004-10-25 07:26:20 bert Exp $
________________________________________________________________________

-*/

#include "jobrunner.h"
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
    	, nrhostattempts_(2)
    	, nrjobattempts_(2)
    	, rshcomm_("rsh")
	, niceval_(19)
	, portnr_(19063)
    	, prog_(cmd)
{
    FilePath fp( GetDataDir() );
    fpproc.add( "Proc" ).add( tmpfnm_base );
    BufferString procdir_ = fpproc.fullPath();
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
	if ( &hostinfo_[idx]->hostdata == &hd )
	    { jhi = hostinfo_[idx]; break; }

    if ( !jhi )
	for ( int idx=0; idx<failedhosts_.size(); idx++ )
	    if ( &failedhosts_[idx]->hostdata == &hd )
		{ jhi = failedhosts_[idx]; break; }

    return const_cast<JobHostInfo*>( jhi );
}


bool JobRunner::addHost( const HostData& hd )
{
    JobHostInfo* jhi = jobHostInfoFor( hd );
    if ( !jhi )
	jhi = new HostDataInfo( hd );
    else if ( failedhosts_.indexOf(jhi) >= 0 )
    {
	// Is failed, but apparently need to try again ...
	failedhosts_ -= jhi;
	jhi->failures_ = 0;
    }
    else
	// already in 'good hosts' ...
	return false;

    // First go for new jobs
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

	    hostinfo_ += hdi;
	    StartRes res = startJob( ji, *hdi );
	    if ( res == HostSucks || res == Started )
		return res == Started;
	}
    }

    delete jhi;
    return false;
}


StartRes JobRunner::startJob( JobInfo& ji, JobHostInfo& jhi )
{
    ji.attempts++;
    if ( ji.attempts > nrjobattempts_ )
    {
	jobinfos_ -= &ji; failedjobs_ += &ji;
	return JobSucks;
    }

    if ( !runJob(ji,jhi.hostdata_) )
    {
	jhi.nrfailed_++;
	return jhi.nrfailed_ > nrhostattempts_ ? HostSucks : NotStarted;
    }

    jhi.nrfailed_ = 0;
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
	if ( ji->hostdata_ == &jhi->hostdata_
	  && (ji->state_ == JobInfo::Working || ji->state_ == JobInfo::Paused) )
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

    //TODO pause or un-pause
    if ( ji->state_ == JobInfo::Working && yn )
    else if ( ji->state_ == JobInfo::Paused && !yn )
}


bool JobRunner::isPaused( int hnr ) const
{
    if ( hnr < 0 || hnr >= hostinfo_.size() )
	return false;
    JobInfo* ji = currentJob( hostinfo_[hnr] );
    return ji ? ji->state_ == JobInfo::Paused : false;
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
	if ( jobinfos_[ijob]->state_ != JobInfo::Complete )
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
	JobInfo* ji = currentJob( jhi );
	if ( !ji )
	{
	    hostinfo_ -= jhi;
	    if ( !addHost(jhi->hostdata_) )
		hostinfo_ += jhi;
	}
    }

    return haveIncomplete() ? MoreToDo : Finished;
}
