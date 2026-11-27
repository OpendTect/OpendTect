/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmain.h"

#include "applicationdata.h"

#include "uiactiverunningproc.h"
#include "uiattribpartserv.h"
#include "uicolortable.h"
#include "uidockwin.h"
#include "uiempartserv.h"
#include "uiglinfo.h"
#include "uiioobjseldlg.h"
#include "uimain.h"
#include "uimpepartserv.h"
#include "uimsg.h"
#include "uinlapartserv.h"
#include "uinotfinisheddlg.h"
#include "uinotsaveddlg.h"
#include "uiodapplmgr.h"
#include "uiodmenumgr.h"
#include "uiodscenemgr.h"
#include "uiodviewer2dmgr.h"
#include "uipixmap.h"
#include "uiseispartserv.h"
#include "uiserviceclientmgr.h"
#include "uisplashscreen.h"
#include "uistatusbar.h"
#include "uisurvey.h"
#include "uisurvinfoed.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"

#include "commandlaunchmgr.h"
#include "commandlineparser.h"
#include "ctxtioobj.h"
#include "envvars.h"
#include "genc.h"
#include "ioman.h"
#include "ioobj.h"
#include "moddepmgr.h"
#include "mousecursor.h"
#include "nrbytes2string.h"
#include "od_helpids.h"
#include "oddirs.h"
#include "odinst.h"
#include "odplatform.h"
#include "odsession.h"
#include "odsysmem.h"
#include "odver.h"
#include "plugins.h"
#include "ptrman.h"
#include "settings.h"
#include "survgeom.h"
#include "survinfo.h"
#include "systeminfo.h"
#include "timefun.h"
#include "timer.h"
#include "visdata.h"

#include <QTimer>


extern "C"
{
    mGlobal(uiTools) bool doProductSelection(bool&,uiDialog*&,uiRetVal&);
}


extern "C++"
{
    mGlobal(uiODMain) void ODMain(std::unique_ptr<uiDialog>&,
				  std::unique_ptr<uiODMain>&);
}


static uiODMain* manODMainWin( uiODMain* i, bool set )
{
    mDefineStaticLocalObject( uiODMain*, theinst, = nullptr );
    if ( set )
	theinst = i;
    return theinst;
}


uiODMain* ODMainWin()
{
    return manODMainWin( nullptr, false );
}


static const char* sKeyShowLowRes = "UI.LowRes.Show";
static const char* sKeyShowSubRes = "Ui.SubRes.Show";
static const int cScreenLowRes = 768;
static const int cScreenSubRes = 920;

