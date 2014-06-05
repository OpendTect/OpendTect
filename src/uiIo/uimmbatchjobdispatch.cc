/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          April 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimmbatchjobdispatch.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uicombobox.h"
#include "uislider.h"
#include "uilabel.h"
#include "uiseparator.h"
#include "uibutton.h"
#include "uitextedit.h"
#include "uitextfile.h"
#include "uiprogressbar.h"
#include "uitaskrunner.h"
#include "uistatusbar.h"
#include "uimsg.h"
#include "timer.h"
#include "timefun.h"
#include "oddirs.h"
#include "envvars.h"
#include "hostdata.h"
#include "batchjobdispatch.h"
#include "jobrunner.h"
#include "jobdescprov.h"
#include "jobinfo.h"
#include "od_iostream.h"
#include "genc.h"
#include "ioman.h"
#include "survinfo.h"
#include "plugins.h"



bool uiMMBatchJobDispatcher::initMMProgram( int argc, char** argv,
						IOPar& jobpars )
{
    const FixedString arg1( argv[1] );
    const int bgadd = arg1 == "-bg" ? 1 : 0;
    if ( argc+bgadd < 2 )
    {
	od_cout() << "Usage: " << argv[0] << " parfile" << od_endl;
	return false;
    }

    FilePath fp( argv[ 1 + bgadd ] );
    const BufferString parfnm( fp.fullPath() );
    od_istream strm( parfnm );
    if ( !strm.isOK() )
    {
	od_cout() << argv[0] << ": Cannot open parameter file" << od_endl;
	return false;
    }

    jobpars.read( strm, sKey::Pars() );
    if ( jobpars.isEmpty() )
    {
	od_cout() << argv[0] << ": Invalid parameter file" << parfnm << od_endl;
	return false;
    }
    strm.close();

    if ( bgadd )
	ForkProcess();

    const char* res = jobpars.find( sKey::Survey() );
    if ( res && *res && SI().getDirName() != res )
	IOMan::setSurvey( res );
    PIM().loadAuto( false );
    jobpars.set( sKey::FileName(), parfnm );

    return true;
}



