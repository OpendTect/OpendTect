/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiodmain.h"

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
#include "uipluginsel.h"
#include "uiseispartserv.h"
#include "uisetdatadir.h"
#include "uistrattreewin.h"
#include "uisurvey.h"
#include "uisurvinfoed.h"
#include "uitoolbar.h"
#include "ui2dsip.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"
#include "uiserviceclientmgr.h"

#include "coltabsequence.h"
#include "ctxtioobj.h"
#include "envvars.h"
#include "genc.h"
#include "file.h"
#include "ioman.h"
#include "ioobj.h"
#include "moddepmgr.h"
#include "mousecursor.h"
#include "nrbytes2string.h"
#include "oddirs.h"
#include "odinst.h"
#include "odplatform.h"
#include "odsessionfact.h"
#include "odsysmem.h"
#include "odver.h"
#include "plugins.h"
#include "ptrman.h"
#include "settingsaccess.h"
#include "survgeom.h"
#include "survinfo.h"
#include "timer.h"
#include "visdata.h"
#include "od_helpids.h"

#include <iostream>


extern "C" const char* GetSettingsDataDir();

static uiODMain* manODMainWin( uiODMain* i )
{
    mDefineStaticLocalObject( uiODMain*, theinst, = nullptr );
    if ( i ) theinst = i;
    return theinst;
}


uiODMain* ODMainWin()
{
    return manODMainWin( nullptr );
}


static const char* sKeyShowLowRes = "UI.LowRes.Show";
static const char* sKeyShowSubRes = "Ui.SubRes.Show";
static const int cScreenLowRes = 768;
static const int cScreenSubRes = 920;

static void checkScreenRes()
{
    uiMain& uimain = uiMain::theMain();
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


int ODMain( int , char** )
{ /* Not used, only for compatibility */ return -1; }


int ODMain( uiMain& app )
{
    OD::ModDeps().ensureLoaded( "AllNonUi" );
    OD::ModDeps().ensureLoaded( "uiBase" );
    uiDialog::setTitlePos( uiDialog::LeftSide );

    PtrMan<uiODMain> odmain = new uiODMain( app );
    manODMainWin( odmain );

    checkScreenRes();

    bool dodlg = true;
    Settings::common().getYN( uiPluginSel::sKeyDoAtStartup(), dodlg );
    ObjectSet<PluginManager::Data>& pimdata = PIM().getData();
    if ( dodlg && pimdata.size() )
    {
	uiPluginSel dlg( odmain );
	dlg.setPopupArea( uiMainWin::Auto );
	if ( dlg.nrPlugins() && !dlg.go() )
	    return 1;
    }

    SetProgramRestarter( ODMainProgramRestarter );

    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "uiODMain" );
    PIM().loadAuto( true );
    if ( !odmain->ensureGoodSurveySetup() )
	return 1;

    File::initTempDir();

    odmain->initScene();
    return odmain->go() ? 0 : 1;
}


#define mMemStatusFld 4
static uiString cputxt_;

uiODMain::uiODMain( uiMain& a )
    : uiMainWin(0,toUiString("OpendTect Main Window"),5,true)
    , uiapp_(a)
    , failed_(true)
    , applmgr_(0)
    , menumgr_(0)
    , scenemgr_(0)
    , ctabed_(0)
    , ctabtb_(0)
    , sesstimer_(*new Timer("Session restore timer"))
    , memtimer_(*new Timer("Memory display timer"))
    , newsurvinittimer_(*new Timer("New survey init timer"))
    , neednewsurvinit_(false)
    , lastsession_(*new ODSession)
    , cursession_(0)
    , restoringsess_(false)
    , restarting_(false)
    , sessionSave(this)
    , sessionRestoreEarly(this)
    , sessionRestore(this)
    , justBeforeGo(this)
    , programname_( "OpendTect" )
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
    memtimer_.tick.notify( mCB(this,uiODMain,memTimerCB) );
    memtimer_.start( 1000 );

    if ( !useallcpus )
	cputxt_ = tr( "[cpu] %1/%2" ).arg( odnrcpus ).arg( systemnrcpus );

    mAttachCB( postFinalise(), uiODMain::afterStartupCB );
}


uiODMain::~uiODMain()
{
    detachAllNotifiers();
    memtimer_.stop();
    if ( ODMainWin()==this )
	manODMainWin( nullptr );

    delete &lastsession_;
    delete &sesstimer_;
    delete &memtimer_;
    delete &newsurvinittimer_;

    delete menumgr_;
    delete scenemgr_;
    delete viewer2dmgr_;
    delete applmgr_;
}



bool uiODMain::ensureGoodDataDir()
{
    while ( !IOMan::isValidDataRoot(GetBaseDataDir()) )
    {
	uiSetDataDir dlg( this );
	if ( !dlg.go() )
	{
	    if ( uiMSG().askGoOn( tr("Without a valid data root, %1 "
				     "cannot start.\nDo you wish to exit?")
					.arg(programname_) ) )
		return false;
	}
	else if ( uiSetDataDir::setRootDataDir(this,dlg.selectedDir()) )
	    break;
    }

    return true;
}