static void checkScreenRes()
{
    uiMain& uimain = uiMain::instance();
    const int nrscreens = uimain.nrScreens();

    bool anyacceptable = false, anysubstd = false, anyok = false,
	 needscale = false;
    for ( int idx=0; idx<nrscreens; idx++ )
    {
	const uiSize sz( uimain.getScreenSize(idx,true) );
	const double devpixrat = uimain.getDevicePixelRatio(idx);
	if ( sz.height() < cScreenLowRes )
	{
	    anysubstd = true;
	    if ( !mIsUdf(devpixrat) && devpixrat > 1.1 )
		needscale = true;
	    continue;
	}

	anyacceptable = true;
	if ( sz.height() < cScreenSubRes )
	{
	    if ( !mIsUdf(devpixrat) && devpixrat > 1.1 )
		needscale = true;
	    anysubstd = true;
	}
	else
	    anyok = true;
    }

    Settings& setts = Settings::common();
    bool dontshowagain = false;
    const uiString& es = uiString::emptyString();
    if ( !anyacceptable )
    {
	if ( !setts.isFalse(sKeyShowLowRes) )
	    dontshowagain = needscale
		? uiMSG().error( od_static_tr( "checkScreenRes",
		    "Your display scale factor is set too high."
		    "\nYou can probably not use OpendTect properly."
		    "\nYou can set the display scale lower in the"
		    "\nOperating System display settings." ),
		    es, es, true )
		: uiMSG().error( od_static_tr( "checkScreenRes",
		"Your vertical screen resolution is lower than %1 pixels."
		"\nYou can probably not use OpendTect properly.")
		    .arg( cScreenLowRes ), es, es, true );
    }
    else if ( !anyok )
    {
	if ( !setts.isFalse(sKeyShowSubRes) )
	    dontshowagain = needscale
	    ? uiMSG().warning( od_static_tr( "checkScreenRes",
		"Your display scale factor is set too high."
		"\nOpendTect may be unusable without using small screen fonts."
		"\n\nYou can set font sizes in the 'General Settings' window,"
		"\nor menu Utilities-Settings-Look&Feel,"
		"\nor alternatively you can set the display scale lower"
		"\nin the Operating System display settings." ),
		es, es, true )
	    : uiMSG().warning( od_static_tr( "checkScreenRes",
	    "Your vertical screen resolution is lower than %1 pixels."
	    "\nOpendTect may be unusable without using small screen fonts."
	    "\n\nYou can set font sizes in the 'General Settings' window,"
	    "\nor menu Utilities-Settings-Look&Feel.")
		    .arg( cScreenSubRes ), es, es, true );
    }
    else if ( anysubstd )
    {
	if ( !setts.isFalse(sKeyShowSubRes) )
	    dontshowagain = needscale
	    ? uiMSG().warning( od_static_tr( "checkScreenRes",
		"Your display scale factor is set too high on one of "
		"your screens."
		"\nOpendTect may only be usable by making the screen "
		"display smaller."
		"\n\nYou can set font sizes in the 'General Settings' window,"
		"\nor menu Utilities-Settings-Look&Feel,"
		"or alternatively you can set the display scale lower"
		"in the Operating System display settings." ),
		es, es, true )
	    : uiMSG().warning( od_static_tr( "checkScreenRes",
	    "One of your screens has a vertical screen resolution < %1 "
	    "pixels.\nOpendTect may only be usable by making the screen "
	    "fonts smaller."
	    "\n\nYou can set font sizes in the 'General Settings' window,"
	    "\nor menu Utilities-Settings-Look&Feel.")
		    .arg( cScreenSubRes ), es, es, true );
    }

    if ( dontshowagain )
    {
	const char* ky = !anyacceptable ? sKeyShowLowRes : sKeyShowSubRes;
	setts.setYN( ky, false );
	setts.write();
    }
}


void ODMainProgramRestarter()
{
    if ( ODMainWin() )
	ODMainWin()->restart();
    else
	pFreeFnErrMsg("No ODMainWin(). Cannot restart");
}


mExternCPP(uiODMain) void ODMain( std::unique_ptr<uiDialog>& prodseldlgman,
				  std::unique_ptr<uiODMain>& odmain )
{
    OD::ModDeps().ensureLoaded( "AllNonUi" );
    OD::ModDeps().ensureLoaded( "uiIo" );

    uiDialog::setTitlePos( uiDialog::LeftSide );

    auto closedhandler = std::make_shared<LambdaCallBacker>();
    auto starthandler = std::make_shared<LambdaCallBacker>();
    auto prodselnotif = std::make_shared<Notifier<uiDialog>>( nullptr );
    auto skippluginsel = std::make_shared<bool>( false );
    QTimer::singleShot(0, [&prodseldlgman,closedhandler,starthandler,
			   prodselnotif,skippluginsel]
    {
	uiMain& app = uiMain::instance();
	const CommandLineParser clp;
	uiRetVal uirv = IOMan::setDataSource( clp );
	mIfIOMNotOK( { app.exit(1); return; } )

	checkScreenRes();
	(*skippluginsel) = false;
	uiDialog* prodseldlg = nullptr;
	uirv.setOK();
	if ( !doProductSelection((*skippluginsel),prodseldlg,uirv) )
	{
	    closeAndNullPtr( prodseldlg );
	    if ( !uirv.isOK() )
		uiMSG().error( uirv );

	    app.exit(1);
	    return;
	}

	if ( prodseldlg && !(*skippluginsel) )
	{
	    prodseldlgman.reset( prodseldlg );
	    prodseldlg->setDeleteOnClose( false );
	    closedhandler->set( [&prodseldlgman,closedhandler,
				 starthandler,prodselnotif]()
	    {
		uiDialog* seldlg = prodseldlgman.get();
		const bool accepted = seldlg->uiResult() == uiDialog::Accepted;
		closedhandler->detachCB( seldlg->windowClosed );
		seldlg->close();
		if ( accepted )
		    prodselnotif->trigger();
		else
		    uiMain::instance().exit(1);

		return;
	    });

	    closedhandler->attachCB( prodseldlg->windowClosed );
	    prodseldlg->show();
	}
	else
	{
	    prodselnotif->trigger();
	}
    });

    starthandler->set( [&,starthandler,prodselnotif,skippluginsel]()
    {
	starthandler->detachCB( *prodselnotif.get() );
	prodseldlgman.reset( nullptr );

	SetProgramRestarter( ODMainProgramRestarter );
	const BufferString pmfnm = GetSWSetupShareFileName( "splash.png" );
	const uiPixmap pixmap( pmfnm.buf() );
	uiSplashScreen splash( pixmap );
	splash.show();
	uiMain::instance().processEvents();
	splash.showMessage( "Loading plugins ..." );

	PIM().loadAuto( false, !(*skippluginsel) );
	OD::ModDeps().ensureLoaded( "uiODMain" );
	odmain = std::make_unique<uiODMain>( uiMain::instance() );
	manODMainWin( odmain.get(), true );

	PIM().loadAuto( true, !(*skippluginsel) );

	splash.showMessage( "Initializing Scene ..." );
	odmain->initScene();
	odmain->setActivateOnFirstShow();
	odmain->go();
	odmain->updateCaption();
	splash.finish( odmain.get() );
    });

    starthandler->attachCB( *prodselnotif.get() );
}


