/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          April 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiseismmproc.h"
#include "uiseisioobjinfo.h"
#include "seisjobexecprov.h"
#include "jobrunner.h"
#include "jobdescprov.h"
#include "jobinfo.h"
#include "hostdata.h"
#include "iopar.h"
#include "ioman.h"
#include "iostrm.h"
#include "oddirs.h"
#include "timer.h"
#include "file.h"
#include "filepath.h"
#include "executor.h"
#include "ptrman.h"
#include "strmprov.h"
#include "envvars.h"
#include "keystrs.h"
#include "settings.h"
#include "seissingtrcproc.h"
#include "thread.h"
#include "timefun.h"

#include "uilabel.h"
#include "uilistbox.h"
#include "uicombobox.h"
#include "uiprogressbar.h"
#include "uibutton.h"
#include "uitextedit.h"
#include "uiseparator.h"
#include "uitextfile.h"
#include "uiiosel.h"
#include "uitaskrunner.h"
#include "uimsg.h"
#include "uistatusbar.h"
#include "uislider.h"
#include "uigeninput.h"
#include "uilabel.h"
#include <stdlib.h>
#include <iostream>

static const char* outlsfilename = "outls.2ds";


uiSeisMMProc::uiSeisMMProc( uiParent* p, const IOPar& ip,
			    const char* prnm, const char* pfnm )
	: uiDialog(p,uiDialog::Setup("Job management",mNoDlgTitle,"103.2.0")
		.nrstatusflds(-1)
		.fixedsize(true))
	, progname(prnm)
	, parfnm(pfnm)
	, hdl(*new HostDataList)
    	, iop(*new IOPar(ip))
	, avmachfld(0), usedmachfld(0)
	, jrppolselfld(0), nicefld(0)
	, tmpstordirfld(0), inlperjobfld(0), logvwer(0)
	, progrfld(0) , progbar(0)
	, jrpstartfld(0), jrpstopfld(0)
    	, jobprov(0), jobrunner(0)
    	, outioobjinfo(0), isrestart(false)
	, lsfileemitted(false)
	, timer(0)
	, nrcyclesdone(0)
{
    MultiID outid = iop.find( SeisJobExecProv::outputKey(iop) );
    outioobjinfo = new uiSeisIOObjInfo( outid );
    if ( !outioobjinfo->isOK() )
	{ setOkText( "Output cube not found in Object Management" );
	    setCancelText( "" ); return; }

    const int nrhosts = hdl.size();
    const bool multihost = nrhosts > 1;
    int maxhostdisp = multihost ? (nrhosts>7 ? 8 : (nrhosts<3 ? 3 : nrhosts))	
				: 1;
    
    const int hostnmwdth = 30;
    is2d = outioobjinfo->is2D();

    setOkText( "  Dismiss  " );
    setTitleText( multihost ? "Multi-Machine Processing"
		    : (is2d ? "Multi-line processing"
			    : "Line-split processing") );
    FixedString res = iop.find( sKey::Target() );
    caption = "Processing";
    if ( res )
	{ caption += " '"; caption += res; caption += "'"; }
    setCaption( caption );

    statusBar()->addMsgFld( "Message", Alignment::Left, 20 );
    statusBar()->addMsgFld( "DoneTxt", Alignment::Right, 20 );
    statusBar()->addMsgFld( "NrDone", Alignment::Left, 10 );
    statusBar()->addMsgFld( "Activity", Alignment::Left, 1 );

    uiSeparator* sep = 0;
    uiObject* sepattach = 0;
    uiObject* inlperjobattach = 0;
    if ( !is2d )
    {
	BufferString tmpstordir = iop.find(sKey::TmpStor()).str();
	isrestart = !tmpstordir.isEmpty();
	if ( !isrestart )
	{
	    tmpstordir = SeisJobExecProv::getDefTempStorDir();
	    FilePath fp( tmpstordir ); fp.setFileName( 0 );
	    tmpstordir = fp.fullPath();
	}

	if ( isrestart )
	{
	    BufferString msg( sKey::TmpStor() ); msg += ": ";
	    uiLabel* tmpstorloc = new uiLabel( this, msg );

	    inlperjobattach = new uiLabel( this, tmpstordir );
	    inlperjobattach->attach( rightOf, tmpstorloc );
	}
	else
	{
	    tmpstordirfld = new uiIOFileSelect( this,
			    sKey::TmpStor(), false, tmpstordir );
	    tmpstordirfld->usePar( uiIOFileSelect::tmpstoragehistory() );
	    if ( !tmpstordir.isEmpty() && File::isDirectory(tmpstordir) )
		tmpstordirfld->setInput( tmpstordir );
	    tmpstordirfld->selectDirectory( true );
	    tmpstordirfld->stretchHor( true );
	    inlperjobattach = tmpstordirfld->mainObject();
	}

	inlperjobfld = new uiGenInput( this, "Nr inlines per job",
				       IntInpSpec( defltNrInlPerJob(iop) ) );

	sepattach = inlperjobfld->mainObject();

	inlperjobfld->attach( alignedBelow, inlperjobattach );
    }

    if ( sepattach )
    {
	sep = new uiSeparator( this, "Hor sep 1", true );
	sep->attach( stretchedBelow, sepattach );
    }

    uiGroup* machgrp = new uiGroup( this, "Machine handling" );
    if ( multihost )
    {
	avmachfld = new uiLabeledListBox( machgrp, "Available hosts", true,
					  uiLabeledListBox::AboveMid );
	for ( int idx=0; idx<hdl.size(); idx++ )
	{
	    const HostData& hd = *hdl[idx];
	    BufferString nm( hd.name() );
	    const int nraliases = hd.nrAliases();
	    for ( int aliasidx=0; aliasidx<nraliases; aliasidx++ )
		{ nm += " / "; nm += hd.alias(aliasidx); }
	    avmachfld->box()->addItem( nm );
	}

	avmachfld->setPrefWidthInChar( hostnmwdth );
	avmachfld->setPrefHeightInChar( maxhostdisp );
    }

    uiGroup* usedmachgrp = new uiGroup( machgrp, "Used machine handling" );
    usedmachfld = new uiLabeledListBox( usedmachgrp,
				    multihost ? "Used hosts" : "", false,
				    uiLabeledListBox::AboveMid );
    usedmachfld->box()->setPrefWidthInChar( hostnmwdth );
    usedmachfld->box()->setPrefHeightInChar( maxhostdisp );

    uiButton* stopbut = new uiPushButton( usedmachgrp, "St&op", true );
    stopbut->activated.notify( mCB(this,uiSeisMMProc,stopPush) );
    uiButton* vwlogbut = new uiPushButton( usedmachgrp, "&View log", false );
    vwlogbut->activated.notify( mCB(this,uiSeisMMProc,vwLogPush) );
    vwlogbut->attach( rightAlignedBelow, usedmachfld );

    uiButton* addbut;
    if ( multihost )
    {
	stopbut->attach( alignedBelow, usedmachfld );
	addbut = new uiPushButton( machgrp, ">> &Add >>", true );
	if ( avmachfld )  addbut->attach( centeredRightOf, avmachfld );
	usedmachgrp->attach( ensureRightOf, addbut );
	machgrp->setHAlignObj( avmachfld );
    }
    else
    {
	addbut = new uiPushButton( usedmachgrp, "St&art", true );
	addbut->attach( alignedBelow, usedmachfld );
	stopbut->attach( centeredBelow, usedmachfld );
    	machgrp->setHAlignObj( stopbut );
    }
    addbut->activated.notify( mCB(this,uiSeisMMProc,addPush) );

    if ( sep )
	machgrp->attach( ensureBelow, sep );

    uiGroup* jrppolgrp = new uiGroup( this, "Job run policy group" );

    nicefld = new uiSlider( jrppolgrp, "Nice level" );
    nicefld->setMinValue( -0.5 ); nicefld->setMaxValue( 19.5 );
    nicefld->setValue( hdl.defNiceLevel() );
    uiLabel* nicelbl = new uiLabel( jrppolgrp, "'Nice' level (0-19)" );
    nicelbl->attach( rightOf, nicefld );

    if ( avmachfld ) nicefld->setPrefWidthInChar( hostnmwdth );

    jrppolselfld = new uiComboBox( jrppolgrp, "JobRun policy" );
    jrppolselfld->addItem( "Run" );
    jrppolselfld->addItem( "Pause" );
    jrppolselfld->addItem( "Go - Only between" );
    jrppolselfld->setCurrentItem( ((int)0) );
    jrppolselfld->selectionChanged.notify( mCB(this,uiSeisMMProc,jrpSel) );
    jrppolselfld->attach( alignedBelow, nicefld );
    if ( avmachfld ) jrppolselfld->setPrefWidthInChar( hostnmwdth );
    jrpworklbl = new uiLabel( jrppolgrp, "Processes" );
    jrpworklbl->attach( rightOf, jrppolselfld );

    const char* envstr = GetEnvVar( "DTECT_STOP_OFFICEHOURS" );
    jrpstartfld = new uiGenInput( jrppolgrp, "", envstr ? envstr : "18:00" );
    jrpstartfld->attach( rightOf, jrppolselfld );

    envstr = GetEnvVar( "DTECT_START_OFFICEHOURS" );
    jrpstopfld = new uiGenInput( jrppolgrp, "and", envstr ? envstr : "7:30" );
    jrpstopfld->attach( rightOf, jrpstartfld );

    jrppolgrp->setHAlignObj( nicefld );
    jrppolgrp->attach( ensureBelow, machgrp );

    sep = new uiSeparator( this, "Hor sep 2", true );
    sep->attach( stretchedBelow, jrppolgrp );

    progrfld = new uiTextEdit( this, "Processing progress", true );
    progrfld->attach( ensureBelow, sep );
    progrfld->attach( widthSameAs, sep );
    progrfld->setPrefHeightInChar( 7 );

    progbar = new uiProgressBar( this, "", 1, 0 );
    progbar->attach( widthSameAs, progrfld );
    progbar->attach( alignedBelow, progrfld );

    postFinalise().notify( mCB(this,uiSeisMMProc,initWin) );
}


