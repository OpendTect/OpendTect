/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmain.h"

#include "applicationdata.h"

#include "uiattribpartserv.h"
#include "uimain.h"
#include "uicolortable.h"
#include "uidockwin.h"
#include "uiempartserv.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uiioobjseldlg.h"
#include "uistatusbar.h"
#include "uilabel.h"
#include "uimpepartserv.h"
#include "uimsg.h"
#include "uinlapartserv.h"
#include "uinotsaveddlg.h"
#include "uiodapplmgr.h"
#include "uiodmenumgr.h"
#include "uiodscenemgr.h"
#include "uiodviewer2dmgr.h"
#include "uipixmap.h"
#include "uiseispartserv.h"
#include "uisetdatadir.h"
#include "uisplashscreen.h"
#include "uistrattreewin.h"
#include "uisurvey.h"
#include "uisurvinfoed.h"
#include "uitoolbar.h"
#include "ui2dsip.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"
#include "uiserviceclientmgr.h"

#include "coltabsequence.h"
#include "commandlaunchmgr.h"
#include "commandlineparser.h"
#include "ctxtioobj.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "ioman.h"
#include "ioobj.h"
#include "moddepmgr.h"
#include "mousecursor.h"
#include "nrbytes2string.h"
#include "oddirs.h"
#include "odinst.h"
#include "odplatform.h"
#include "odsession.h"
#include "odsysmem.h"
#include "odver.h"
#include "plugins.h"
#include "ptrman.h"
#include "settingsaccess.h"
#include "survgeom.h"
#include "survinfo.h"
#include "threadwork.h"
#include "timer.h"
#include "visdata.h"
#include "od_helpids.h"

#include <iostream>


extern "C" {

    mGlobal(uiTools) bool doProductSelection(bool&,uiRetVal&);

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
	{ pFreeFnErrMsg("No ODMainWin(). Cannot restart"); }
}


int ODMain( uiMain& app )
{
    OD::ModDeps().ensureLoaded( "AllNonUi" );
    OD::ModDeps().ensureLoaded( "uiIo" );

    uiDialog::setTitlePos( uiDialog::LeftSide );

    PtrMan<ApplicationData> bapp = new ApplicationData();

    const CommandLineParser clp;
    uiRetVal uirv = IOMan::setDataSource( clp );
    mIfIOMNotOK( return 1 )

    checkScreenRes();
    uirv.setOK();
    bool skippluginsel = false;
    if ( !doProductSelection(skippluginsel,uirv) )
    {
	if ( !uirv.isOK() )
	    uiMSG().error( uirv );

	return 1;
    }

    SetProgramRestarter( ODMainProgramRestarter );
    const uiPixmap pm( "../splash" );
    PtrMan<uiSplashScreen> splash = new uiSplashScreen( pm );
    splash->show();
    splash->showMessage( "Loading plugins ..." );

    PIM().loadAuto( false, !skippluginsel );
    OD::ModDeps().ensureLoaded( "uiODMain" );
    PtrMan<uiODMain> odmain = new uiODMain( app );
    manODMainWin( odmain.ptr(), true );

    PIM().loadAuto( true, !skippluginsel );

    splash->showMessage( "Initializing Scene ..." );
    odmain->initScene();
    splash = nullptr;
    odmain->setActivateOnFirstShow();

    return odmain->go() ? 0 : 1;
}


#define mMemStatusFld 4
static uiString cputxt_;

uiODMain::uiODMain( uiMain& a )
    : uiMainWin(nullptr,toUiString("OpendTect Main Window"),5,true)
    , sessionSave(this)
    , sessionRestoreEarly(this)
    , sessionRestore(this)
    , justBeforeGo(this)
    , beforeExit(this)
    , uiapp_(a)
    , lastsession_(*new ODSession)
    , programname_( "OpendTect" )
    , sesstimer_(*new Timer("Session restore timer"))
    , memtimer_(*new Timer("Memory display timer"))
    , newsurvinittimer_(*new Timer("New survey init timer"))
{
    setIconText( getProgramString() );
    uiapp_.setTopLevel( this );

    applmgr_ = new uiODApplMgr( *this );
    if ( buildUI() )
	failed_ = false;

    mAttachCB( IOM().afterSurveyChange, uiODMain::afterSurveyChgCB );
    mAttachCB( sesstimer_.tick, uiODMain::sessTimerCB );

    const int systemnrcpus = Threads::getSystemNrProcessors();
    const int odnrcpus = Threads::getNrProcessors();
    const bool useallcpus = systemnrcpus == odnrcpus;

    uiString statustt = tr( "System memory: Free/Available" );
    if ( !useallcpus )
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

    if ( !useallcpus )
	cputxt_ = tr( "[cpu] %1/%2" ).arg( odnrcpus ).arg( systemnrcpus );

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
	{ delete ctio->ioobj_; deleteAndNullPtr( ctio ); }
    else
    {
	delete ctio->ioobj_; ctio->ioobj_ = dlg.ioObj()->clone();
	const MultiID id( ctio->ioobj_ ? ctio->ioobj_->key() : MultiID("") );
	cursessid_ = id;
    }

    return ctio;
}