uiMMBatchJobDispatcher::uiMMBatchJobDispatcher( uiParent* p, const IOPar& iop,
						const HelpKey& helpkey )
    : uiDialog(p,uiDialog::Setup("",mNoDlgTitle,helpkey)
		.nrstatusflds(-1)
		.fixedsize(true))
    , jobpars_(*new IOPar(iop))
    , hdl_(*new HostDataList)
    , avmachfld_(0), usedmachfld_(0)
    , nicefld_(0)
    , logvwer_(0)
    , progrfld_(0)
    , progbar_(0)
    , jrpstartfld_(0), jrpstopfld_(0)
    , jobrunner_(0)
    , timer_(0)
    , nrcyclesdone_(0)
    , basecaption_("Job management")
{
    setCaption( basecaption_ );

    const int nrhosts = hdl_.size();
    const bool multihost = nrhosts > 1;
    int maxhostdisp = 1;
    if ( multihost )
	maxhostdisp = nrhosts>7 ? 8 : (nrhosts<3 ? 3 : nrhosts);
    const int hostnmwdth = 30;

    statusBar()->addMsgFld( "Message", Alignment::Left, 20 );
    statusBar()->addMsgFld( "DoneTxt", Alignment::Right, 20 );
    statusBar()->addMsgFld( "NrDone", Alignment::Left, 10 );
    statusBar()->addMsgFld( "Activity", Alignment::Left, 1 );

    specparsgroup_ = new uiGroup( this, "Specific parameters group" );
    uiSeparator* sep = new uiSeparator( this, "Hor sep 1" );
    sep->attach( stretchedBelow, specparsgroup_ );

    uiGroup* machgrp = new uiGroup( this, "Machine handling" );
    uiLabeledListBox* avmachfld = 0;
    if ( multihost )
    {
	avmachfld = new uiLabeledListBox( machgrp, "Available hosts",
					  OD::ChooseAtLeastOne,
					  uiLabeledListBox::AboveMid );
	machgrp->setHAlignObj( avmachfld );
	avmachfld_ = avmachfld->box();
	for ( int idx=0; idx<hdl_.size(); idx++ )
	{
	    const HostData& hd = *hdl_[idx];
	    BufferString nm( hd.getHostName() );
	    const int nraliases = hd.nrAliases();
	    for ( int aliasidx=0; aliasidx<nraliases; aliasidx++ )
		{ nm += " / "; nm += hd.alias(aliasidx); }
	    avmachfld_->addItem( nm );
	}

	avmachfld_->setPrefWidthInChar( mCast(float,hostnmwdth) );
	avmachfld_->setPrefHeightInChar( mCast(float,maxhostdisp) );
    }

    uiGroup* usedmachgrp = new uiGroup( machgrp, "Used machine handling" );
    uiLabeledListBox* usedmachfld = new uiLabeledListBox( usedmachgrp,
				multihost ? "Used hosts" : "",
				OD::ChooseOnlyOne, uiLabeledListBox::AboveMid );
    usedmachfld_ = usedmachfld->box();
    usedmachfld_->setPrefWidthInChar( hostnmwdth );
    usedmachfld_->setPrefHeightInChar( maxhostdisp );

    uiButton* stopbut = new uiPushButton( usedmachgrp, "St&op", true );
    stopbut->activated.notify( mCB(this,uiMMBatchJobDispatcher,stopPush) );
    uiButton* vwlogbut = new uiPushButton( usedmachgrp, "&View log", false );
    vwlogbut->activated.notify( mCB(this,uiMMBatchJobDispatcher,vwLogPush) );
    vwlogbut->attach( rightAlignedBelow, usedmachfld );

    uiButton* addbut;
    if ( multihost )
    {
	stopbut->attach( alignedBelow, usedmachfld );
	addbut = new uiPushButton( machgrp, ">> &Add >>", true );
	if ( avmachfld )
	    addbut->attach( centeredRightOf, avmachfld );
	usedmachgrp->attach( ensureRightOf, addbut );
    }
    else
    {
	addbut = new uiPushButton( usedmachgrp, "St&art", true );
	addbut->attach( alignedBelow, usedmachfld );
	stopbut->attach( centeredBelow, usedmachfld );
	machgrp->setHAlignObj( stopbut );
    }
    addbut->activated.notify( mCB(this,uiMMBatchJobDispatcher,addPush) );

    if ( sep )
	machgrp->attach( ensureBelow, sep );

    uiGroup* jrppolgrp = new uiGroup( this, "Job run policy group" );

    nicefld_ = new uiSlider( jrppolgrp,
		uiSlider::Setup(tr("'Nice' level (0-19)")), "Nice level" );
    nicefld_->setMinValue( -0.5 ); nicefld_->setMaxValue( 19.5 );
    nicefld_->setValue( hdl_.niceLevel() );
    if ( avmachfld_ )
	nicefld_->setPrefWidthInChar( hostnmwdth );

    jrppolselfld_ = new uiComboBox( jrppolgrp, "JobRun policy" );
    jrppolselfld_->addItem( "Run" );
    jrppolselfld_->addItem( "Pause" );
    jrppolselfld_->addItem( "Schedule" );
    jrppolselfld_->setCurrentItem( ((int)0) );
    jrppolselfld_->selectionChanged.notify(
				mCB(this,uiMMBatchJobDispatcher,jrpSel) );
    jrppolselfld_->attach( alignedBelow, nicefld_ );
    if ( avmachfld_ ) jrppolselfld_->setPrefWidthInChar( hostnmwdth );
    jrpworklbl_ = new uiLabel( jrppolgrp, "Processes" );
    jrpworklbl_->attach( rightOf, jrppolselfld_ );

    const char* envstr = GetEnvVar( "DTECT_STOP_OFFICEHOURS" );
    jrpstartfld_ = new uiGenInput( jrppolgrp, "", envstr ? envstr : "18:00" );
    jrpstartfld_->attach( rightOf, jrppolselfld_ );

    envstr = GetEnvVar( "DTECT_START_OFFICEHOURS" );
    jrpstopfld_ = new uiGenInput( jrppolgrp, "and", envstr ? envstr : "7:30" );
    jrpstopfld_->attach( rightOf, jrpstartfld_ );

    jrppolgrp->setHAlignObj( nicefld_ );
    jrppolgrp->attach( ensureBelow, machgrp );

    sep = new uiSeparator( this, "Hor sep 2" );
    sep->attach( stretchedBelow, jrppolgrp );

    progrfld_ = new uiTextEdit( this, "Processing progress", true );
    progrfld_->attach( ensureBelow, sep );
    progrfld_->attach( widthSameAs, sep );
    progrfld_->setPrefHeightInChar( 7 );

    progbar_ = new uiProgressBar( this, "", 1, 0 );
    progbar_->attach( widthSameAs, progrfld_ );
    progbar_->attach( alignedBelow, progrfld_ );

    postFinalise().notify( mCB(this,uiMMBatchJobDispatcher,initWin) );
}


