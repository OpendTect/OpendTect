/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          April 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiclusterproc.h"

#include "dirlist.h"
#include "timer.h"
#include "file.h"
#include "filepath.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"
#include "manobjectset.h"
#include "seistrctr.h"
#include "strmdata.h"
#include "strmoper.h"
#include "survinfo.h"
#include "strmprov.h"
#include "envvars.h"
#include "keystrs.h"
#include "seissingtrcproc.h"
#include "settings.h"

#include "uiclusterjobprov.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiprogressbar.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include <iostream>


static int getExitStatus( const char* logfile, BufferString& msg )
{
    StreamData sd = StreamProvider(logfile).makeIStream();
    if ( !sd.usable() )
	return -1;

    StrmOper::seek( *sd.istrm, -50, std::ios::end );
    char buf[51];
    if ( !StrmOper::readBlock(*sd.istrm,(void*) buf,50) )
	return -1;

    buf[50] = '\0';
    char* ptr = buf + 49;
    while ( --ptr && ptr > buf )
	if ( *ptr == '\n' )
	    break;

    msg = ptr;
    ptr = strstr( msg.buf(), "finished with code" );
    if ( !ptr )
    {
	msg.setEmpty();
	return -1;
    }

    ptr += 19;
    *(ptr + 1) = '\0';
    return atoi( ptr );
}


struct ClusterJobInfo
{
    			ClusterJobInfo(const char* logfnm, const char* desc )
			    : status_(-2),logfnm_(logfnm), desc_(desc) {}

    int			status_;	// 0=finished,1=error,-2=notsubmitted,
    					// -1=submitted
    BufferString	logfnm_;
    BufferString	desc_;
};


class ClusterJobSubmitter : public ParallelTask
{
public:
ClusterJobSubmitter( ObjectSet<ClusterJobInfo>& jobs, const char* cmd )
    : ParallelTask("Cluster Job Submitter")
    , cmd_(cmd)
    , jobs_(jobs)
{
    for ( int idx=0; idx<jobs_.size(); idx++ )
    {
	if ( jobs_[idx]->status_ == 1 || jobs_[idx]->status_ == -2 )
	    jobstodo_ += idx;
    }
}

od_int64 nrIterations() const
{ return jobstodo_.size(); }

bool doWork( od_int64 start, od_int64 stop, int )
{
    for ( int idx=mCast(int,start); idx<=stop; idx++ )
    {
	BufferString jobcmd( cmd_ );
	jobcmd += " \'";
	FilePath fp( jobs_[jobstodo_[idx]]->logfnm_ );
	fp.setExtension( ".scr" );
	jobcmd += fp.fullPath();
	jobcmd += "\' &";
	if ( system(jobcmd) )
	    continue;
	
	jobs_[jobstodo_[idx]]->status_ = -1;
	addToNrDone(1);
    }

    return true;
}

protected:

    BufferString	cmd_;
    ObjectSet<ClusterJobInfo>&	jobs_;
    TypeSet<int>	jobstodo_;
};


class ClusterProc : public NamedObject
{
public:
ClusterProc( const IOPar& pars )
    : pars_(pars)
{
}

bool init()
{
    FixedString res = pars_.find( sKey::Survey() );
    if ( !res.isEmpty() && SI().getDirName() != res )
	IOMan::setSurvey( res.str() );

    FixedString scriptdir = pars_.find( "Script dir" );
    if ( scriptdir.isEmpty() || !File::isDirectory(scriptdir.str()) )
	return false;

    DirList scriptfiles( scriptdir, DirList::FilesOnly, "*.scr" );
    for ( int idx=0; idx<scriptfiles.size(); idx++ )
    {
	FilePath fp( scriptfiles.fullPath(idx) );
	fp.setExtension( ".par" );
	IOPar iop;
	if ( !iop.read(fp.fullPath(),sKey::Pars()) )
	    continue;

	FixedString desc = iop.find( sKey::Desc() );
	fp.setExtension( ".log" );
	jobs_ += new ClusterJobInfo( fp.fullPath(), desc.str() );
    }

    return true;
}


int totalNrJobs()
{ return jobs_.size(); };

void checkProgress( int& nrjobsfinished, int& nrjobswitherr, BufferString& msg )
{
    nrjobsfinished = 0;
    nrjobswitherr = 0;
    for ( int idx=0; idx<jobs_.size(); idx++ )
    {
	int status = jobs_[idx]->status_;
	if ( !status )
	{
	    nrjobsfinished++;
	    continue;
	}
	else if ( status == 1 )
	{
	    nrjobswitherr++;
	    continue;
	}
	else if ( status == -2 )
	    continue;

	const int exitstatus = getExitStatus( jobs_[idx]->logfnm_.buf(), msg );
	if ( exitstatus < 0 )
	    continue;

	jobs_[idx]->status_ = exitstatus ? 1 : 0;
    }
}


bool submitJobs( TaskRunner* tr )
{
    FixedString submitcmd = pars_.find( "Command" );
    if ( submitcmd.isEmpty() )
	return false;

    ClusterJobSubmitter jobsubmitter( jobs_, submitcmd.str() );
    if ( !TaskRunner::execute( tr, jobsubmitter ) )
	return false;

    return true;
}

protected:

    const IOPar&		pars_;
    ManagedObjectSet<ClusterJobInfo>	jobs_;
};


uiClusterProc::uiClusterProc( uiParent* p, const IOPar& iop )
    : uiDialog(p,uiDialog::Setup("Cluster Processing","Progress window",""))
    , pars_(iop)
    , scriptdirnm_(iop.find(uiClusterProc::sKeyScriptDir()))
    , proc_(*new ClusterProc(iop))
{
    msgfld_ = new uiTextEdit( this, "Processing progress", true );
    msgfld_->setPrefHeightInChar( 7 );
    msgfld_->setPrefWidth( 500 );

    progbar_ = new uiProgressBar( this );
    progbar_->attach( alignedBelow, msgfld_ );
    progbar_->attach( widthSameAs, msgfld_ );

    proc_.init();
    totalnr_ = proc_.totalNrJobs();
    progbar_->setTotalSteps( totalnr_ );

    BufferString labeltxt( "Number of jobs finished: " );
    labeltxt += "XXXX out of XXXX (XXXX with error)";
    label_ = new uiLabel( this, labeltxt.buf() );
    label_->attach( alignedBelow, progbar_ );

    uiTaskRunner dlg( this );
    proc_.submitJobs( &dlg );

    timer_ = new Timer("uiClusterProc timer");
    timer_->tick.notify( mCB(this,uiClusterProc,progressCB) );
    timer_->start( 500, false );
}


uiClusterProc::~uiClusterProc()
{
    delete timer_;
    delete &proc_;
}


void uiClusterProc::progressCB( CallBacker* )
{
    int nrjobsdone, nrjobswitherr;
    BufferString msg;
    proc_.checkProgress( nrjobsdone, nrjobswitherr, msg );
    progbar_->setProgress( nrjobsdone + nrjobswitherr );
    if ( !msg.isEmpty() )
	msgfld_->append( msg.buf() );

    BufferString labeltxt( "Number of jobs finished: " );
    labeltxt += nrjobsdone + nrjobswitherr;
    labeltxt += " out of "; labeltxt += totalnr_;
    labeltxt += " ("; labeltxt += nrjobswitherr;
    labeltxt += "with error)";
    label_->setText( labeltxt );
    if ( totalnr_ && (nrjobsdone + nrjobswitherr) == totalnr_ )
    {
	timer_->stop();
	uiTaskRunner dlg( this );
	if ( nrjobswitherr )
	{
	    BufferString ques; ques += nrjobswitherr;
	    ques += " job(s) either failed or finished with error, ";
	    ques += "do you still want to proceed with merging the output?";
	    const int resp = uiMSG().question( ques.buf(), "Merge anyway",
		    		"Re-submit failed jobs", "Abort" );
	    if ( resp == -1 )
		return;

	    if ( !resp )
	    {
		proc_.submitJobs( &dlg );
		timer_->start( 500, false );
		return;
	    }
	}

	mergeOutput( pars_, &dlg, msg );
	label_->setText( msg.buf() );
    }
}


#define mErrRet(s) { msg = s; return false; }
bool uiClusterProc::mergeOutput( const IOPar& pars, TaskRunner* tr,
				 BufferString& msg, bool withdelete )
{
    MultiID key;
    if ( !pars.get(uiClusterJobProv::sKeyOutputID(),key) )
	mErrRet("Missing ID of Temporary storage in the parameters file")
    PtrMan<IOObj> inobj = IOM().get( key );
    if ( !pars.get("Output.ID",key) )
	mErrRet("Missing ID of output dataset in the parameters file")
    PtrMan<IOObj> outobj = IOM().get( key );
    if ( !inobj || !outobj )
	mErrRet("Cannot open Output" ) 
    PtrMan<SeisSingleTraceProc> exec = new SeisSingleTraceProc( inobj, outobj,
	    	"Data transfer", &pars, "Writing results to output cube" );

    if ( !exec )
	return false;

    if ( !TaskRunner::execute( tr, *exec ) )
	mErrRet("Failed to merge output data")
    else
	msg = "Finished merging output data";

    if ( !withdelete )
	return true;
    
    MultiID tempid;
    if ( pars.get("Output.0.Seismic.ID",tempid) )
    {
	IOM().to( SeisTrcTranslatorGroup::ioContext().getSelKey() );
	IOM().permRemove( tempid );
    }

    FixedString tmpdir = pars.find( sKey::TmpStor() );
    if ( tmpdir && File::isDirectory(tmpdir.str()) )
	File::removeDir( tmpdir.str() );

    return true;
}