static bool hasSceneItems( uiVisPartServer* visserv )
{
    TypeSet<SceneID> sceneids;
    visserv->getSceneIds( sceneids );
    if ( sceneids.isEmpty() ) return false;

    int nrchildren = 0;
    TypeSet<VisID> visids;
    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	visserv->getChildIds( sceneids[0], visids );
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

void uiODMain::saveSession()
{
    CtxtIOObj* ctio = getUserSessionIOData( false );
    if ( !ctio ) { delete ctio; return; }
    ODSession sess; cursession_ = &sess;
    if ( !updateSession() ) mDelCtioRet()
    uiString bs;
    if ( !ODSessionTranslator::store(sess,ctio->ioobj_,bs) )
	{ uiMSG().error( bs ); mDelCtioRet() }

    lastsession_ = sess; cursession_ = &lastsession_;
    mDelCtioRet()
}


void uiODMain::restoreSession()
{
    CtxtIOObj* ctio = getUserSessionIOData( true );
    if ( !ctio ) { delete ctio; return; }
    restoreSession( ctio->ioobj_ );
    mDelCtioRet()
}


class uiODMainAutoSessionDlg : public uiDialog
{ mODTextTranslationClass(uiODMainAutoSessionDlg);
public:

uiODMainAutoSessionDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Auto-load session"),mNoDlgTitle,
				 mODHelpKey(mODMainAutoSessionDlgHelpID) ))
{
    bool douse = false; MultiID id;
    ODSession::getStartupData( douse, id );

    usefld_ = new uiGenInput( this, tr("Auto-load session mode"),
	  BoolInpSpec(douse,uiStrings::sEnabled(),uiStrings::sDisabled() ));
    mAttachCB( usefld_->valueChanged, uiODMainAutoSessionDlg::useChg );
    doselfld_ = new uiGenInput( this, tr("Use one for this survey"),
				BoolInpSpec(!id.isUdf()) );
    mAttachCB( doselfld_->valueChanged, uiODMainAutoSessionDlg::useChg );
    doselfld_->attach( alignedBelow, usefld_ );

    IOObjContext ctxt = mIOObjContext( ODSession );
    ctxt.forread_ = true;
    sessionfld_ = new uiIOObjSel( this, ctxt );
    sessionfld_->setInput( id );
    sessionfld_->attach( alignedBelow, doselfld_ );

    loadnowfld_ = new uiGenInput( this, tr("Load selected session now"),
				  BoolInpSpec(true) );
    loadnowfld_->attach( alignedBelow, sessionfld_ );

    mAttachCB( postFinalize(), uiODMainAutoSessionDlg::initDlg );
}

~uiODMainAutoSessionDlg()
{
    detachAllNotifiers();
}

void initDlg( CallBacker* )
{
    useChg( nullptr );
}

void useChg( CallBacker* )
{
    const bool douse = usefld_->getBoolValue();
    const bool dosel = douse ? doselfld_->getBoolValue() : false;
    doselfld_->display( douse );
    sessionfld_->display( dosel );
    loadnowfld_->display( dosel );
}


bool acceptOK( CallBacker* )
{
    const bool douse = usefld_->getBoolValue();
    const bool dosel = douse ? doselfld_->getBoolValue() : false;
    if ( !dosel )
    {
	ODSession::setStartupData( douse, MultiID::udf() );
	return true;
    }

    const IOObj* ioobj = sessionfld_->ioobj();
    if ( !ioobj ) return false;

    ODSession::setStartupData( douse, sessionfld_->key() );
    return true;
}

    uiGenInput*		usefld_;
    uiGenInput*		doselfld_;
    uiIOObjSel*		sessionfld_;
    uiGenInput*		loadnowfld_;
};


void uiODMain::autoSession()
{
    uiODMainAutoSessionDlg dlg( this );
    if ( dlg.go() )
    {
	if ( dlg.loadnowfld_->getBoolValue() )
	    handleStartupSession();
    }
}