#define mMemStatusFld 4
#define mTransientStatusFld 0

static od_int64 sMemFldStatusUntil = 0;
static od_int64 sTransientStatusUntil = 0;

static void setMemFldStatusMsg( uiStatusBar* sb, const uiString& msg )
{
    if ( !sb )
	return;

    sMemFldStatusUntil = Time::getMilliSeconds() + 5000;
    sb->message( msg, mMemStatusFld );
}


void uiODMain::setTransientStatusMsg( const uiString& msg )
{
    sTransientStatusUntil = Time::getMilliSeconds() + 5000;
    statusBar()->message( msg, mTransientStatusFld );
}


bool uiODMain::hasTransientStatusMsg() const
{
    return Time::getMilliSeconds() < sTransientStatusUntil;
}


uiODSessionController::~uiODSessionController()
{}


bool uiODSessionController::confirmClose( bool, bool& accept )
{
    accept = false;
    return false;
}


void uiODSessionController::adjustRestart( bool, bool&, bool& )
{}


void uiODSessionController::writeRestartState( bool, bool,
						const IOPar&, const IOPar& )
{}


bool uiODSessionController::restoreRestart()
{
    return false;
}


bool uiODSessionController::willOfferStartupSession() const
{
    return false;
}


bool uiODSessionController::offerStartupSession()
{
    return false;
}


void uiODSessionController::afterStartupSession( bool )
{}

uiODMain::uiODMain( uiMain& a )
    : uiMainWin(nullptr,toUiString("OpendTect Main Window"),5,true)
    , sessionSave(this)
    , sessionRestoreEarly(this)
    , sessionRestore(this)
    , beforeExit(this)
    , justBeforeGo(this)
    , uiapp_(a)
    , lastsession_(*new ODSession)
    , programname_(ApplicationData::applicationName())
    , sesstimer_(*new Timer("Session restore timer"))
    , memtimer_(*new Timer("Memory display timer"))
    , newsurvinittimer_(*new Timer("New survey init timer"))
    , autoloadsessiontimer_(*new Timer("Auto-load Session"))
{
    setIconText( getProgramString() );
    uiapp_.setTopLevel( this );

    applmgr_ = new uiODApplMgr( *this );
    if ( buildUI() )
	failed_ = false;

    mAttachCB( IOM().afterSurveyChange, uiODMain::afterSurveyChgCB );
    mAttachCB( sesstimer_.tick, uiODMain::sessTimerCB );

    uiString statustt = tr( "System memory: Free/Available" );
    statustt.append( tr("| CPU: Used/Available") );
    statusBar()->setToolTip( mMemStatusFld, statustt );
    statusBar()->setTxtAlign( mMemStatusFld, Alignment::HCenter );
    mAttachCB( memtimer_.tick, uiODMain::memTimerCB );
    memtimer_.start( 1000 );

    if ( uiSurvey::lastSurveyState() == uiSurvey::NewFresh )
    {
	neednewsurvinit_ = true;
	newsurvinittimer_.start( 200, true );
	mAttachCB( newsurvinittimer_.tick, uiODMain::newSurvInitTimerCB );
    }

    mAttachCB( postFinalize(), uiODMain::afterStartupCB );
}