uiMMBatchJobDispatcher::~uiMMBatchJobDispatcher()
{
    delete logvwer_;
    delete timer_;

    delete &jobpars_;
    delete &hdl_;
}


void uiMMBatchJobDispatcher::initWin( CallBacker* cb )
{
    jrpSel( cb );
}


void uiMMBatchJobDispatcher::startWork( CallBacker* )
{
    if ( !initWork(false) || !jobrunner_ )
    {
	if ( !errmsg_.isEmpty() )
	    uiMSG().error( errmsg_ );
	setCancelText( "Dismiss" );
	button(OK)->display( false );
	return;
    }

    jobrunner_->setFirstPort( hdl_.firstPort() );
    jobrunner_->setRshComm( hdl_.loginCmd() );
    jobrunner_->setNiceNess( hdl_.niceLevel() );

    jobrunner_->preJobStart.notify( mCB(this,uiMMBatchJobDispatcher,jobPrep) );
    jobrunner_->postJobStart.notify( mCB(this,uiMMBatchJobDispatcher,jobStart));
    jobrunner_->jobFailed.notify( mCB(this,uiMMBatchJobDispatcher,jobFail) );
    jobrunner_->msgAvail.notify( mCB(this,uiMMBatchJobDispatcher,infoMsgAvail));

    setOkText( "Finish Now" );
    setCancelText( "Abort" );

    timer_ = new Timer("uiMMBatchJobDispatcher timer");
    timer_->tick.notify( mCB(this,uiMMBatchJobDispatcher,doCycle) );
    timer_->start( 100, true );
}


#define mRetFullFail \
{ \
    if ( uiMSG().askGoOn("Do you want to (try to) remove all temporary data?" \
		 "If you don't, you may be able to re-start the job later") ) \
	removeTempResults(); \
    return; \
}


void uiMMBatchJobDispatcher::doCycle( CallBacker* )
{
    nrcyclesdone_++;

    if ( jobrunner_->nextStep() == Executor::ErrorOccurred() )
    {
	delete jobrunner_;
	jobrunner_ = 0;
	return;
    }

    handleJobPausing();
    updateCurMachs();
    updateAliveDisp();

    bool userwantsretry = false;
    const bool needwrapup = ready4WrapUp( userwantsretry );
    if ( needwrapup || userwantsretry )
    {
	if ( needwrapup )
	{
	    if ( wrapUp() )
		return; // Finished.
	    else if ( !recoverFailedWrapUp() )
	    {
		statusBar()->message( "Post-processing failed", 0 );
		clearAliveDisp();
		mRetFullFail
	    }
	}

	else if ( userwantsretry ) // no more jobs, but some failed
	{
	    if ( !initWork(true) )
	    {
		if ( !errmsg_.isEmpty() )
		    uiMSG().error( errmsg_ );
		statusBar()->message( "Error recovery failed", 0 );
		mRetFullFail
	    }
	}

	updateCurMachs();
	updateAliveDisp();
    }


    int niceval = nicefld_->getIntValue();
    if ( niceval > 19 ) niceval = 19; if ( niceval < 0 ) niceval = 0;
    jobrunner_->setNiceNess( niceval );
    timer_->start( 250, true );
}


void uiMMBatchJobDispatcher::stopPush( CallBacker* )
{
    const int rhidx = runnerHostIdx( curUsedMachName() );
    if ( rhidx >= 0 )
	jobrunner_->removeHost( rhidx );
}


void uiMMBatchJobDispatcher::vwLogPush( CallBacker* )
{
    if ( !jobrunner_ ) return;

    BufferString hostnm( curUsedMachName() );
    const HostNFailInfo* hfi = 0;
    const ObjectSet<HostNFailInfo>& hi = jobrunner_->hostInfo();
    for ( int idx=0; idx<hi.size(); idx++ )
    {
	if ( hi[idx]->hostdata_.isKnownAs(hostnm) )
	    { hfi = hi[idx]; break; }
    }
    if ( !hfi ) return;

    JobInfo* ji = jobrunner_->currentJob( hfi );
    FilePath logfp( jobrunner_->getBaseFilePath(*ji, hfi->hostdata_) );
    logfp.setExtension( ".log", false );

    delete logvwer_;
    const BufferString fnm( logfp.fullPath() );
    logvwer_ = new uiTextFileDlg( this, uiTextFile::Setup(File::Log),
				  uiTextFileDlg::Setup(fnm), fnm );
    logvwer_->go();
}