uiSeisMMProc::~uiSeisMMProc()
{
    delete logvwer;
    delete jobprov;
    delete jobrunner;
    delete outioobjinfo;
    delete timer;

    delete &hdl;
    delete &iop;
}


void uiSeisMMProc::initWin( CallBacker* cb )
{
    jrpSel( cb );
    if ( !avmachfld && (is2d || isrestart) )
	addPush(cb);
}


void uiSeisMMProc::setNiceNess()
{
    if ( !jobrunner ) return;
    int v = nicefld->getIntValue();
    if ( v > 19 ) v = 19;
    if ( v < 0 ) v = 0;
    jobrunner->setNiceNess( v );
}


#define mErrRet(s) { uiMSG().error(s); return; }

void uiSeisMMProc::startWork( CallBacker* )
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


    timer = new Timer("uiSeisMMProc timer");
    timer->tick.notify( mCB(this,uiSeisMMProc,doCycle) );
    timer->start( 100, true );
}

#define mMMKey			"MultiMachine"
#define mNrInlPerJobSettKey	"Nr inline per job"
#define mNrInlPerJobProcKey	"Nr of Inlines per Job"

int uiSeisMMProc::defltNrInlPerJob( const IOPar& inputpar )
{
    static int nr_inl_job = -1;

    inputpar.get( mNrInlPerJobProcKey, nr_inl_job );

    if ( nr_inl_job <= 0 )
    {
	IOPar* iopar = Settings::common().subselect( mMMKey );
	if ( !iopar ) iopar = new IOPar;

	bool insettings = iopar->get( mNrInlPerJobSettKey, nr_inl_job );

	if ( !insettings )
	{
	    nr_inl_job = 3;
	    iopar->set( mNrInlPerJobSettKey, nr_inl_job );

	    Settings::common().mergeComp( *iopar, mMMKey );
	    Settings::common().write();
	} 
    } 

    return nr_inl_job;
}