uiODMain::~uiODMain()
{
    detachAllNotifiers();
    memtimer_.stop();
    if ( ODMainWin()==this )
	manODMainWin( nullptr, true );

    delete &lastsession_;
    delete &sesstimer_;
    delete &memtimer_;
    delete &newsurvinittimer_;
    delete &autoloadsessiontimer_;

    delete menumgr_;
    delete scenemgr_;
    delete viewer2dmgr_;
    delete applmgr_;
}


bool uiODMain::buildUI()
{
    scenemgr_ = new uiODSceneMgr( this );
    viewer2dmgr_ = new uiODViewer2DMgr( this );
    menumgr_ = new uiODMenuMgr( this );
    menumgr_->initSceneMgrDepObjs( applmgr_, scenemgr_ );

    auto* tb = new uiColorTableToolBar( this );
    ctabtb_ = tb;
    ctabed_ = new uiVisColTabEd( *tb );
    ctabed_->seqChange().notify( mCB(applmgr_,uiODApplMgr,colSeqChg) );
    ctabed_->mapperChange().notify( mCB(applmgr_,uiODApplMgr,colMapperChg));

    return true;
}


void uiODMain::initScene()
{
    menumgr_->fillSceneMenu();
    const bool addscene = !GetEnvVarYN( "OD_NOSCENE_AT_STARTUP" );
    if ( addscene )
	scenemgr_->initMenuMgrDepObjs();

    readSettings();

    justBeforeGo.trigger();
}


IOPar& uiODMain::sessionPars()
{
    return cursession_->pluginpars();
}


CtxtIOObj* uiODMain::getUserSessionIOData( bool restore )
{
    CtxtIOObj* ctio = mMkCtxtIOObj(ODSession);
    ctio->ctxt_.forread_ = restore;
    ctio->setObj( cursessid_ );
    uiIOObjSelDlg dlg( this, *ctio );
    dlg.setHelpKey( mODHelpKey(mSessionSaveRestoreHelpID) );
    if ( !dlg.go() )
    {
	delete ctio->ioobj_;
	deleteAndNullPtr( ctio );
    }
    else
    {
	delete ctio->ioobj_;
	ctio->ioobj_ = dlg.ioObj()->clone();
	const MultiID id = ctio->ioobj_ ? ctio->ioobj_->key() : MultiID::udf();
	cursessid_ = id;
    }

    return ctio;
}


static bool hasSceneItems( uiVisPartServer* visserv )
{
    TypeSet<SceneID> sceneids;
    visserv->getSceneIds( sceneids );
    if ( sceneids.isEmpty() )
	return false;

    int nrchildren = 0;
    for ( const auto& sceneid : sceneids )
    {
	TypeSet<VisID> visids;
	visserv->getSceneChildIds( sceneid, visids );
	nrchildren += visids.size();
    }

    return nrchildren>sceneids.size()*3;
}


bool uiODMain::hasSessionChanged()
{
    if ( !hasSceneItems(applMgr().visServer()) )
	return false;

    ODSession sess;
    cursession_ = &sess;
    updateSession();
    cursession_ = &lastsession_;
    return !( sess == lastsession_ );
}


#define mDelCtioRet()	{ delete ctio->ioobj_; delete ctio; return; }

bool uiODMain::saveSessionDlg()
{
    CtxtIOObj* ctio = getUserSessionIOData( false );
    if ( !ctio )
	return false;

    ODSession sess;
    cursession_ = &sess;
    uiString errmsg;
    const bool ok = updateSession() &&
	ODSessionTranslator::store( sess, ctio->ioobj_, errmsg );
    if ( ok )
    {
	lastsession_ = sess;
	cursession_ = &lastsession_;
	setMemFldStatusMsg( statusBar(), tr("Session saved") );
    }
    else
    {
	cursession_ = &lastsession_;
	if ( !errmsg.isEmpty() )
	    uiMSG().error( errmsg );
    }

    delete ctio->ioobj_;
    delete ctio;
    return ok;
}


void uiODMain::saveSession()
{
    saveSessionDlg();
}