void uiMMBatchJobDispatcher::jrpSel( CallBacker* )
{
    const bool isgo = *jrppolselfld_->text() == 'G';
    jrpstartfld_->display( isgo );
    jrpstopfld_->display( isgo );
    jrpworklbl_->display( !isgo );
}


static void addObjNm( BufferString& msg, const JobRunner* jr, int nr )
{
    msg += jr->descProv()->objType(); msg += " ";
    msg += jr->descProv()->objName( nr );
}


void uiMMBatchJobDispatcher::jobPrep( CallBacker* )
{
    prepareCurrentJob();

    /* CDash doesn't like this:
    if ( !prepareCurrentJob() )
	// TODO put errmsg_ somewhere
	;
    */
}


void uiMMBatchJobDispatcher::jobStart( CallBacker* )
{
    const JobInfo& ji = jobrunner_->curJobInfo();
    BufferString msg( "Started processing " );
    addObjNm( msg, jobrunner_, ji.descnr_ );
    if ( ji.hostdata_ )
	{ msg += " on "; msg += ji.hostdata_->getHostName(); }
    progrfld_->append( msg );
}


void uiMMBatchJobDispatcher::jobFail( CallBacker* )
{
    const JobInfo& ji = jobrunner_->curJobInfo();
    BufferString msg( "Failure for " );
    addObjNm( msg, jobrunner_, ji.descnr_ );
    if ( ji.hostdata_ )
	{ msg += " on "; msg += ji.hostdata_->getHostName(); }
    if ( !ji.infomsg_.isEmpty() )
	{ msg += ": "; msg += ji.infomsg_; }
    progrfld_->append( msg );
}


void uiMMBatchJobDispatcher::infoMsgAvail( CallBacker* )
{
    const JobInfo& ji = jobrunner_->curJobInfo();
    if ( ji.infomsg_.isEmpty() ) { pErrMsg("huh?"); return; }

    BufferString msg( "Info for " );
    addObjNm( msg, jobrunner_, ji.descnr_ );
    if ( ji.hostdata_ )
	{ msg += " on "; msg += ji.hostdata_->getHostName(); }

    msg += ": "; msg += ji.infomsg_;
    progrfld_->append( msg );
}


void uiMMBatchJobDispatcher::updateCurMachs()
{
    BufferStringSet machs;
    jobrunner_->showMachStatus( machs );
    sort( machs );

    const int oldsz = usedmachfld_->size();
    const int newsz = machs.size();

    int curit = oldsz > 0 ? usedmachfld_->currentItem() : -1;
    usedmachfld_->setEmpty();
    if ( newsz > 0 )
    {
	usedmachfld_->addItems( machs );
	if ( curit >= usedmachfld_->size() )
	    curit = usedmachfld_->size() - 1;
	usedmachfld_->setCurrentItem(curit);
    }
}



void uiMMBatchJobDispatcher::updateAliveDisp()
{
    const int nrdispstrs = 6;
    const char* dispstrs[]
	= { ">..", ".>.", "..>", "..<", ".<.", "<.." };
    statusBar()->message( dispstrs[ nrcyclesdone_ % nrdispstrs ], 3 );

    const int totsteps = mCast( int, jobrunner_->totalNr() );
    const int nrdone = mCast( int, jobrunner_->nrDone() );
    const bool hastot = totsteps > 0;
    progbar_->display( hastot );
    if ( hastot )
    {
	progbar_->setTotalSteps( totsteps );
	progbar_->setProgress( nrdone );

	const float fpct = 100.f * ((float)nrdone) / totsteps;
	int pct = (int)fpct; if ( pct > 100 ) pct = 100;
	BufferString newcap( "[" ); newcap += pct; newcap += "%] ";
	newcap += basecaption_;
	setCaption( newcap );
    }
}


void uiMMBatchJobDispatcher::clearAliveDisp()
{
    statusBar()->message( "", 3 );
}