void uiSeisMMProc::mkJobRunner( int nr_inl_job )
{
    if ( jobrunner ) delete jobrunner;

    jobrunner = jobprov->getRunner( nr_inl_job );
    if ( jobprov->errMsg() )
    {
	delete jobrunner; jobrunner = 0;
	mErrRet(jobprov->errMsg())
    }

    jobrunner->setFirstPort( hdl.firstPort() );
    jobrunner->setRshComm( hdl.rshComm() );
    jobrunner->setNiceNess( hdl.defNiceLevel() );

    jobrunner->preJobStart.notify( mCB(this,uiSeisMMProc,jobPrepare) );
    jobrunner->postJobStart.notify( mCB(this,uiSeisMMProc,jobStarted) );
    jobrunner->jobFailed.notify( mCB(this,uiSeisMMProc,jobFailed) );
    jobrunner->msgAvail.notify( mCB(this,uiSeisMMProc,infoMsgAvail) );
}


static int getSecs( const char* txt )
{
    if ( !txt || !*txt ) return 0;
    BufferString bs( txt );
    char* mid = strchr( bs.buf(), ':' );
    if ( mid ) *mid++ = '\0';

    int secs=-1;
    if ( mid && *mid )
    {
	char* head = mid-1;
	while ( *head != ' ' && head > bs.buf() ) head--;

	char* tail = strchr( mid, ':' );
	if ( tail ) *tail++ = '\0';

	secs = toInt( head ) * 3600;
	secs += toInt( mid ) * 60;

	if( tail )
	    secs += toInt( tail );
    }   
    return secs;
}