void uiODMain::setSessionController( uiODSessionController* ctl )
{
    sessionctl_ = ctl;
}


bool uiODMain::captureSession( ODSession& sess )
{
    ODSession* prev = cursession_;
    cursession_ = &sess;
    const bool ok = updateSession();
    cursession_ = prev;
    return ok;
}


void uiODMain::noteSessionStored( const ODSession& sess )
{
    lastsession_ = sess;
    cursession_ = &lastsession_;
}


void uiODMain::setCurrentSessionID( const MultiID& mid )
{
    cursessid_ = mid;
}


bool uiODMain::restoreSession( const MultiID& mid )
{
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
	return false;

    cursessid_ = mid;
    restoreSession( ioobj.ptr() );
    return true;
}


bool uiODMain::willOfferStartupSession() const
{
    return sessionctl_ && sessionctl_->willOfferStartupSession();
}


bool uiODMain::canStoreSession()
{
    if ( !cursessid_.isUdf() )
	return true;

    uiVisPartServer* visserv = applMgr().visServer();
    return visserv && hasSceneItems( visserv );
}


void uiODMain::restoreSession()
{
    CtxtIOObj* ctio = getUserSessionIOData( true );
    if ( !ctio )
	return;

    restoreSession( ctio->ioobj_ );
    mDelCtioRet()
}




void uiODMain::restoreSession( const IOObj* ioobj )
{
    ODSession sess;
    uiString bs;
    if ( !ODSessionTranslator::retrieve(sess,ioobj,bs) )
    {
	uiMSG().error( bs );
	return;
    }

    cursession_ = &sess;
    doRestoreSession();
    cursession_ = &lastsession_;
    lastsession_.clear();
    sesstimer_.start( 200, true );
    sceneMgr().setToViewMode( true );
    sceneMgr().updateTrees();
}


bool uiODMain::updateSession()
{
    cursession_->clear();
    applMgr().EMServer()->fillPar( cursession_->empars() );
    applMgr().seisServer()->fillPar( cursession_->seispars() );
    applMgr().visServer()->fillPar( cursession_->vispars() );
    applMgr().attrServer()->fillPar( cursession_->attrpars(true,false),
				     true, false );
    applMgr().attrServer()->fillPar( cursession_->attrpars(true, true),
				     true, true );
    applMgr().attrServer()->fillPar( cursession_->attrpars(false, false),
				     false, false );
    applMgr().attrServer()->fillPar( cursession_->attrpars(false, true),
				     false, true );
    sceneMgr().getScenePars( cursession_->scenepars() );
    if ( applMgr().nlaServer()
      && !applMgr().nlaServer()->fillPar( cursession_->nlapars() ) )
	return false;

    applMgr().mpeServer()->fillPar( cursession_->mpepars() );
    viewer2DMgr().fillPar( cursession_->vwr2dpars() );

    sessionSave.trigger();
    return true;
}


void uiODMain::doRestoreSession()
{
    MouseCursorManager::setOverride( MouseCursor::Wait );
    sceneMgr().cleanUp( false );
    applMgr().resetServers();
    restoringsess_ = true;
    restoremsgs_.setEmpty();

    sessionRestoreEarly.trigger();
    applMgr().EMServer()->usePar( cursession_->empars() );
    applMgr().seisServer()->usePar( cursession_->seispars() );
    if ( applMgr().nlaServer() )
	applMgr().nlaServer()->usePar( cursession_->nlapars() );

    if ( SI().has2D() )
    {
	applMgr().attrServer()->usePar( cursession_->attrpars(true,false),
					true, false, restoremsgs_ );
	applMgr().attrServer()->usePar( cursession_->attrpars(true,true),
					true, true, restoremsgs_ );
    }

    if ( SI().has3D() )
    {
	applMgr().attrServer()->usePar( cursession_->attrpars(false,false),
					false, false, restoremsgs_ );
	applMgr().attrServer()->usePar( cursession_->attrpars(false,true),
					false, true, restoremsgs_ );
    }

    const bool visok = applMgr().visServer()->usePar( cursession_->vispars() );
    if ( visok )
    {
	applMgr().mpeServer()->usePar( cursession_->mpepars() );
	sceneMgr().useScenePars( cursession_->scenepars() );
    }

    sessionRestore.trigger();
    if ( visok )
	applMgr().visServer()->calculateAllAttribs();
    else
    {
	MouseCursorManager::restoreOverride();
	uiMSG().error( tr("An error occurred while reading session file.\n"
			  "A new scene will be launched") );
	MouseCursorManager::setOverride( MouseCursor::Wait );
	sceneMgr().cleanUp( true );
    }

    if ( visok )
	viewer2DMgr().usePar( cursession_->vwr2dpars() );

    restoringsess_ = false;
    MouseCursorManager::restoreOverride();
    if ( !restoremsgs_.isEmpty() )
    {
	uiString basemsg = tr("Errors during session restore");
	uiMSG().errorWithDetails( restoremsgs_, basemsg );
    }
}


