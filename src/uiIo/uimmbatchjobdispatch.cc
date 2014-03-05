/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          April 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimmbatchjobdispatch.h"
#include "uiseismmjobdispatch.h"


uiMMBatchJobDispatcher::uiMMBatchJobDispatcher( uiParent* p,
						const Batch::JobSpec& js )
	: uiDialog(p,uiDialog::Setup("Job management",mNoDlgTitle,mNoHelpID)
		.nrstatusflds(-1)
		.fixedsize(true))
	, jobspec_(js)
	, hdl_(*new HostDataList)
	, avmachfld_(0), usedmachfld_(0)
	, nicefld_(0)
	, logvwer_(0)
	, progrfld_(0) , progbar_(0)
	, jrpstartfld(0)_, jrpstopfld_(0)
	, timer_(0)
	, nrcyclesdone_(0)
{
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

    uiObject* sepattach = 0;
    uiGroup* specparsgroup_ = new uiGroup( "Specific parameters group" );
    uiSeparator* sep = new uiSeparator( this, "Hor sep 1" );
    sep->attach( stretchedBelow, specparsgroup_ );

    uiGroup* machgrp = new uiGroup( this, "Machine handling" );
    if ( multihost )
    {
	avmachfld_ = new uiLabeledListBox( machgrp, "Available hosts", true,
					  uiLabeledListBox::AboveMid );
	for ( int idx=0; idx<hdl_.size(); idx++ )
	{
	    const HostData& hd = *hdl_[idx];
	    BufferString nm( hd.name() );
	    const int nraliases = hd.nrAliases();
	    for ( int aliasidx=0; aliasidx<nraliases; aliasidx++ )
		{ nm += " / "; nm += hd.alias(aliasidx); }
	    avmachfld_->box()->addItem( nm );
	}

	avmachfld_->setPrefWidthInChar( mCast(float,hostnmwdth) );
	avmachfld_->setPrefHeightInChar( mCast(float,maxhostdisp) );
    }

    uiGroup* usedmachgrp = new uiGroup( machgrp, "Used machine handling" );
    usedmachfld_ = new uiLabeledListBox( usedmachgrp,
				    multihost ? "Used hosts" : "", false,
				    uiLabeledListBox::AboveMid );
    usedmachfld_->box()->setPrefWidthInChar( hostnmwdth );
    usedmachfld_->box()->setPrefHeightInChar( maxhostdisp );

    uiButton* stopbut = new uiPushButton( usedmachgrp, "St&op", true );
    stopbut->activated.notify( mCB(this,uiMMBatchJobDispatcher,stopPush) );
    uiButton* vwlogbut = new uiPushButton( usedmachgrp, "&View log", false );
    vwlogbut->activated.notify( mCB(this,uiMMBatchJobDispatcher,vwLogPush) );
    vwlogbut->attach( rightAlignedBelow, usedmachfld_ );

    uiButton* addbut;
    if ( multihost )
    {
	stopbut->attach( alignedBelow, usedmachfld_ );
	addbut = new uiPushButton( machgrp, ">> &Add >>", true );
	if ( avmachfld_ )  addbut->attach( centeredRightOf, avmachfld_ );
	usedmachgrp->attach( ensureRightOf, addbut );
	machgrp->setHAlignObj( avmachfld_ );
    }
    else
    {
	addbut = new uiPushButton( usedmachgrp, "St&art", true );
	addbut->attach( alignedBelow, usedmachfld_ );
	stopbut->attach( centeredBelow, usedmachfld_ );
	machgrp->setHAlignObj( stopbut );
    }
    addbut->activated.notify( mCB(this,uiMMBatchJobDispatcher,addPush) );

    if ( sep )
	machgrp->attach( ensureBelow, sep );

    uiGroup* jrppolgrp = new uiGroup( this, "Job run policy group" );

    nicefld = new uiSlider( jrppolgrp, "Nice level" );
    nicefld->setMinValue( -0.5 ); nicefld->setMaxValue( 19.5 );
    nicefld->setValue( hdl_.defNiceLevel() );
    uiLabel* nicelbl = new uiLabel( jrppolgrp, "'Nice' level (0-19)" );
    nicelbl->attach( rightOf, nicefld );

    if ( avmachfld_ ) nicefld->setPrefWidthInChar( hostnmwdth );

    jrppolselfld_ = new uiComboBox( jrppolgrp, "JobRun policy" );
    jrppolselfld_->addItem( "Run" );
    jrppolselfld_->addItem( "Pause" );
    jrppolselfld_->addItem( "Go - Only between" );
    jrppolselfld_->setCurrentItem( ((int)0) );
    jrppolselfld_->selectionChanged.notify( mCB(this,uiMMBatchJobDispatcher,jrpSel) );
    jrppolselfld_->attach( alignedBelow, nicefld );
    if ( avmachfld_ ) jrppolselfld_->setPrefWidthInChar( hostnmwdth );
    jrpworklbl = new uiLabel( jrppolgrp, "Processes" );
    jrpworklbl->attach( rightOf, jrppolselfld_ );

    const char* envstr = GetEnvVar( "DTECT_STOP_OFFICEHOURS" );
    jrpstartfld = new uiGenInput( jrppolgrp, "", envstr ? envstr : "18:00" );
    jrpstartfld->attach( rightOf, jrppolselfld_ );

    envstr = GetEnvVar( "DTECT_START_OFFICEHOURS" );
    jrpstopfld = new uiGenInput( jrppolgrp, "and", envstr ? envstr : "7:30" );
    jrpstopfld->attach( rightOf, jrpstartfld );

    jrppolgrp->setHAlignObj( nicefld );
    jrppolgrp->attach( ensureBelow, machgrp );

    sep = new uiSeparator( this, "Hor sep 2" );
    sep->attach( stretchedBelow, jrppolgrp );

    progrfld = new uiTextEdit( this, "Processing progress", true );
    progrfld->attach( ensureBelow, sep );
    progrfld->attach( widthSameAs, sep );
    progrfld->setPrefHeightInChar( 7 );

    progbar = new uiProgressBar( this, "", 1, 0 );
    progbar->attach( widthSameAs, progrfld );
    progbar->attach( alignedBelow, progrfld );

    postFinalise().notify( mCB(this,uiMMBatchJobDispatcher,initWin) );
}