bool uiMMBatchJobDispatcher::ready4WrapUp( bool& havefails ) const
{
    if ( jobrunner_->jobsLeft() > 0 )
	return false;

    const int nrfailed = jobrunner_->nrJobs( true );
    if ( nrfailed < 1 )
	return true;

    havefails = true;
    BufferString msg( "Failed ", jobrunner_->descProv()->objType(),
		      nrfailed > 1 ? "s:\n" : ": " );
    BufferString newpart;
    bool needspace = false;
    for ( int idx=0; idx<nrfailed; idx++ )
    {
	const JobInfo& ji = jobrunner_->jobInfo( idx, true );
	if ( needspace )
	    newpart.addSpace();
	newpart += jobrunner_->descProv()->objName( ji.descnr_ );
	if ( newpart.size() < 70 )
	    needspace = true;
	else
	{
	    msg.add( newpart );
	    newpart.addNewLine(); needspace = false;
	}
    }
    msg.add( newpart ).add( "\n\nDo you want to re-try?" );
    return !uiMSG().askGoOn(msg);
}



bool uiMMBatchJobDispatcher::wrapUp()
{
    Executor* exec = getPostProcessor();
    if ( exec )
    {
	uiTaskRunner uitr( this );
	const bool res = uitr.execute( *exec );
	delete exec;
	if ( !res )
	    return false;
    }

    progrfld_->append( "Processing completed" );
    setCtrlStyle( CloseOnly );
    button(uiDialog::CANCEL)->display(false);
    clearAliveDisp();

    removeTempResults();
    delete jobrunner_; jobrunner_ = 0;
    return true;
}


void uiMMBatchJobDispatcher::removeTempResults()
{
    errmsg_.setEmpty();
    if ( !removeTmpProcFiles() && !errmsg_.isEmpty() )
	uiMSG().warning( errmsg_ );
}


static int getSecs( const char* txt )
{
    if ( !txt || !*txt ) return 0;
    BufferString bs( txt );
    char* mid = bs.find( ':' );
    if ( mid ) *mid++ = '\0';

    int secs=-1;
    if ( mid && *mid )
    {
	char* head = mid-1;
	while ( *head != ' ' && head > bs.buf() ) head--;

	char* tail = firstOcc( mid, ':' );
	if ( tail ) *tail++ = '\0';

	secs = toInt( head ) * 3600;
	secs += toInt( mid ) * 60;

	if( tail )
	    secs += toInt( tail );
    }
    return secs;
}


bool uiMMBatchJobDispatcher::isPaused() const
{
    const char* txt = jrppolselfld_->text();
    bool dopause = *txt == 'P';
    if ( *txt == 'G' )
    {
	const int t = getSecs( Time::getDateTimeString() );
	const int t0 = getSecs( jrpstartfld_->text() );
	const int t1 = getSecs( jrpstopfld_->text() );

	bool run = t1 >= t0 ? t >= t0 && t <= t1
			    : t >= t0 || t <= t1;
        dopause = !run;
    }
    return dopause;
}


void uiMMBatchJobDispatcher::handleJobPausing()
{
    if ( !jobrunner_ )
	return;

    const bool ispaused = isPaused();

    const int nrhosts = jobrunner_->hostInfo().size();
    for ( int idx=0; idx<nrhosts; idx++ )
	jobrunner_->pauseHost( idx, ispaused );
}



#ifdef __win__
#define mAddReDirectToNull checkcmd += " > NUL"
#else
#define mAddReDirectToNull checkcmd += " > /dev/null"
#endif

#ifndef __win__

static bool hostOK( const HostData& hd, const char* rshcomm,
		      BufferString& errmsg )
{
    BufferString remotecmd( rshcomm );
    remotecmd += " "; remotecmd += hd.getHostName();
    BufferString checkcmd( remotecmd ); checkcmd += " whoami";
    mAddReDirectToNull;
    if ( system(checkcmd.buf()) )
    {
	errmsg = "Cannot establish a ";
	errmsg += rshcomm; errmsg += " connection with ";
	errmsg += hd.getHostName();
	return false;
    }

    checkcmd = remotecmd; checkcmd += " cd ";
    checkcmd += hd.convPath( HostData::Appl, GetSoftwareDir(0) ).fullPath();
    if ( system(checkcmd.buf()) )
    {
	errmsg = "Cannot find application directory ";
	errmsg += hd.getHostName(); errmsg += ":";
	errmsg += hd.convPath(HostData::Appl, GetSoftwareDir(0)).fullPath();
	errmsg += "\nMake sure the filesystem is mounted on remote host ";
	return false;
    }

    checkcmd = remotecmd; checkcmd += " cd ";
    checkcmd += hd.convPath( HostData::Data, GetBaseDataDir() ).fullPath();
    if ( system(checkcmd.buf()) )
    {
	errmsg = "Cannot find data directory ";
	errmsg += hd.getHostName(); errmsg += ":";
	errmsg += hd.convPath(HostData::Data, GetBaseDataDir()).fullPath();
	errmsg += "\nMake sure the filesystem is mounted on remote host";
	return false;
    }

    return true;
}