void uiODMain::sessTimerCB( CallBacker* )
{
    sceneMgr().layoutScenes();
}


void uiODMain::autoloadSessionCB( CallBacker* )
{
    static bool checkedrestart = false;
    if ( !checkedrestart )
    {
	checkedrestart = true;
	if ( sessionctl_ && sessionctl_->restoreRestart() )
	    return;
    }

    const bool declined = sessionctl_ ? sessionctl_->offerStartupSession()
				      : false;
    if ( sessionctl_ )
	sessionctl_->afterStartupSession( declined );
}




void uiODMain::afterStartupCB( CallBacker* )
{
    uiServiceClientMgr::setFor( *this );
    const ODInst::AutoInstType ait = ODInst::getAutoInstType();
    if ( ait != ODInst::AutoInstType::NoAuto )
	checkUpdateAvailable();
}


void uiODMain::newSurvInitTimerCB( CallBacker* )
{
    mDetachCB( newsurvinittimer_.tick, uiODMain::newSurvInitTimerCB );
    if ( neednewsurvinit_ )
	applMgr().setZStretch();
}


void uiODMain::translateText()
{
    uiMainWin::translateText();
    applMgr().sceneMgr().translateText();
}


void uiODMain::memTimerCB( CallBacker* )
{
    mDefineStaticLocalObject( Threads::Lock, memtimerlock, (true) );
    Threads::Locker locker( memtimerlock, Threads::Locker::DontWaitForLock );
    if ( !locker.isLocked() )
	return;

    if ( Time::getMilliSeconds() < sMemFldStatusUntil )
	return;

    od_int64 tot, free;
    OD::getSystemMemory( tot, free );
    NrBytesToStringCreator converter;
    converter.setUnitFrom( tot );
    uiString txt = tr( "[free mem] %1/%2 | [cpu] %3/%4" );

    //Use separate calls to avoid the reuse of the converter's buffer
    txt.arg( converter.getString(free,1,false) );
    txt.arg( converter.getString(tot,1,true) );

    const int systemnrcpus = Threads::getSystemNrProcessors();
    const int odnrcpus = Threads::getNrProcessors();
    txt.arg( odnrcpus ).arg( systemnrcpus );
    statusBar()->message( txt, mMemStatusFld );
}


void uiODMain::go()
{
    if ( failed_ )
    {
	uiapp_.exit(1);
	return;
    }

    show();
    if ( uiMain::reqOpenGL() )
    {
        IOPar& graphicspar = const_cast<IOPar&>( System::graphicsInformation());
        uiGLI().createAndShowMessage( true, &graphicspar, "dTect.Last GL info");
    }

    mAttachCB( autoloadsessiontimer_.tick, uiODMain::autoloadSessionCB );
    autoloadsessiontimer_.start( 500, true );
}


void uiODMain::setProgramName( const char* nm )
{
    programname_ = nm;
    setIconText( getProgramString() );
    updateCaption();
}


void uiODMain::setProgInfo( const char* info )
{
    programinfo_ = info;
    updateCaption();
}


bool uiODMain::askStore( bool& askedanything, const uiString& actiontype )
{
    if ( !applmgr_->attrServer() )
	return false;

    if ( !NotSavedPrompter::NSP().doTrigger(uiMainWin::activeWindow(), true,
								actiontype) )
		return false;

    if ( !NotFinishedPrompter::NFP().doTrigger(uiMainWin::activeWindow(), true,
								actiontype) )
	    return false;

    bool doask = false;
    Settings::common().getYN( "dTect.Ask store session", doask );
    if ( doask && hasSessionChanged() )
    {
	askedanything = true;
	const int res =
	    uiMSG().askSave( tr("Do you want to save this session?"), true );
	if ( res == 1 )
	    saveSession();
	else if ( res == -1 )
	    return false;
    }

    if ( SI().has2D() && !askStoreAttribs( true, askedanything ) )
	return false;

    if ( SI().has3D() && !askStoreAttribs( false, askedanything ) )
	return false;

    return true;
}


