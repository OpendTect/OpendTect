/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          April 2002
________________________________________________________________________

-*/

#include "uibatchjobdispatcherlauncher.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uicombobox.h"
#include "uislider.h"
#include "uilabel.h"
#include "uimmbatchjobdispatch.h"
#include "uiseparator.h"
#include "uibutton.h"
#include "uitextedit.h"
#include "uitextfile.h"
#include "uiprogressbar.h"
#include "uitaskrunner.h"
#include "uistatusbar.h"
#include "uimsg.h"
#include "staticstring.h"
#include "timer.h"
#include "timefun.h"
#include "oddirs.h"
#include "envvars.h"
#include "hostdata.h"
#include "keystrs.h"
#include "batchjobdispatch.h"
#include "jobrunner.h"
#include "jobdescprov.h"
#include "jobinfo.h"
#include "od_iostream.h"
#include "genc.h"
#include "dbman.h"
#include "survinfo.h"
#include "plugins.h"


#define mLogMsg(s) if ( logstrm_ ) *logstrm_ << s << od_endl;
bool uiMMBatchJobDispatcher::initMMProgram( int argc, char** argv,
						IOPar& jobpars )
{
    const FixedString arg1( argv[1] );
    if ( argc < 2 )
    {
	od_cout() << "Usage: " << argv[0] << " parfile" << od_endl;
	return false;
    }

    File::Path fp( argv[1] );
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
	od_cout() << argv[0] << ": Invalid parameter file"
                  << parfnm << od_endl;
	return false;
    }
    strm.close();

    DBM().setDataSource( jobpars );
    PIM().loadAuto( false );
    jobpars.set( sKey::FileName(), parfnm );

    return true;
}