void uiSeisMMProc::doCycle( CallBacker* )
{
    nrcyclesdone++;

    jobrunner->nextStep();

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


void uiSeisMMProc::updateAliveDisp()
{
    static const int nrdispstrs = 6;
    static const char* dispstrs[]
	= { ">..", ".>.", "..>", "..<", ".<.", "<.." };
    statusBar()->message( dispstrs[ nrcyclesdone % nrdispstrs ], 3 );

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
void uiSeisMMProc::updateCurMachs()
{
    BufferStringSet machs;
    jobrunner->showMachStatus( machs );
    sort( machs );

    const int oldsz = usedmachfld->box()->size(); 
    const int newsz = machs.size();

    int curit = oldsz ? usedmachfld->box()->currentItem() : -1;
    usedmachfld->box()->setEmpty();
    if ( newsz )
    {
	usedmachfld->box()->addItems( machs );
	if ( curit >= usedmachfld->box()->size() )
	    curit = usedmachfld->box()->size() - 1;
	usedmachfld->box()->setCurrentItem(curit);
    }
    else
	usedmachfld->box()->clearSelection();

    mReturn();
}


int uiSeisMMProc::runnerHostIdx( const char* mach ) const
{
    if ( !jobrunner || !mach || !*mach ) return -1;

    const ObjectSet<HostNFailInfo>& hi = jobrunner->hostInfo();
    for ( int idx=0; idx<hi.size(); idx++ )
    {
	if ( hi[idx]->hostdata_.isKnownAs(mach) )
	    return idx;
    }
    return -1;
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


void uiSeisMMProc::addPush( CallBacker* )
{
    uiListBox* lb = avmachfld ? avmachfld->box() : 0;
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

	BufferString hnm = lb ? lb->textOfItem( idx ) : hdl[0]->name();
	char* ptr = strchr( hnm.buf(), ' ' );
	if ( ptr ) *ptr = '\0';

	const HostData* hd = hdl.find( hnm.buf() );
	if ( !hd ) { pErrMsg("Huh"); continue; }

#ifndef __win__
	BufferString errmsg;
	if ( !hd->isKnownAs(HostData::localHostName())
		&& !isHostOK(*hd,hdl.rshComm(),errmsg) )
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
	    progrfld->append( msg );
	}
    }
}


const char* uiSeisMMProc::curUsedMachName()
{
   static BufferString mach;
   mach = usedmachfld->box()->getText();

   char* ptr = strstr( mach.buf(), " -:- ");
   if ( ptr ) *ptr='\0';

   return mach;
}


void uiSeisMMProc::stopPush( CallBacker* )
{
    int rhidx = runnerHostIdx( curUsedMachName() );
    if ( rhidx >= 0 )
	jobrunner->removeHost( rhidx );
}


void uiSeisMMProc::vwLogPush( CallBacker* )
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


void uiSeisMMProc::jobPrepare( CallBacker* cb )
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


void uiSeisMMProc::jobStarted( CallBacker* cb )
{
    const JobInfo& ji = jobrunner->curJobInfo();
    BufferString msg( "Started processing " );
    addObjNm( msg, jobrunner, ji.descnr_ );
    if ( ji.hostdata_ )
	{ msg += " on "; msg += ji.hostdata_->name(); }
    progrfld->append( msg );
}


void uiSeisMMProc::jobFailed( CallBacker* cb )
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


void uiSeisMMProc::infoMsgAvail( CallBacker* cb )
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


void uiSeisMMProc::jrpSel( CallBacker* )
{
    const bool isgo = *jrppolselfld->text() == 'G';
    jrpstartfld->display( isgo );
    jrpstopfld->display( isgo );
    jrpworklbl->display( !isgo );
}


void uiSeisMMProc::pauseJobs()
{
    if ( !jobrunner ) return;

    const char* txt = jrppolselfld->text();
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
	Threads::sleep( 1 );
	removed = jp->removeTempSeis(); 
    }

    if ( !removed )
	ErrMsg( "Could not remove all temporary seismics" );
}


bool uiSeisMMProc::readyForPostProcess()
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


bool uiSeisMMProc::wrapUp( bool force )
{
    if ( !force && !readyForPostProcess() )
	return false;

    Executor* exec = jobprov ? jobprov->getPostProcessor() : 0;
    if ( exec )
    {
	uiTaskRunner uitr( this );
	const bool res = uitr.execute( *exec );
	delete exec;
	if ( !res )
	    return false;
    }   

    progrfld->append( "Processing completed" );
    setCtrlStyle( LeaveOnly );
    button(uiDialog::CANCEL)->display(false);
    rmTmpSeis( jobprov );
    delete jobrunner; jobrunner = 0;
    return true;
}


bool uiSeisMMProc::rejectOK( CallBacker* )
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


bool uiSeisMMProc::acceptOK(CallBacker*)
{
    if ( !outioobjinfo->ioObj() || !jobrunner ) return true;

    if ( usedmachfld->box()->size() && !uiMSG().askGoOn(
	    "This will stop further processing and wrap up",false) )
	return false;
    
    jobrunner->stopAll();
    return wrapUp( true );
}