bool uiODMain::askStoreAttribs( bool is2d, bool& askedanything )
{
    bool doask = true;
    Settings::common().getYN( "dTect.Ask store attributeset", doask );
    if ( doask && !applmgr_->attrServer()->setSaved( is2d ) )
    {
	askedanything = true;
	const int res = uiMSG().askSave( tr("Current attribute set has changed."
					    "\nStore the changes now?") );
	if ( res == 1 )
	    applmgr_->attrServer()->saveSet( is2d );
	else if ( res == -1 )
	    return false;
    }

    return true;
}


void uiODMain::afterSurveyChgCB( CallBacker* )
{
    updateCaption();
    cursessid_.setUdf();
    autoloadsessiontimer_.start( 500, true );
}


void uiODMain::checkUpdateAvailable()
{
    const BufferString odver = GetFullODVersion();
    if ( odver.isEmpty() || odver.contains("development") )
	return;

    CallBack cb( mCB(this,uiODMain,updateStatusCB) );
    ODInst::startUpdateCheck( cb );
}


void uiODMain::updateStatusCB( CallBacker* cb )
{
    const auto* ct = Threads::CommandLaunchMgr::getMgr().getCommandTask( cb );
    if ( !ct )
	return;

    const int exitcode = ct->getExitCode();
    if ( exitcode !=0 && exitcode != 2 )
    {
	ODInst::updatesAvailable( 0 ); // actual status is unknown
	const BufferString stderrstr = ct->getStdError();
	if ( !stderrstr.isEmpty() )
	    ErrMsg( BufferString("Error checking for software updates: ",
				 stderrstr) );
    }
    else
	ODInst::updatesAvailable( exitcode == 2 ? 1 : 0);

    updateCaptionCB( nullptr );
}


void uiODMain::updateCaptionCB( CallBacker* )
{
    mEnsureExecutedInMainThread( uiODMain::updateCaptionCB );
    updateCaption();
}


void uiODMain::updateCaption()
{
    uiString capt = toUiString( "%1 (%2)" )
			.arg( getProgramString() )
			.arg( OD::Platform::local().osName() );

    if ( ODInst::getAutoInstType() == ODInst::AutoInstType::InformOnly &&
	 ODInst::updatesAvailable() == 1 )
	capt.append( tr(" *UPDATE AVAILABLE*") );

    const BufferString usr( GetSoftwareUser() );
    if ( !usr.isEmpty() )
	capt.append( tr(" [%1] ").arg( usr ) );

    if ( !SI().name().isEmpty() )
	capt.append( " : %1" ).arg( SI().name() );

    if ( !programinfo_.isEmpty() )
	capt.append( " [%1]" ).arg( programinfo_ );

    setCaption( capt );
}


bool uiODMain::closeOK( bool withinteraction, bool doconfirm )
{
    saveSettings();

    const uiString actstr = restarting_ ? uiStrings::sRestart()
					: uiStrings::sClose();
    bool closeactivebatch = true;
    if ( !restarting_ )
	closeactivebatch = ActiveProcPrompter::APP().doTrigger(
						uiMainWin::activeWindow() );

    if ( !closeactivebatch || doconfirm )
    {
	bool handled = false;
	bool accept = false;
	const bool nosession = !restarting_ && cursessid_.isUdf();
	if ( nosession && sessionctl_ )
	    handled = sessionctl_->confirmClose( restarting_, accept );

	if ( handled )
	{
	    if ( !accept )
		return false;
	}
	else if ( !uiMSG().askGoOn(tr("Do you want to %1 %2?")
				    .arg(restarting_ ? "restart" : "close")
				    .arg(programname_),
		  actstr, uiStrings::sCancel()) )
	    return false;
    }

     //This bool is not required. Keeping to maintain API.
     bool dummy = false;
     if ( withinteraction )
     {
	 if ( !askStore(dummy,
	      uiStrings::phrJoinStrings(actstr,toUiString(programname_))) )
	 {
	     uiMSG().message( restarting_ ? tr("Restart cancelled")
					  : tr("Closing cancelled") );
	     return false;
	 }
     }


     //Clean up all scenes manually.
     // If QT closes it down, it will trigger new rendering,
     //which required GL context, which is only partial.
     if ( scenemgr_ )
        scenemgr_->cleanUp(false);

     beforeExit.trigger();

     if ( failed_ )
	return true;

     return true;
}


