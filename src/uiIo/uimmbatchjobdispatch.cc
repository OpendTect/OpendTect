/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimmbatchjobdispatch.h"

#include "envvars.h"
#include "genc.h"
#include "hostdata.h"
#include "ioman.h"
#include "jobdescprov.h"
#include "jobinfo.h"
#include "jobrunner.h"
#include "mousecursor.h"
#include "netsocket.h"
#include "od_iostream.h"
#include "oddirs.h"
#include "oscommand.h"
#include "plugins.h"
#include "survinfo.h"
#include "systeminfo.h"
#include "timefun.h"
#include "timer.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiprogressbar.h"
#include "uiseparator.h"
#include "uislider.h"
#include "uistatusbar.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "uitextfile.h"



bool uiMMBatchJobDispatcher::initMMProgram( int argc, char** argv,
						IOPar& jobpars )
{
    const StringView arg1( argv[1] );
    if ( argc < 2 )
    {
	od_cout() << "Usage: " << argv[0] << " parfile" << od_endl;
	return false;
    }

    FilePath fp( argv[1] );
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

    BufferString res = jobpars.find( sKey::DataRoot() );
    if ( !res.isEmpty() && SI().getDataDirName() != res )
	SetEnvVar( __iswin__ ? "DTECT_WINDATA" : "DTECT_DATA", res );

    res = jobpars.find( sKey::Survey() );
    if ( !res.isEmpty() && SI().getDirName() != res && !IOMan::setSurvey(res) )
	return false;

    PIM().loadAuto( false );
    jobpars.set( sKey::FileName(), parfnm );

    return true;
}



uiMMBatchJobDispatcher::uiMMBatchJobDispatcher( uiParent* p, const IOPar& iop,
						const HelpKey& helpkey )
    : uiDialog(p,uiDialog::Setup(uiStrings::sEmptyString(),mNoDlgTitle,helpkey)
		.nrstatusflds(-1)
		.fixedsize(true))
    , jobpars_(*new IOPar(iop))
    , hdl_(*new const HostDataList(false))
    , basecaption_(tr("Distributed Computing"))
{
    setCaption( basecaption_ );

    const int nrhosts = hdl_.size();
    const bool multihost = nrhosts > 1;
    int maxhostdisp = 1;
    if ( multihost )
	maxhostdisp = nrhosts>7 ? 8 : (nrhosts<3 ? 3 : nrhosts);
    const int hostnmwdth = 30;

    statusBar()->addMsgFld( toUiString("Message"), Alignment::Left, 20 );
    statusBar()->addMsgFld( toUiString("DoneTxt"), Alignment::Right, 20 );
    statusBar()->addMsgFld( toUiString("NrDone"), Alignment::Left, 10 );
    statusBar()->addMsgFld( toUiString("Activity"), Alignment::Left, 1 );

    specparsgroup_ = new uiGroup( this, "Specific parameters group" );
    auto* sep = new uiSeparator( this, "Hor sep 1" );
    sep->attach( stretchedBelow, specparsgroup_ );

    auto* machgrp = new uiGroup( this, "Machine handling" );
    if ( multihost )
    {
	uiListBox::Setup su( OD::ChooseAtLeastOne, tr("Available hosts"),
			     uiListBox::AboveMid );
	avmachfld_ = new uiListBox( machgrp, su );
	machgrp->setHAlignObj( avmachfld_ );
	for ( const auto* hd : hdl_ )
	    avmachfld_->addItem( hd->getFullDispString() );

	avmachfld_->setCurrentItem( 0 );
	avmachfld_->setPrefWidthInChar( hostnmwdth );
	avmachfld_->setPrefHeightInChar( maxhostdisp );
    }

    auto* usedmachgrp = new uiGroup( machgrp, "Used machine handling" );
    uiListBox::Setup su( OD::ChooseOnlyOne,
		multihost ? tr("Used hosts") : uiString::emptyString(),
		uiListBox::AboveMid );
    su.prefnrlines( isMultiHost() ? maxhostdisp : 1 );
    usedmachfld_ = new uiListBox( usedmachgrp, su );
    usedmachfld_->setPrefWidthInChar( hostnmwdth );

    auto* stopbut = new uiPushButton( usedmachgrp, uiStrings::sStop(), true );
    mAttachCB( stopbut->activated, uiMMBatchJobDispatcher::stopPush );
    auto* vwlogbut = new uiPushButton( usedmachgrp, tr("View Log"), false );
    mAttachCB( vwlogbut->activated, uiMMBatchJobDispatcher::vwLogPush );
    vwlogbut->attach( rightAlignedBelow, usedmachfld_ );

    if ( multihost )
    {
	stopbut->attach( alignedBelow, usedmachfld_ );

	auto* stopallbut = new uiPushButton( usedmachgrp, tr("Stop all"), true);
	stopallbut->attach( rightTo, stopbut );
	mAttachCB( stopallbut->activated, uiMMBatchJobDispatcher::stopAllPush );

	addbut_ = new uiPushButton( machgrp, tr( ">> Add >>" ), true );
	if ( avmachfld_ )
	    addbut_->attach( centeredRightOf, avmachfld_ );
	usedmachgrp->attach( ensureRightOf, addbut_ );
    }
    else
    {
	addbut_ = new uiPushButton( usedmachgrp, tr("Start"), true );
	addbut_->attach( alignedBelow, usedmachfld_ );
	stopbut->attach( centeredBelow, usedmachfld_ );
	machgrp->setHAlignObj( stopbut );
    }

    mAttachCB( addbut_->activated, uiMMBatchJobDispatcher::addPush );

    if ( sep )
	machgrp->attach( ensureBelow, sep );

    auto* jrppolgrp = new uiGroup( this, "Job run policy group" );

    nicefld_ = new uiSlider( jrppolgrp,
		uiSlider::Setup(tr("'Nice' level (0-19)")), "Nice level" );
    nicefld_->setMinValue( -0.5 ); nicefld_->setMaxValue( 19.5 );
    nicefld_->setValue( hdl_.niceLevel() );
    if ( avmachfld_ )
	nicefld_->setPrefWidthInChar( hostnmwdth );

    jrppolselfld_ = new uiComboBox( jrppolgrp, "JobRun policy" );
    jrppolselfld_->addItem( tr("Run") );
    jrppolselfld_->addItem( uiStrings::sPause() );
    jrppolselfld_->addItem( tr("Schedule") );
    jrppolselfld_->setCurrentItem( ((int)0) );
    mAttachCB( jrppolselfld_->selectionChanged, uiMMBatchJobDispatcher::jrpSel);
    jrppolselfld_->attach( alignedBelow, nicefld_ );
    if ( avmachfld_ ) jrppolselfld_->setPrefWidthInChar( hostnmwdth );
    jrpworklbl_ = new uiLabel( jrppolgrp, tr("Processes") );
    jrpworklbl_->attach( rightOf, jrppolselfld_ );

    const char* envstr = GetEnvVar( "DTECT_STOP_OFFICEHOURS" );

    jrpstartfld_ = new uiGenInput( jrppolgrp, uiString::emptyString(),
				   envstr ? envstr : "18:00" );

    jrpstartfld_->attach( rightOf, jrpworklbl_ );

    envstr = GetEnvVar( "DTECT_START_OFFICEHOURS" );
    jrpstopfld_ = new uiGenInput( jrppolgrp, tr("and"),
				  envstr ? envstr : "7:30" );
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

    mAttachCB( postFinalize(), uiMMBatchJobDispatcher::initWin );
}