uiMMBatchJobDispatcher::~uiMMBatchJobDispatcher()
{
    delete logvwer;
    delete jobprov;
    delete jobrunner;
    delete outioobjinfo;
    delete timer_;

    delete &hdl_;
}


void uiMMBatchJobDispatcher::initWin( CallBacker* cb )
{
    jrpSel( cb );
    if ( !avmachfld_ && (is2d || isrestart) )
	addPush(cb);
}


void uiMMBatchJobDispatcher::setNiceNess()
{
    if ( !jobrunner ) return;
    int v = nicefld->getIntValue();
    if ( v > 19 ) v = 19;
    if ( v < 0 ) v = 0;
    jobrunner->setNiceNess( v );
}


#define mErrRet(s) { uiMSG().error(s); return; }

void uiMMBatchJobDispatcher::startWork( CallBacker* )
{
    BufferString tmpstordir;
    if ( !tmpstordirfld )
	iop.get( sKey::TmpStor(), tmpstordir );
    else
    {
	tmpstordir = tmpstordirfld->getInput();
	if ( !File::isWritable(tmpstordir) )
	    mErrRet("The temporary storage directory is not writable")
	tmpstordir = SeisJobExecProv::getDefTempStorDir( tmpstordir );
	const_cast<IOPar&>(iop).set( sKey::TmpStor(), tmpstordir );
	tmpstordirfld->setSensitive( false );
    }

    jobprov = new SeisJobExecProv( progname, iop );
    if ( jobprov->errMsg() )
	mErrRet(jobprov->errMsg())

    int nr_inl_job = 1;
    if ( inlperjobfld )
    {
	nr_inl_job = inlperjobfld->getIntValue();
	if ( nr_inl_job < 1 ) nr_inl_job = 1;
	if ( nr_inl_job > 100 ) nr_inl_job = 100;
	inlperjobfld->setValue( nr_inl_job );
	inlperjobfld->setSensitive( false );
    }

    mkJobRunner( nr_inl_job );

    if ( !is2d )
    {
	iop.get( sKey::TmpStor(), tmpstordir );
	if ( !File::isDirectory(tmpstordir) )
	{
	    if ( File::exists(tmpstordir) )
		File::remove( tmpstordir );
	    File::createDir( tmpstordir );
	}
	if ( !File::isDirectory(tmpstordir) )
	    mErrRet("Cannot create temporary storage directory")
    }

    jobprov->pars().write( parfnm, sKey::Pars() );

    setOkText( "Finish Now" );
    setCancelText( "Abort" );


    timer = new Timer("uiMMBatchJobDispatcher timer");
    timer->tick.notify( mCB(this,uiMMBatchJobDispatcher,doCycle) );
    timer->start( 100, true );
}