uiString uiODMain::getProgramString() const
{
    return toUiString( "%1 V%2" ).arg( programname_ ).arg( GetFullODVersion() );
}


uiString uiODMain::getProgramName()
{
    return toUiString(programname_);
}


bool uiODMain::prepareRestart( bool withinteraction, bool doconfirm )
{
    IOPar seispar;
    IOPar horpar;
    if ( applMgr().seisServer() )
	applMgr().seisServer()->fillPar( seispar );

    if ( applMgr().EMServer() )
	applMgr().EMServer()->fillPar( horpar );

    const bool haspreload = seispar.size()!=0 || horpar.size()!=0;
    bool keepsession = !cursessid_.isUdf();
    bool keeppreload = haspreload;
    if ( sessionctl_ )
	sessionctl_->adjustRestart( withinteraction, keepsession, keeppreload );

    if ( !closeOK(withinteraction,doconfirm) )
	return false;

    if ( sessionctl_ )
	sessionctl_->writeRestartState( keepsession, keeppreload,
					seispar, horpar );

    return true;
}


void uiODMain::restart( bool withinteraction, bool doconfirm )
{
    restarting_ = true;
    if ( !withinteraction && !doconfirm )
	exit( false, false );
    else if ( !prepareRestart(withinteraction,doconfirm) )
    {
	restarting_ = false;
	return;
    }

    SetProgramRestarter( GetBasicProgramRestarter() );
    uiapp_.restart();

    // Re-start failed ...
    restarting_ = false;
    uiMSG().error(
	tr("Failed to restart, please close and launch again manually") );
}


void uiODMain::exit( bool withinteraction, bool doconfirm, int retcode )
{
    if ( withinteraction || doconfirm )
    {
	if ( !closeOK(withinteraction,doconfirm) )
	    return;
    }

    uiapp_.exit( retcode );
}


uiServiceClientMgr& uiODMain::serviceMgr()
{
    return uiServiceClientMgr::getMgr();
}


// uiPluginInitMgr
uiPluginInitMgr::uiPluginInitMgr()
    : appl_(*ODMainWin())
{
    mAttachCB( IOM().surveyToBeChanged, uiPluginInitMgr::beforeSurvChgCB );
    mAttachCB( IOM().afterSurveyChange, uiPluginInitMgr::afterSurvChgCB );
    if ( !ODMainWin() )
	return;

    mAttachCB( appl_.beforeExit, uiPluginInitMgr::applCloseCB );
    mAttachCB( appl_.menuMgr().dTectMnuChanged, uiPluginInitMgr::menuChgCB );
    mAttachCB( appl_.menuMgr().dTectTBChanged, uiPluginInitMgr::tbChgCB );
    mAttachCB( appl_.sceneMgr().treeAdded, uiPluginInitMgr::treeAddCB );
}


uiPluginInitMgr::~uiPluginInitMgr()
{
    detachAllNotifiers();
}


void uiPluginInitMgr::init()
{
    if ( ODMainWin() )
    {
	dTectMenuChanged();
	dTectToolbarChanged();
    }
}


void uiPluginInitMgr::beforeSurvChgCB( CallBacker* )
{ beforeSurveyChange(); }

void uiPluginInitMgr::afterSurvChgCB( CallBacker* )
{ afterSurveyChange(); }

void uiPluginInitMgr::applCloseCB( CallBacker* )
{ applicationClosing(); }

void uiPluginInitMgr::menuChgCB( CallBacker* )
{ dTectMenuChanged(); }

void uiPluginInitMgr::tbChgCB( CallBacker* )
{ dTectToolbarChanged(); }

void uiPluginInitMgr::treeAddCB( CallBacker* cb )
{
    mCBCapsuleUnpack(SceneID,sceneid,cb);
    treeAdded( sceneid );
}