bool uiODMain::ensureGoodSurveySetup()
{
    if ( !ensureGoodDataDir() )
	return false;

    BufferString errmsg;
    int res = 0;
    if ( !IOMan::validSurveySetup(errmsg) )
    {
	std::cerr << errmsg << std::endl;
	uiMSG().error( toUiString(errmsg) );
	return false;
    }
    else if ( !IOM().isReady() )
    {
	while ( res == 0 )
	{
	    res = uiODApplMgr::manageSurvey();
	    if ( res == 0 && uiMSG().askGoOn( tr("Without a valid survey, %1 "
				     "cannot start.\nDo you wish to exit?")
				     .arg( programname_ )) )
		return false;
	}
    }
    else
	res = 1;

    if ( res == 3 )
    {
	neednewsurvinit_ = true;
	newsurvinittimer_.start( 200, true );
	newsurvinittimer_.tick.notify( mCB(this,uiODMain,newSurvInitTimerCB) );
    }

    return true;
}


bool uiODMain::buildUI()
{
    scenemgr_ = new uiODSceneMgr( this );
    viewer2dmgr_ = new uiODViewer2DMgr( this );
    menumgr_ = new uiODMenuMgr( this );
    menumgr_->initSceneMgrDepObjs( applmgr_, scenemgr_ );

    uiColorTableToolBar* tb = new uiColorTableToolBar( this );
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
	{ delete ctio->ioobj_; deleteAndZeroPtr( ctio ); }
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
    TypeSet<int> sceneids;
    visserv->getChildIds( -1, sceneids );
    if ( sceneids.isEmpty() ) return false;

    int nrchildren = 0;
    TypeSet<int> visids;
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

uiODMainAutoSessionDlg( uiODMain* p )
    : uiDialog(p,uiDialog::Setup(tr("Auto-load session"),mNoDlgTitle,
				 mODHelpKey(mODMainAutoSessionDlgHelpID) ))
{
    bool douse = false; MultiID id;
    ODSession::getStartupData( douse, id );

    usefld_ = new uiGenInput( this, tr("Auto-load session mode"),
	  BoolInpSpec(douse,uiStrings::sEnabled(),uiStrings::sDisabled() ));
    usefld_->valuechanged.notify( mCB(this,uiODMainAutoSessionDlg,useChg) );
    doselfld_ = new uiGenInput( this, tr("Use one for this survey"),
				BoolInpSpec(!id.isEmpty()) );
    doselfld_->valuechanged.notify( mCB(this,uiODMainAutoSessionDlg,useChg) );
    doselfld_->attach( alignedBelow, usefld_ );

    IOObjContext ctxt = mIOObjContext( ODSession );
    ctxt.forread_ = true;
    sessionfld_ = new uiIOObjSel( this, ctxt );
    sessionfld_->setInput( id );
    sessionfld_->attach( alignedBelow, doselfld_ );

    loadnowfld_ = new uiGenInput( this, tr("Load selected session now"),
				  BoolInpSpec(true) );
    loadnowfld_->attach( alignedBelow, sessionfld_ );

    postFinalise().notify( mCB(this,uiODMainAutoSessionDlg,useChg) );
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
    if ( !douse || id.isEmpty() )
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
}


void uiODMain::newSurvInitTimerCB( CallBacker* )
{
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


void uiODMain::updateCaption()
{
    uiString capt = toUiString( "%1 (%2)" )
	.arg( getProgramString() )
	.arg( OD::Platform::local().osName() );

    if ( ODInst::getAutoInstType() == ODInst::InformOnly
	&& ODInst::updatesAvailable() )
	capt.append( tr(" *UPDATE AVAILABLE*") );

    const char* usr = GetSoftwareUser();
    if ( usr && *usr )
    {
	capt.append( tr(" [%1] ").arg( usr ) );
    }

    if ( !SI().name().isEmpty() )
	capt.append( " : %1" ).arg( SI().name() );

    setCaption( capt );
}


bool uiODMain::closeOK()
{
    saveSettings();

    bool askedanything = false;
    uiString actstr = restarting_ ? uiStrings::sRestart() : uiStrings::sClose();
    if ( !askStore(askedanything,
	  uiStrings::phrJoinStrings(actstr,toUiString(programname_)) ) )
    {
	uiMSG().message( restarting_ ? tr("Restart cancelled")
				     : tr("Closing cancelled"));
	return false;
    }

    if ( failed_ ) return true;

    if ( !askedanything )
    {
	if ( !uiMSG().askGoOn( tr("Do you want to %1 %2?")
			       .arg(restarting_?"restart":"close")
				       .arg(programname_),
			       actstr, uiStrings::sCancel()) )
	    return false;
    }

    return true;
}


void uiODMain::closeApplication()
{
}


uiString uiODMain::getProgramString() const
{
    return toUiString( "%1 V%2" ).arg( programname_ ).arg( GetFullODVersion() );
}


uiString uiODMain::getProgramName()
{
    return toUiString(programname_);
}


void uiODMain::restart()
{
    restarting_ = true;

    if ( !closeOK() )
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


void uiODMain::exit()
{
    if ( !closeOK() ) return;

    uiapp_.exit(0);
}


void uiODMain::forceExit()
{
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
    dTectMenuChanged();
    dTectToolbarChanged();
}


void uiPluginInitMgr::beforeSurvChgCB( CallBacker* )
{ beforeSurveyChange(); }

void uiPluginInitMgr::afterSurvChgCB( CallBacker* )
{ afterSurveyChange(); }

void uiPluginInitMgr::applCloseCB( CallBacker* )
{}

void uiPluginInitMgr::menuChgCB( CallBacker* )
{ dTectMenuChanged(); }

void uiPluginInitMgr::tbChgCB( CallBacker* )
{ dTectToolbarChanged(); }

void uiPluginInitMgr::treeAddCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sceneid,cb);
    treeAdded( sceneid );
}