uiMMBatchJobDispatcher::~uiMMBatchJobDispatcher()
{
    detachAllNotifiers();
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
    jobrunner_->setNiceNess( hdl_.niceLevel() );

    const Network::Authority auth = jobrunner_->authority();
    const BufferString clienthost = auth.getHost();
    if ( DBG::isOn(DBG_MM) )
    {
	BufferString msg( "Started listening server on machine ",
		GetLocalHostName(), " with IP address: " );
	msg.add( auth.getConnHost(Network::Authority::IPv4) ).addNewLine()
	   .add( "Server is listening to host " ).add( clienthost )
	   .add( " on port " ).add( auth.getPort() );
	DBG::message( msg );
    }

    const char* localhoststr = Network::Socket::sKeyLocalHost();
    if ( !auth.addressIsValid() ||
	 clienthost == localhoststr ||
	 clienthost == System::hostAddress(localhoststr,true) ||
	 clienthost == System::hostAddress(localhoststr,false) )
    {
	uiMSG().error( tr("Invalid IP address on the client machine: %1")
					.arg(clienthost) );
	setCancelText( uiStrings::sClose() );
	button(OK)->display( false );
	deleteAndZeroPtr( jobrunner_ );
	return;
    }

    mAttachCB( jobrunner_->preJobStart, uiMMBatchJobDispatcher::jobPrep );
    mAttachCB( jobrunner_->postJobStart, uiMMBatchJobDispatcher::jobStart );
    mAttachCB( jobrunner_->jobFailed, uiMMBatchJobDispatcher::jobFail );
    mAttachCB( jobrunner_->msgAvail, uiMMBatchJobDispatcher::infoMsgAvail );

    setOkText( tr("Finish") );
    setCancelText( uiStrings::sAbort() );

    timer_ = new Timer("uiMMBatchJobDispatcher timer");
    mAttachCB( timer_->tick, uiMMBatchJobDispatcher::doCycle );
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
	deleteAndZeroPtr( jobrunner_ );
	addbut_->setSensitive( true );
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


    int niceval = nicefld_->getIntValue();
    if ( niceval > 19 ) niceval = 19; if ( niceval < 0 ) niceval = 0;
    jobrunner_->setNiceNess( niceval );
    timer_->start( 250, true );
}