#endif


#define mErrRet(s) { uiMSG().error(s); return; }

void uiMMBatchJobDispatcher::addPush( CallBacker* )
{
    const int nrmach = avmachfld_ ? avmachfld_->size() : 1;
    if ( nrmach < 1 )
	return;

    const int nrsel = avmachfld_ ? avmachfld_->nrChosen() : 1;
    if ( nrsel < 1 )
	mErrRet("Please select one or more hosts")

    if ( !jobrunner_ )
    {
	startWork( 0 );
	if ( !jobrunner_ )
	    return;
    }

    for ( int idx=0; idx<nrmach; idx++ )
    {
	if ( avmachfld_ && !avmachfld_->isChosen(idx) ) continue;

	BufferString hnm = avmachfld_ ? avmachfld_->textOfItem( idx )
				      : hdl_[0]->getHostName();
	char* ptr = hnm.find( ' ' );
	if ( ptr )
	    *ptr = '\0';

	const HostData* hd = hdl_.find( hnm.buf() );
	if ( !hd )
	    { pErrMsg("Huh"); continue; }

#ifndef __win__
	BufferString errmsg;
	if ( !hd->isKnownAs(HostData::localHostName())
		&& !hostOK(*hd,hdl_.loginCmd(),errmsg) )
	    { progrfld_->append( errmsg.buf() ); continue; }
#endif

	if ( !jobrunner_->addHost(*hd) && jobrunner_->jobsLeft() > 0 )
	{
	    BufferString msg = "Could not start job";
	    if ( isMultiHost() )
		msg.add( " on " ).add( hnm );
	    if ( jobrunner_->errorMsg() && *jobrunner_->errorMsg() )
		msg.add( " : " ).add( jobrunner_->errorMsg() );
	    progrfld_->append( msg );
	}
    }
}


const char* uiMMBatchJobDispatcher::curUsedMachName()
{
   mDeclStaticString( mach );
   mach = usedmachfld_->getText();

   char* ptr = mach.find( " -:- ");
   if ( ptr ) *ptr='\0';

   return mach;
}



int uiMMBatchJobDispatcher::runnerHostIdx( const char* mach ) const
{
    if ( !jobrunner_ || !mach || !*mach ) return -1;

    const ObjectSet<HostNFailInfo>& hi = jobrunner_->hostInfo();
    for ( int idx=0; idx<hi.size(); idx++ )
    {
	if ( hi[idx]->hostdata_.isKnownAs(mach) )
	    return idx;
    }
    return -1;
}



bool uiMMBatchJobDispatcher::rejectOK( CallBacker* )
{
    if ( !jobrunner_ )
	return true;

    int res = 1;
    if ( needConfirmEarlyStop() )
    {
	if ( jobrunner_->jobsDone() > 0 || jobrunner_->jobsInProgress() > 0 )
	{
	    BufferString msg = "This will stop all processing!";
	    if ( !haveTmpProcFiles() )
	    {
		msg += "\n\nDo you want to do this?";
		res = uiMSG().askGoOn( msg ) ? 0 : -1;
	    }
	    else
	    {
		msg += "\n\nDo you want to remove already processed data?";
		res = uiMSG().askRemove( msg, true );
	    }
	}
	if ( res == -1 )
	    return false;
    }

    jobrunner_->stopAll();
    if ( res == 1 )
	removeTempResults();

    return true;
}


bool uiMMBatchJobDispatcher::acceptOK(CallBacker*)
{
    if ( needConfirmEarlyStop() )
    {
	if ( !usedmachfld_->isEmpty() && !uiMSG().askGoOn(
		    "This will stop further processing and wrap up",false) )
	    return false;
    }

    if ( jobrunner_ )
	jobrunner_->stopAll();
    return wrapUp();
}