#define mNrInlPerJobProcKey	"Nr of Inlines per Job"

int uiMMBatchJobDispatcher::defltNrInlPerJob( const IOPar& inputpar )
{
    mDefineStaticLocalObject( int, nr_inl_job, (-1));
    inputpar.get( mNrInlPerJobProcKey, nr_inl_job );

    if ( nr_inl_job <= 0 )
	nr_inl_job = InlineSplitJobDescProv::defaultNrInlPerJob();

    return nr_inl_job;
}


void uiMMBatchJobDispatcher::mkJobRunner( int nr_inl_job )
{
    if ( jobrunner ) delete jobrunner;

    jobrunner = jobprov->getRunner( nr_inl_job );
    if ( jobprov->errMsg() )
    {
	delete jobrunner; jobrunner = 0;
	mErrRet(jobprov->errMsg())
    }

    jobrunner->setFirstPort( hdl_.firstPort() );
    jobrunner->setRshComm( hdl_.rshComm() );
    jobrunner->setNiceNess( hdl_.defNiceLevel() );

    jobrunner->preJobStart.notify( mCB(this,uiMMBatchJobDispatcher,jobPrepare) );
    jobrunner->postJobStart.notify( mCB(this,uiMMBatchJobDispatcher,jobStarted) );
    jobrunner->jobFailed.notify( mCB(this,uiMMBatchJobDispatcher,jobFailed) );
    jobrunner->msgAvail.notify( mCB(this,uiMMBatchJobDispatcher,infoMsgAvail) );
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


void uiMMBatchJobDispatcher::doCycle( CallBacker* )
{
    nrcyclesdone_++;

    if ( jobrunner->nextStep() == Executor::ErrorOccurred() )
    {
	delete jobrunner;
	jobrunner = 0;
	return;
    }

    pauseJobs();
    updateCurMachs();
    updateAliveDisp();

    if ( jobrunner->jobsLeft() == 0 )
    {
	if ( wrapUp(false) )
	    return;

	mkJobRunner(1);

	if ( !jobrunner )
	    return;
    }

    setNiceNess();
    updateCurMachs();
    updateAliveDisp();

    timer->start( 250, true );
}


void uiMMBatchJobDispatcher::updateAliveDisp()
{
    const int nrdispstrs = 6;
    const char* dispstrs[]
	= { ">..", ".>.", "..>", "..<", ".<.", "<.." };
    statusBar()->message( dispstrs[ nrcyclesdone_ % nrdispstrs ], 3 );

    const int totsteps = mCast( int, jobrunner->totalNr() );
    const int nrdone = mCast( int, jobrunner->nrDone() );
    const bool hastot = totsteps > 0;
    progbar->display( hastot );
    if ( hastot )
    {
	progbar->setTotalSteps( totsteps );
	progbar->setProgress( nrdone );

	const float fpct = 100.f * ((float)nrdone) / totsteps;
	int pct = (int)fpct; if ( pct > 100 ) pct = 100;
	BufferString newcap( "[" ); newcap += pct; newcap += "%] ";
	newcap += caption;
	setCaption( newcap );
    }
}


#define mReturn()    { deepErase( machs ); return; }
void uiMMBatchJobDispatcher::updateCurMachs()
{
    BufferStringSet machs;
    jobrunner->showMachStatus( machs );
    sort( machs );

    const int oldsz = usedmachfld_->box()->size();
    const int newsz = machs.size();

    int curit = oldsz ? usedmachfld_->box()->currentItem() : -1;
    usedmachfld_->box()->setEmpty();
    if ( newsz )
    {
	usedmachfld_->box()->addItems( machs );
	if ( curit >= usedmachfld_->box()->size() )
	    curit = usedmachfld_->box()->size() - 1;
	usedmachfld_->box()->setCurrentItem(curit);
    }
    else
	usedmachfld_->box()->clearSelection();

    mReturn();
}


#ifdef __win__
#define mReDirectToNull checkcmd += " > NUL"
#else
#define mReDirectToNull checkcmd += " > /dev/null"
#endif

static bool isHostOK( const HostData& hd, const char* rshcomm,
		      BufferString& errmsg )
{
    BufferString remotecmd( rshcomm );
    remotecmd += " "; remotecmd += hd.name();
    BufferString checkcmd( remotecmd ); checkcmd += " whoami";
    mReDirectToNull;
    if ( system(checkcmd.buf()) )
    {
	errmsg = "Cannot establish a ";
	errmsg += rshcomm; errmsg += " connection with ";
	errmsg += hd.name();
	return false;
    }

    checkcmd = remotecmd; checkcmd += " cd ";
    checkcmd += hd.convPath( HostData::Appl, GetSoftwareDir(0) ).fullPath();
    if ( system(checkcmd.buf()) )
    {
	errmsg = "Cannot find application directory ";
	errmsg += hd.name(); errmsg += ":";
	errmsg += hd.convPath(HostData::Appl, GetSoftwareDir(0)).fullPath();
	errmsg += "\nMake sure the filesystem is mounted on remote host ";
	return false;
    }

    checkcmd = remotecmd; checkcmd += " cd ";
    checkcmd += hd.convPath( HostData::Data, GetBaseDataDir() ).fullPath();
    if ( system(checkcmd.buf()) )
    {
	errmsg = "Cannot find data directory ";
	errmsg += hd.name(); errmsg += ":";
	errmsg += hd.convPath(HostData::Data, GetBaseDataDir()).fullPath();
	errmsg += "\nMake sure the filesystem is mounted on remote host";
	return false;
    }

    return true;
}


void uiMMBatchJobDispatcher::addPush( CallBacker* )
{
    uiListBox* lb = avmachfld_ ? avmachfld_->box() : 0;
    const int nrmach = lb ? lb->size() : 1;
    if ( nrmach < 1 ) return;
    const int nrsel = lb ? lb->nrSelected() : 1;
    if ( nrsel < 1 )
	mErrRet("Please select one or more hosts")

    for ( int idx=0; idx<nrmach; idx++ )
    {
	if ( lb && !lb->isSelected(idx) ) continue;

	if ( !jobrunner )
	    startWork(0);
	if ( !jobrunner )
	    continue;

	BufferString hnm = lb ? lb->textOfItem( idx ) : hdl_[0]->name();
	char* ptr = hnm.find( ' ' );
	if ( ptr ) *ptr = '\0';

	const HostData* hd = hdl_.find( hnm.buf() );
	if ( !hd ) { pErrMsg("Huh"); continue; }

#ifndef __win__
	BufferString errmsg;
	if ( !hd->isKnownAs(HostData::localHostName())
		&& !isHostOK(*hd,hdl_.rshComm(),errmsg) )
	{
	    progrfld->append( errmsg.buf() );
	    continue;
	}
#endif

	if ( !jobrunner->addHost(*hd) && jobrunner->jobsLeft() > 0 )
	{
	    BufferString msg = "Could not start job";
	    if ( lb )
		{ msg += " on "; msg += hnm; }
	    if ( jobrunner->errorMsg() )
	    {
		msg += " : ";
		msg += jobrunner->errorMsg();
	    }
	    progrfld->append( msg );
	}
    }
}


const char* uiMMBatchJobDispatcher::curUsedMachName()
{
   mDeclStaticString( mach );
   mach = usedmachfld_->box()->getText();

   char* ptr = mach.find( " -:- ");
   if ( ptr ) *ptr='\0';

   return mach;
}


void uiMMBatchJobDispatcher::stopPush( CallBacker* )
{
    int rhidx = runnerHostIdx( curUsedMachName() );
    if ( rhidx >= 0 )
	jobrunner->removeHost( rhidx );
}


void uiMMBatchJobDispatcher::vwLogPush( CallBacker* )
{
    if ( !jobrunner ) return;

    BufferString hostnm( curUsedMachName() );
    const HostNFailInfo* hfi = 0;
    const ObjectSet<HostNFailInfo>& hi = jobrunner->hostInfo();
    for ( int idx=0; idx<hi.size(); idx++ )
    {
	if ( hi[idx]->hostdata_.isKnownAs(hostnm) )
	    { hfi = hi[idx]; break; }
    }
    if ( !hfi ) return;

    JobInfo* ji = jobrunner->currentJob( hfi );
    FilePath logfp( jobrunner->getBaseFilePath(*ji, hfi->hostdata_) );
    logfp.setExtension( ".log", false );

    delete logvwer;
    logvwer = new uiTextFileDlg( this, uiTextFileDlg::Setup(logfp.fullPath())
					.scroll2bottom(true) );
    logvwer->go();
}


static void addObjNm( BufferString& msg, const JobRunner* jr, int nr )
{
    msg += jr->descProv()->objType(); msg += " ";
    msg += jr->descProv()->objName( nr );
}


void uiMMBatchJobDispatcher::jobPrepare( CallBacker* cb )
{
    if ( !is2d ) return;

    // Put a copy of the .2ds file in the proc directory
    // Makes sure 2D changes are only done on master
    if ( !lsfileemitted )
    {
	const BufferString lsfnm =
		FilePath(jobrunner->procDir(),outlsfilename).fullPath();
	lsfileemitted = jobprov->emitLSFile( lsfnm );
    }
    if ( lsfileemitted )
    {
	FilePath fp( jobrunner->curJobFilePath() );
	fp.setFileName( outlsfilename );
	const BufferString lsfnm( fp.fullPath() );
		// This lsfnm may differ from above - remote directories!
	jobprov->preparePreSet( jobrunner->curJobIOPar(),
				SeisJobExecProv::sKeyOutputLS() );
	jobrunner->curJobIOPar().set(
		IOPar::compKey(SeisJobExecProv::sKeyWorkLS(),sKey::FileName()),
		lsfnm );
    }
}


void uiMMBatchJobDispatcher::jobStarted( CallBacker* cb )
{
    const JobInfo& ji = jobrunner->curJobInfo();
    BufferString msg( "Started processing " );
    addObjNm( msg, jobrunner, ji.descnr_ );
    if ( ji.hostdata_ )
	{ msg += " on "; msg += ji.hostdata_->name(); }
    progrfld->append( msg );
}


void uiMMBatchJobDispatcher::jobFailed( CallBacker* cb )
{
    const JobInfo& ji = jobrunner->curJobInfo();
    BufferString msg( "Failure for " );
    addObjNm( msg, jobrunner, ji.descnr_ );
    if ( ji.hostdata_ )
	{ msg += " on "; msg += ji.hostdata_->name(); }
    if ( !ji.infomsg_.isEmpty() )
	{ msg += ": "; msg += ji.infomsg_; }
    progrfld->append( msg );
}


void uiMMBatchJobDispatcher::infoMsgAvail( CallBacker* cb )
{
    const JobInfo& ji = jobrunner->curJobInfo();
    if ( ji.infomsg_.isEmpty() ) { pErrMsg("huh?"); return; }

    BufferString msg( "Info for " );
    addObjNm( msg, jobrunner, ji.descnr_ );
    if ( ji.hostdata_ )
	{ msg += " on "; msg += ji.hostdata_->name(); }

    msg += ": "; msg += ji.infomsg_;
    progrfld->append( msg );
}


void uiMMBatchJobDispatcher::jrpSel( CallBacker* )
{
    const bool isgo = *jrppolselfld_->text() == 'G';
    jrpstartfld->display( isgo );
    jrpstopfld->display( isgo );
    jrpworklbl->display( !isgo );
}


void uiMMBatchJobDispatcher::pauseJobs()
{
    if ( !jobrunner ) return;

    const char* txt = jrppolselfld_->text();
    bool pause = *txt == 'P';
    if ( *txt == 'G' )
    {
	const int t = getSecs( Time::getDateTimeString() );
	const int t0 = getSecs( jrpstartfld->text() );
	const int t1 = getSecs( jrpstopfld->text() );

	bool run = t1 >= t0 ? t >= t0 && t <= t1
			    : t >= t0 || t <= t1;
        pause = !run;
    }

    const int nrhosts = jobrunner->hostInfo().size();
    for ( int idx=0; idx<nrhosts; idx++ )
	jobrunner->pauseHost( idx, pause );
}


static void rmTmpSeis( SeisJobExecProv* jp )
{
    if ( !jp ) return;

    bool removed = jp->removeTempSeis();
    int count = 30;

    while ( !removed && count-- > 0 )
    {
	sleepSeconds( 1 );
	removed = jp->removeTempSeis();
    }

    if ( !removed )
	ErrMsg( "Could not remove all temporary seismics" );
}


bool uiMMBatchJobDispatcher::readyForPostProcess()
{
    if ( jobrunner->jobsLeft() > 0 ) return false;
    const int nrfailed = jobrunner->nrJobs(true);
    if ( nrfailed < 1 ) return true;

    BufferString msg( "Failed " );
    msg += jobrunner->descProv()->objType();
    msg += nrfailed > 1 ? "s:\n" : ": ";
    BufferString newpart;
    bool needspace = false;
    for ( int idx=0; idx<nrfailed; idx++ )
    {
	const JobInfo& ji = jobrunner->jobInfo( idx, true );
	if ( needspace ) newpart += " ";
	newpart += jobrunner->descProv()->objName( ji.descnr_ );
	if ( newpart.size() < 70 )
	    needspace = true;
	else
	{
	    msg += newpart;
	    newpart = "\n"; needspace = false;
	}
    }
    msg += newpart;
    msg += "\n\nDo you want to re-try?";
    return !uiMSG().askGoOn(msg);
}


bool uiMMBatchJobDispatcher::wrapUp( bool force )
{
    if ( !force && !readyForPostProcess() )
	return false;

    Executor* exec = jobprov ? jobprov->getPostProcessor() : 0;
    if ( exec )
    {
	uiTaskRunner uitr( this );
	const bool res = TaskRunner::execute( &uitr, *exec );
	delete exec;
	if ( !res )
	    return false;
    }

    progrfld->append( "Processing completed" );
    setCtrlStyle( CloseOnly );
    button(uiDialog::CANCEL)->display(false);
    rmTmpSeis( jobprov );
    delete jobrunner; jobrunner = 0;
    return true;
}


bool uiMMBatchJobDispatcher::rejectOK( CallBacker* )
{
    if ( !outioobjinfo->ioObj() || !jobrunner ) return true;

    int res = 1;
    if ( jobrunner->jobsDone() > 0 || jobrunner->jobsInProgress() > 0 )
    {
	BufferString msg = "This will stop all processing!";
	if ( is2d )
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
    if ( res == -1 ) return false;

    jobrunner->stopAll();
    if ( res == 1 )
	rmTmpSeis( jobprov );

    return true;
}


bool uiMMBatchJobDispatcher::acceptOK(CallBacker*)
{
    if ( !outioobjinfo->ioObj() || !jobrunner ) return true;

    if ( usedmachfld_->box()->size() && !uiMSG().askGoOn(
	    "This will stop further processing and wrap up",false) )
	return false;

    jobrunner->stopAll();
    return wrapUp( true );
}