void uiMMBatchJobDispatcher::stopPush( CallBacker* )
{
    const int rhidx = runnerHostIdx( curUsedMachName() );
    if ( !jobrunner_ || rhidx<0 )
	return;

    jobrunner_->removeHost( rhidx );
    addbut_->setSensitive( true );
}


void uiMMBatchJobDispatcher::stopAllPush( CallBacker* )
{
    if ( jobrunner_ )
	jobrunner_->stopAll();
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
			      uiTextFileDlg::Setup(toUiString(fnm)), fnm );
    logvwer_->go();
}


void uiMMBatchJobDispatcher::jrpSel( CallBacker* )
{
    const bool doschedule = jrppolselfld_->currentItem() == 2;
    jrpstartfld_->display( doschedule );
    jrpstopfld_->display( doschedule );
    jrpworklbl_->setText( doschedule ? tr("Between") : tr("Processes") );
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
    {
	msg.add( " on " )
	   .add( ji.hostdata_->isStaticIP() ? ji.hostdata_->getIPAddress()
					    : ji.hostdata_->getHostName(false));
    }

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
    statusBar()->message( uiString::emptyString(), 3 );
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
    msg= tr("%1 %2 %3").arg(msg).arg(toUiString(newpart)).
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

    progrfld_->append( "Processing completed" );
    setCtrlStyle( CloseOnly );
    uiButton* cancbuttn = button( uiDialog::CANCEL );
    if ( cancbuttn )
	cancbuttn->display(false);
    clearAliveDisp();

    removeTempResults();
    deleteAndZeroPtr( jobrunner_ );
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


static bool hostOK( const HostData& hd, const char* rshcomm,
		    BufferString& errmsg )
{
    OS::MachineCommand remotecmd;
    remotecmd.setRemExec( rshcomm )
	     .setHostName( hd.connAddress() )
	     .setHostIsWindows( hd.isWindows() );
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


#define mErrRet(s) { uiMSG().error(s); return; }

void uiMMBatchJobDispatcher::addPush( CallBacker* )
{
    const int nrmach = avmachfld_ ? avmachfld_->size() : 1;
    if ( nrmach < 1 )
	return;

    const int nrsel = avmachfld_ ? avmachfld_->nrChosen() : 1;
    if ( nrsel < 1 )
	mErrRet(tr("Please select one or more hosts"))

    MouseCursorChanger cursorchanger(MouseCursor::Wait);
    if ( !jobrunner_ )
    {
	startWork( 0 );
	if ( !jobrunner_ )
	    return;
    }

    const BufferString localhnm( GetLocalHostName() );

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

	if ( !__iswin__ && !hd->isWindows() )
	{
	    BufferString errmsg;
	    if ( !hd->isKnownAs(localhnm)
		    && !hostOK(*hd,hdl_.loginCmd(),errmsg) )
		{ progrfld_->append( errmsg.buf() ); continue; }
	}

	if ( !jobrunner_->addHost(*hd) && jobrunner_->jobsLeft() > 0 )
	{
	    uiString msg = tr("Could not start job");
	    if ( isMultiHost() )
		msg.arg( " on %1" ).arg( hnm );
	    if ( jobrunner_->errorMsg().isSet() )
		msg.arg( " : " ).arg( jobrunner_->errorMsg() );
	    progrfld_->append( msg.getFullString() );
	}
    }

    if ( hdl_.size() == 1 )
	addbut_->setSensitive( false );
}


const char* uiMMBatchJobDispatcher::curUsedMachName()
{
   mDeclStaticString( mach );
   mach = isMultiHost() ? usedmachfld_->getText() : usedmachfld_->textOfItem(0);

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
	    uiString msg = tr("This will stop all processing!%1");
	    if ( !haveTmpProcFiles() )
	    {
		msg.arg("\n\nDo you want to do this?");
		res = uiMSG().askGoOn( msg ) ? 0 : -1;
	    }
	    else
	    {
		msg.arg("\n\nDo you want to delete already processed data?");
		res = uiMSG().askDelete( msg, true );
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
		    tr("This will stop further processing and wrap up"),false) )
	    return false;
    }

    if ( !jobrunner_ )
	return true; //Already wrapped up

    jobrunner_->stopAll();
    return wrapUp();
}