uiMMBatchJobDispatcher::uiMMBatchJobDispatcher( uiParent* p, const IOPar& iop,
						const HelpKey& helpkey )
    : uiDialog(p,uiDialog::Setup(uiString::empty(),mNoDlgTitle,helpkey)
		.nrstatusflds(-1)
		.fixedsize(true))
    , jobpars_(*new IOPar(iop))
    , hdl_(*new const HostDataList(false))
    , avmachfld_(0), usedmachfld_(0)
    , priofld_(0)
    , logvwer_(0)
    , logstrm_(0)
    , progrfld_(0)
    , progbar_(0)
    , jrpstartfld_(0), jrpstopfld_(0)
    , jobrunner_(0)
    , timer_(0)
    , nrcyclesdone_(0)
    , basecaption_(tr("Distributed Processing"))
{
    setCaption( basecaption_ );

    const int nrhosts = hdl_.size();
    const bool multihost = nrhosts > 1;
    int maxhostdisp = 1;
    if ( multihost )
	maxhostdisp = nrhosts>7 ? 8 : (nrhosts<3 ? 3 : nrhosts);
    const int hostnmwdth = 30;

    statusBar()->addMsgFld( toUiString("Message"), OD::Alignment::Left, 20 );
    statusBar()->addMsgFld( toUiString("DoneTxt"), OD::Alignment::Right, 20 );
    statusBar()->addMsgFld( toUiString("NrDone"), OD::Alignment::Left, 10 );
    statusBar()->addMsgFld( toUiString("Activity"), OD::Alignment::Left, 1 );

    specparsgroup_ = new uiGroup( this, "Specific parameters group" );
    uiSeparator* sep = new uiSeparator( this, "Hor sep 1" );
    sep->attach( stretchedBelow, specparsgroup_ );

    uiGroup* machgrp = new uiGroup( this, "Machine handling" );
    if ( multihost )
    {
	uiListBox::Setup su( OD::ChooseAtLeastOne, tr("Available hosts"),
			     uiListBox::AboveMid );
	avmachfld_ = new uiListBox( machgrp, su );
	machgrp->setHAlignObj( avmachfld_ );
	for ( int idx=0; idx<hdl_.size(); idx++ )
	{
	    const HostData& hd = *hdl_[idx];
	    BufferString nm( hd.getHostName() );
	    const int nraliases = hd.nrAliases();
	    for ( int aliasidx=0; aliasidx<nraliases; aliasidx++ )
		{ nm += " / "; nm += hd.alias(aliasidx); }
	    avmachfld_->addItem( toUiString(nm) );
	}

	avmachfld_->setPrefWidthInChar( hostnmwdth );
	avmachfld_->setPrefHeightInChar( maxhostdisp );
    }

    uiGroup* usedmachgrp = new uiGroup( machgrp, "Used machine handling" );
    uiListBox::Setup su( OD::ChooseOnlyOne,
		multihost ? tr("Used hosts") : uiString::empty(),
		uiListBox::AboveMid );
    usedmachfld_ = new uiListBox( usedmachgrp, su );
    usedmachfld_->setPrefWidthInChar( hostnmwdth );
    usedmachfld_->setPrefHeightInChar( maxhostdisp );

    uiButton* stopbut = new uiPushButton( usedmachgrp,
					  uiStrings::sStop(), true );
    stopbut->activated.notify( mCB(this,uiMMBatchJobDispatcher,stopPush) );
    uiButton* vwlogbut = new uiPushButton( usedmachgrp,
					   tr("View Log"), false );
    vwlogbut->activated.notify( mCB(this,uiMMBatchJobDispatcher,vwLogPush) );
    vwlogbut->attach( rightAlignedBelow, usedmachfld_ );

    uiButton* addbut;
    if ( multihost )
    {
	stopbut->attach( alignedBelow, usedmachfld_ );
	addbut = new uiPushButton( machgrp,
		    toUiString( ">> %1 >>" ).arg( uiStrings::sAdd() ), true );
	if ( avmachfld_ )
	    addbut->attach( centeredRightOf, avmachfld_ );
	usedmachgrp->attach( ensureRightOf, addbut );
    }
    else
    {
	addbut = new uiPushButton( usedmachgrp, uiStrings::sStart(), true );
	addbut->attach( alignedBelow, usedmachfld_ );
	stopbut->attach( centeredBelow, usedmachfld_ );
	machgrp->setHAlignObj( stopbut );
    }
    addbut->activated.notify( mCB(this,uiMMBatchJobDispatcher,addPush) );

    if ( sep )
	machgrp->attach( ensureBelow, sep );

    uiGroup* jrppolgrp = new uiGroup( this, "Job run policy group" );

    bool hasunixhost = false;
    for ( int ihost=0; ihost<hdl_.size(); ihost++ )
    {
	if ( !hdl_[ihost]->isWindows() )
	    { hasunixhost = true; break; }
    }

    const StepInterval<int> unixmachpriorg(
		    OS::CommandExecPars::cMachineUserPriorityRange(false) );
    const StepInterval<int> winmachpriorg(
		    OS::CommandExecPars::cMachineUserPriorityRange(true) );
    const int nrsteps = hasunixhost ? unixmachpriorg.nrSteps()
				    : winmachpriorg.nrSteps();

    uiSlider::Setup ssu( tr("Job Priority") );
    ssu.nrdec( 7 );
    priofld_ = new uiSlider( jrppolgrp, ssu );
    priofld_->setInterval( -1.f, 0.f, 1.f/nrsteps );
    priofld_->setValue( hdl_.priorityLevel() );
    uiLabel* sliderlbl = new uiLabel( jrppolgrp,
				      tr("Left:Low, Right: Normal") );
    sliderlbl->attach( rightOf, priofld_ );

    jrppolselfld_ = new uiComboBox( jrppolgrp, "JobRun policy" );
    jrppolselfld_->addItem( uiStrings::sRun() );
    jrppolselfld_->addItem( uiStrings::sPause() );
    jrppolselfld_->addItem( uiStrings::sSchedule() );
    jrppolselfld_->setCurrentItem( ((int)0) );
    jrppolselfld_->selectionChanged.notify(
				mCB(this,uiMMBatchJobDispatcher,jrpSel) );
    jrppolselfld_->attach( alignedBelow, priofld_ );

    if ( avmachfld_ ) jrppolselfld_->setPrefWidthInChar( hostnmwdth );
    jrpworklbl_ = new uiLabel( jrppolgrp, uiStrings::sProcess(mPlural) );
    jrpworklbl_->attach( rightOf, jrppolselfld_ );

    const char* envstr = GetEnvVar( "DTECT_STOP_OFFICEHOURS" );

    jrpstartfld_ = new uiGenInput( jrppolgrp, uiString::empty(),
				   envstr ? envstr : "18:00" );

    jrpstartfld_->attach( rightOf, jrpworklbl_ );

    envstr = GetEnvVar( "DTECT_START_OFFICEHOURS" );
    jrpstopfld_ = new uiGenInput( jrppolgrp, uiStrings::sAnd().toLower(),
				  envstr ? envstr : "7:30" );
    jrpstopfld_->attach( rightOf, jrpstartfld_ );

    jrppolgrp->setHAlignObj( priofld_ );
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
    setCaption( basecaption_ );
}