void uiODMain::restoreSession( const IOObj* ioobj )
{
    ODSession sess; uiString bs;
    if ( !ODSessionTranslator::retrieve(sess,ioobj,bs) )
	{ uiMSG().error( bs ); return; }

    cursession_ = &sess;
    doRestoreSession();
    cursession_ = &lastsession_; lastsession_.clear();
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

    sessionRestoreEarly.trigger();
    applMgr().EMServer()->usePar( cursession_->empars() );
    applMgr().seisServer()->usePar( cursession_->seispars() );
    if ( applMgr().nlaServer() )
	applMgr().nlaServer()->usePar( cursession_->nlapars() );
    if ( SI().has2D() )
    {
	applMgr().attrServer()->usePar( cursession_->attrpars(true,false),
					true, false );
	applMgr().attrServer()->usePar( cursession_->attrpars(true,true),
					true, true );
    }
    if ( SI().has3D() )
    {
	applMgr().attrServer()->usePar( cursession_->attrpars(false,false),
					false, false );
	applMgr().attrServer()->usePar( cursession_->attrpars(false,true),
					false, true );
    }
    applMgr().mpeServer()->usePar( cursession_->mpepars() );
    const bool visok = applMgr().visServer()->usePar( cursession_->vispars() );
    if ( visok )
	sceneMgr().useScenePars( cursession_->scenepars() );

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
}


void uiODMain::handleStartupSession()
{
    bool douse = false;
    MultiID id;
    ODSession::getStartupData( douse, id );
    if ( !douse || id.isUdf() )
	return;

    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj ) return;
    cursessid_ = id;
    restoreSession( ioobj );
}


void uiODMain::sessTimerCB( CallBacker* )
{
    sceneMgr().layoutScenes();
}


void uiODMain::afterStartupCB( CallBacker* )
{
    uiServiceClientMgr::setFor( *this );
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

    od_int64 tot, free;
    OD::getSystemMemory( tot, free );
    NrBytesToStringCreator converter;
    converter.setUnitFrom( tot );
    uiString txt = tr( "[free mem] %1/%2%3" );

    //Use separate calls to avoid the reuse of the converter's buffer
    txt.arg( converter.getString( free, 1, false ) );
    txt.arg( converter.getString( tot,1,true ));
    txt.arg( cputxt_.isEmpty() ? cputxt_ : toUiString(" | %1").arg(cputxt_));

    statusBar()->message( txt, mMemStatusFld );
}


bool uiODMain::go()
{
    if ( failed_ ) return false;

    show();

    Timer tm( "Handle startup session" );
    mAttachCB( tm.tick, uiODMain::afterSurveyChgCB );
    tm.start( 200, true );

    const int rv = uiapp_.exec();
    return rv ? false : true;
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
    if ( !applmgr_->attrServer() ) return false;

    if ( !NotSavedPrompter::NSP().doTrigger( uiMainWin::activeWindow(), true,
					     actiontype ) )
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
	int res = uiMSG().askSave( tr("Current attribute set has changed.\n"
				      "Store the changes now?") );
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
    handleStartupSession();
}


void uiODMain::checkUpdateAvailable()
{
    const BufferString relrootdir( __ismac__ ?
				    FilePath(GetSoftwareDir(true)).pathOnly() :
				    BufferString(GetSoftwareDir(true)) );
    OS::MachineCommand mc;
    ODInst::getMachComm( relrootdir, mc );
    mc.addFlag( "updcheck_report" );
    if ( mc.isBad() || !File::exists(mc.program()) )
	return;

    auto& mgr = Threads::CommandLaunchMgr::getMgr();
    CallBack cb( mCB(this,uiODMain,updateStatusCB) );
    mgr.execute( mc, true, true, &cb );
}


void uiODMain::updateStatusCB( CallBacker* cb )
{
    const auto* ct = Threads::CommandLaunchMgr::getMgr().getCommandTask( cb );
    if ( ct )
    {
	const BufferString stdoutstr = ct->getStdOutput();
	const BufferString stderrstr = ct->getStdError();
	if ( stdoutstr.isEmpty() || !stderrstr.isEmpty() )
	{
	    ODInst::updatesAvailable( 0 );
	    if ( !stderrstr.isEmpty() )
		ErrMsg( BufferString("Error checking for software updates: ",
				     stderrstr) );
	}
	else
	    ODInst::updatesAvailable(
				stdoutstr==ODInst::sKeyHasUpdate() ? 1 : 0 );
	updateCaptionCB( nullptr );
    }

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

    if ( ODInst::getAutoInstType() == ODInst::InformOnly &&
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
     if ( doconfirm )
     {
	 if ( !uiMSG().askGoOn( tr("Do you want to %1 %2?")
				.arg(restarting_?"restart":"close")
				.arg(programname_),
				actstr, uiStrings::sCancel() ) )
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
    if ( !closeOK(withinteraction,doconfirm) )
	return false;

    //TODO:
    // 1) offer to save the session
    // 2) offer to remember which data is preloaded
    // Save session ID and preload spec in a file restart.pars in ~/.od (user)
    // Make sure that if a restart.pars file is found at startup, stuff will
    // be restored. Remove restart.pars when done

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


void uiODMain::exit( bool withinteraction, bool doconfirm )
{
    if ( withinteraction || doconfirm )
    {
	if ( !closeOK(withinteraction,doconfirm) )
	    return;
    }

    uiapp_.exit(0);
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