void uiMMBatchJobDispatcher::enableJobControl( bool yn )
{
    jrppolselfld_->display( yn );
    jrpworklbl_->display( yn );
    if ( yn )
	jrpSel( 0 );
    else
    {
	jrpstartfld_->display( false );
	jrpstopfld_->display( false );
    }
}


void uiMMBatchJobDispatcher::startWork( CallBacker* )
{
    if ( !initWork(false) || !jobrunner_ )
    {
	if ( !errmsg_.isEmpty() )
	    uiMSG().error( errmsg_ );
	setCancelText( uiStrings::sClose() );
	button(OK)->display( false );
	return;
    }

    jobrunner_->setFirstPort( hdl_.firstPort() );
    jobrunner_->setRshComm( hdl_.loginCmd() );
    jobrunner_->setPriority( hdl_.priorityLevel() );

    jobrunner_->preJobStart.notify( mCB(this,uiMMBatchJobDispatcher,jobPrep) );
    jobrunner_->postJobStart.notify( mCB(this,uiMMBatchJobDispatcher,jobStart));
    jobrunner_->jobFailed.notify( mCB(this,uiMMBatchJobDispatcher,jobFail) );
    jobrunner_->msgAvail.notify( mCB(this,uiMMBatchJobDispatcher,infoMsgAvail));

    setOkText( tr("Finish Now") );
    setCancelText( uiStrings::sAbort() );

    timer_ = new Timer("uiMMBatchJobDispatcher timer");
    timer_->tick.notify( mCB(this,uiMMBatchJobDispatcher,doCycle) );
    timer_->start( 100, true );
}


#define mRetFullFail \
{ \
    if ( retFullFailGoOnMsg() ) \
	removeTempResults(); \
    return; \
}


bool uiMMBatchJobDispatcher::retFullFailGoOnMsg()
{
    return uiMSG().askGoOn(tr("Do you want to (try to) remove all "
			    "temporary data?\n\nIf you don't, you may be able "
			    "to re-start the job later"));
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
		statusBar()->message( tr("Post-processing failed"), 0 );
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
		statusBar()->message( tr("Error recovery failed"), 0 );
		mRetFullFail
	    }
	}

	updateCurMachs();
	updateAliveDisp();
    }

    jobrunner_->setPriority( priofld_->getFValue() );

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
    File::Path logfp( jobrunner_->getBaseFilePath(*ji, hfi->hostdata_) );
    logfp.setExtension( ".log", false );

    delete logvwer_;
    const BufferString fnm( logfp.fullPath() );
    logvwer_ = new uiTextFileDlg( this, uiTextFile::Setup(File::Log),
			      uiTextFileDlg::Setup(toUiString(fnm)), fnm );
    logvwer_->go();
}


void uiMMBatchJobDispatcher::jrpSel( CallBacker* )
{
    if ( logstrm_ )
    {
	jrppolselfld_->display( false );
	jrpstartfld_->display( false );
	jrpstopfld_->display( false );
	jrpworklbl_->display( false );
	return;
    }

    const bool doschedule = jrppolselfld_->currentItem() == 2;
    jrpstartfld_->display( doschedule );
    jrpstopfld_->display( doschedule );
    jrpworklbl_->setText( doschedule ? tr("Between")
				     : uiStrings::sProcess(mPlural) );
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
    mLogMsg( msg );
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
    mLogMsg( msg );
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
    mLogMsg( msg );
}


void uiMMBatchJobDispatcher::updateCurMachs()
{
    if ( !jobrunner_ )
	return;

    BufferStringSet machs;
    jobrunner_->showMachStatus( machs );
    machs.sort();

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
    statusBar()->message(toUiString(dispstrs[ nrcyclesdone_ % nrdispstrs ]), 3);

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
	uiString newcap = toUiString("[%1%] %2").arg(pct).arg(basecaption_);
	setCaption( newcap );
    }
}


void uiMMBatchJobDispatcher::clearAliveDisp()
{
    statusBar()->message( uiString::empty(), 3 );
}


bool uiMMBatchJobDispatcher::ready4WrapUp( bool& havefails ) const
{
    if ( jobrunner_->jobsLeft() > 0 )
	return false;

    const int nrfailed = jobrunner_->nrJobs( true );
    if ( nrfailed < 1 )
	return true;

    havefails = true;
    uiString msg = tr("Failed %1%2").arg(jobrunner_->descProv()->objType()).
	arg(nrfailed > 1 ? toUiString(":\n") : toUiString(": "));
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
	    msg = toUiString("%1 %2").arg(msg).arg(toUiString(newpart));
	    newpart.addNewLine(); needspace = false;
	}
    }
    msg= toUiString("%1 %2 %3").arg(msg).arg(toUiString(newpart)).
			arg(tr("\n\nDo you want to re-try?"));
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

    BufferString msg( logstrm_ ? "Finished diagnostics"
				: "Processing completed" );
    progrfld_->append( msg );
    mLogMsg( msg );
    setCtrlStyle( CloseOnly );
    uiButton* cancbuttn = button( uiDialog::CANCEL );
    if ( cancbuttn )
	cancbuttn->display(false);
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
    const int jrpol = jrppolselfld_->currentItem();
    bool dopause = jrpol == 1;
    if ( jrpol == 2 )
    {
	const int t = getSecs( Time::getUsrDateTimeString() );
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


static bool hostOK( const HostData& hd, const char* rshcomm,
		    BufferString& errmsg )
{
    OS::MachineCommand remotecmd;
    remotecmd.setHostName( hd.getHostName() );
    remotecmd.setRemExec( rshcomm );
    remotecmd.setHostIsWindows( hd.isWindows() );
    BufferString stdoutstr;

    OS::MachineCommand checkcmd = OS::MachineCommand( remotecmd );
    checkcmd.setProgram( "whoami" );
    if ( !checkcmd.execute(stdoutstr) || stdoutstr.isEmpty() )
    {
	errmsg = "Cannot establish a ";
	errmsg += rshcomm; errmsg += " connection with ";
	errmsg += hd.getHostName();
	return false;
    }

    checkcmd = OS::MachineCommand( remotecmd );
    checkcmd.setProgram( "ls" ).addArg( "-l" )
	    .addArg( hd.convPath(HostData::Appl,GetSoftwareDir(0)).fullPath() );
    stdoutstr.setEmpty();
    if ( !checkcmd.execute(stdoutstr) || stdoutstr.isEmpty() )
    {
	errmsg = "Cannot find application directory ";
	errmsg += hd.getHostName(); errmsg += ":";
	errmsg += hd.convPath(HostData::Appl, GetSoftwareDir(0)).fullPath();
	errmsg += "\nMake sure the filesystem is mounted on remote host ";
	return false;
    }

    checkcmd = OS::MachineCommand( remotecmd );
    checkcmd.setProgram( "ls" ).addArg( "-l" )
	.addArg( hd.convPath(HostData::Data,GetBaseDataDir()).fullPath() );
    stdoutstr.setEmpty();
    if ( !checkcmd.execute(stdoutstr) || stdoutstr.isEmpty() )
    {
	errmsg = "Cannot find data directory ";
	errmsg += hd.getHostName(); errmsg += ":";
	errmsg += hd.convPath(HostData::Data, GetBaseDataDir()).fullPath();
	errmsg += "\nMake sure the filesystem is mounted on remote host";
	return false;
    }

    return true;
}


int uiMMBatchJobDispatcher::nrSelMachs() const
{
    return avmachfld_ ? avmachfld_->nrChosen() : 1;
}

#define mErrRet(s) { uiMSG().error(s); return; }

void uiMMBatchJobDispatcher::addPush( CallBacker* cb )
{
    const int nrmach = avmachfld_ ? avmachfld_->size() : 1;
    if ( nrmach < 1 )
	return;

    const int nrsel = nrSelMachs();
    if ( nrsel < 1 )
	mErrRet(uiStrings::phrSelect(tr("one or more hosts")))

    if ( !jobrunner_ )
    {
	startWork( 0 );
	if ( !jobrunner_ )
	    return;
    }

    for ( int idx=0; idx<nrmach; idx++ )
    {
	if ( avmachfld_ && !avmachfld_->isChosen(idx) ) continue;

	BufferString hnm = avmachfld_ ? avmachfld_->itemText( idx )
				      : hdl_[0]->getHostName();
	char* ptr = hnm.find( ' ' );
	if ( ptr )
	    *ptr = '\0';

	const HostData* hd = hdl_.find( hnm.buf() );
	if ( !hd )
	    { pErrMsg("Huh"); continue; }

	if ( !__iswin__ && !hd->isWindows() )
	{
	    BufferString errmsg;
	    if ( !hd->isKnownAs(BufferString(GetLocalHostName()))
		    && !hostOK(*hd,hdl_.loginCmd(),errmsg) )
	    { progrfld_->append( errmsg.buf() ); mLogMsg(errmsg); continue; }
	}

	if ( !jobrunner_->addHost(*hd) && jobrunner_->jobsLeft() > 0 )
	{
	    uiString msg = uiStrings::phrCannotStart(tr("job on %1").arg(hnm));
	    if ( !jobrunner_->errorMsg().isEmpty() )
		msg.appendPlainText( ":" )
				.appendPhrase( jobrunner_->errorMsg() );
	    progrfld_->append( toString(msg) );
	    mLogMsg( toString(msg) );
	}
    }

    if ( logstrm_ )
    {
	mDynamicCastGet(uiPushButton*,pb,cb)
	if ( pb )
	    pb->setSensitive( false );
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



bool uiMMBatchJobDispatcher::rejectOK()
{
    if ( !jobrunner_ )
	return true;

    int res = 1;
    if ( needConfirmEarlyStop() )
    {
	if ( jobrunner_->jobsDone() > 0 || jobrunner_->jobsInProgress() > 0 )
	{
	    uiString msg = tr("This will stop all processing!%1");
	    if ( !haveTmpProcFiles() )
	    {
		msg.arg("\n\nDo you want to do this?");
		res = uiMSG().askGoOn( msg ) ? 0 : -1;
	    }
	    else
	    {
		msg.arg("\n\nDo you want to remove already processed data?");
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


bool uiMMBatchJobDispatcher::acceptOK()
{
    if ( needConfirmEarlyStop() )
    {
	if ( !usedmachfld_->isEmpty() && !uiMSG().askGoOn(
		    tr("This will stop further processing and wrap up"),false) )
	    return false;
    }

    if ( jobrunner_ )
	jobrunner_->stopAll();
    return wrapUp();
}

