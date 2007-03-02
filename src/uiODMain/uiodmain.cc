/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uiodmain.cc,v 1.66 2007-03-02 15:49:07 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiodmain.h"

#include "uiattribpartserv.h"
#include "uicmain.h"
#include "uicursor.h"
#include "uidockwin.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uinlapartserv.h"
#include "uiodapplmgr.h"
#include "uiodmenumgr.h"
#include "uiodscenemgr.h"
#include "uipluginsel.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"
#include "uimpepartserv.h"
#include "uipluginsel.h"
#include "uisetdatadir.h"
#include "uisurvey.h"
#include "uitoolbar.h"
#include "uisurvinfoed.h"
#include "ui2dsip.h"

#include "ctxtioobj.h"
#include "envvars.h"
#include "filegen.h"
#include "ioman.h"
#include "ioobj.h"
#include "odsessionfact.h"
#include "pixmap.h"
#include "plugins.h"
#include "ptrman.h"
#include "settings.h"
#include "survinfo.h"
#include "timer.h"

#ifndef USEQT3
# include "uisplashscreen.h"
#endif

static const int cCTHeight = 200;


static uiODMain* manODMainWin( uiODMain* i )
{
    static uiODMain* theinst = 0;
    if ( i ) theinst = i;
    return theinst;
}


uiODMain* ODMainWin()
{
    return manODMainWin(0);
}


int ODMain( int argc, char** argv )
{
    PIM().setArgs( argc, argv );
    PIM().loadAuto( false );
    uiODMain* odmain = new uiODMain( *new uicMain(argc,argv) );

#ifndef USEQT3
    ioPixmap pm( "od.jpg" );
    uiSplashScreen splash( pm );
    splash.show();
    splash.showMessage( "Loading plugins ..." );
#endif

    manODMainWin( odmain );
    bool dodlg = true;
    Settings::common().getYN( uiPluginSel::sKeyDoAtStartup, dodlg );
    ObjectSet<PluginManager::Data>& pimdata = PIM().getData();
    if ( dodlg && pimdata.size() )
    {
	uiPluginSel dlg( odmain );
	if ( dlg.nrPlugins() )
	    dlg.go();
    }
    PIM().loadAuto( true );
    if ( !odmain->ensureGoodSurveySetup() )
	return 1;

#ifdef USEQT3
    odmain->initScene();
#else
    splash.showMessage( "Initializing Scene ..." );
    odmain->initScene();
    splash.finish( odmain );
#endif
    odmain->go();
    delete odmain;
    return 0;
}



uiODMain::uiODMain( uicMain& a )
	: uiMainWin(0,"OpendTect Main Window",4,true)
    	, uiapp(a)
	, failed(true)
    	, menumgr(0)
    	, scenemgr(0)
    	, ctabed(0)
    	, ctabwin(0)
    	, lastsession(*new ODSession)
    	, cursession(0)
    	, sessionSave(this)
    	, sessionRestore(this)
        , applicationClosing(this)
{
    uiMSG().setMainWin( this );
    uiapp.setTopLevel( this );

    if ( !ensureGoodDataDir() )
	::exit( 0 );

    uiSurveyInfoEditor::addInfoProvider( new ui2DSurvInfoProvider );

    applmgr = new uiODApplMgr( *this );

    if ( buildUI() )
	failed = false;

    IOM().afterSurveyChange.notify( mCB(this,uiODMain,handleStartupSession) );
}


uiODMain::~uiODMain()
{
    if ( ODMainWin()==this )
	manODMainWin( 0 );

    delete ctabed;
    delete ctabwin;
    delete &lastsession;
}


bool uiODMain::ensureGoodDataDir()
{
    if ( !uiSetDataDir::isOK() )
    {
	uiSetDataDir dlg( this );
	return dlg.go();
    }

    return true;
}


bool uiODMain::ensureGoodSurveySetup()
{
    BufferString errmsg;
    if ( !IOMan::validSurveySetup(errmsg) )
    {
	std::cerr << errmsg << std::endl;
	uiMSG().error( errmsg );
	return false;
    }
    else if ( !IOM().isReady() )
    {
	while ( !applmgr->manageSurvey() )
	{
	    if ( uiMSG().askGoOn( "No survey selected. Do you wish to quit?" ) )
		return false;
	}
    }

    return true;
}

#define mCBarKey	"dTect.ColorBar"
#define mHVKey		"show vertical"
#define mTopKey		"show on top"

bool uiODMain::buildUI()
{
    scenemgr = new uiODSceneMgr( this );
    menumgr = new uiODMenuMgr( this );
    menumgr->initSceneMgrDepObjs( applmgr, scenemgr );

    const char* s = GetEnvVar( "DTECT_CBAR_POS" );
    bool isvert = !s || *s == 'v' || *s == 'V';
    bool isontop = s && *s
		&& (*s == 't' || *s == 'T' || *(s+1) == 't' || *(s+1) == 'T');
    if ( !s )
    {
	PtrMan<IOPar> iopar = Settings::common().subselect( mCBarKey );
	if ( !iopar ) iopar = new IOPar;

	bool insettings = false;
	insettings |= iopar->getYN( mHVKey, isvert );
	insettings |= iopar->getYN( mTopKey, isontop );

	if ( !insettings )
	{
	    iopar->setYN( mHVKey, isvert );
	    iopar->setYN( mTopKey, isontop );

	    Settings::common().mergeComp( *iopar, mCBarKey );
	    Settings::common().write();
	}
    }

    if ( isvert )
    {
	ctabwin = new uiDockWin( this, "Color Table" );
	ctabed = new uiVisColTabEd( ctabwin, true );
	ctabed->coltabChange.notify( mCB(applmgr,uiODApplMgr,coltabChg) );
	ctabed->setPrefHeight( cCTHeight );
	ctabed->colTabGrp()->attach( hCentered );

	addDockWindow( *ctabwin, isontop ? uiMainWin::TornOff
					 : uiMainWin::Left );
	ctabwin->setResizeEnabled( true );
    }
    else
    {
	uiToolBar* tb = new uiToolBar( this, "Color Table" );
	ctabed = new uiVisColTabEd( ctabwin, false );
	ctabed->coltabChange.notify( mCB(applmgr,uiODApplMgr,coltabChg) );
	tb->addObject( ctabed->colTabGrp()->attachObj() );
    }

    return true;
}


void uiODMain::initScene()
{
    scenemgr->initMenuMgrDepObjs();
}


IOPar& uiODMain::sessionPars()
{
    return cursession->pluginpars();
}


CtxtIOObj* uiODMain::getUserSessionIOData( bool restore )
{
    CtxtIOObj* ctio = mMkCtxtIOObj(ODSession);
    ctio->ctxt.forread = restore;
    uiIOObjSelDlg dlg( this, *ctio );
    if ( !dlg.go() )
	{ delete ctio->ioobj; delete ctio; ctio = 0; }
    else
	{ delete ctio->ioobj; ctio->ioobj = dlg.ioObj()->clone(); }

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
    cursession = &sess;
    updateSession();
    cursession = &lastsession;
    return !( sess == lastsession );
}


#define mDelCtioRet()	{ delete ctio->ioobj; delete ctio; return; }

void uiODMain::saveSession()
{
    CtxtIOObj* ctio = getUserSessionIOData( false );
    if ( !ctio ) { delete ctio; return; }
    ODSession sess; cursession = &sess;
    if ( !updateSession() ) mDelCtioRet()
    BufferString bs;
    if ( !ODSessionTranslator::store(sess,ctio->ioobj,bs) )
	{ uiMSG().error( bs ); mDelCtioRet() }

    lastsession = sess; cursession = &lastsession;
    mDelCtioRet()
}


void uiODMain::restoreSession()
{
    CtxtIOObj* ctio = getUserSessionIOData( true );
    if ( !ctio ) { delete ctio; return; }
    restoreSession(ctio->ioobj);
    mDelCtioRet()
}


class uiODMainAutoSessionDlg : public uiDialog
{
public:

uiODMainAutoSessionDlg( uiODMain* p )
    : uiDialog(p,uiDialog::Setup("Auto-load session"
				,"Set auto-load session","50.3.1"))
    , ctio_(*mMkCtxtIOObj(ODSession))
{
    bool douse = false; MultiID id;
    ODSession::getStartupData( douse, id );

    usefld_ = new uiGenInput( this, "Enable auto-load sessions",
	    		      BoolInpSpec(true) );
    usefld_->setValue( douse );
    usefld_->valuechanged.notify( mCB(this,uiODMainAutoSessionDlg,useChg) );

    ctio_.setObj( id ); ctio_.ctxt.forread = true;
    selgrp_ = new uiIOObjSelGrp( this, ctio_ );
    selgrp_->attach( alignedBelow, usefld_ );
    lbl_ = new uiLabel( this, "Session to use" );
    lbl_->attach( centeredLeftOf, selgrp_ );

    loadnowfld_ = new uiGenInput( this, "Load selected session now",
	    			  BoolInpSpec(true) );
    loadnowfld_->attach( alignedBelow, selgrp_ );

    finaliseDone.notify( mCB(this,uiODMainAutoSessionDlg,useChg) );
}

~uiODMainAutoSessionDlg()
{
    delete ctio_.ioobj; delete &ctio_;
}

void useChg( CallBacker* )
{
    const bool douse = usefld_->getBoolValue();
    selgrp_->display( douse );
    loadnowfld_->display( douse );
    lbl_->display( douse );
}


bool acceptOK( CallBacker* )
{
    const bool douse = usefld_->getBoolValue();
    selgrp_->processInput();
    if ( douse && selgrp_->nrSel() < 1 )
	{ uiMSG().error("Please select a session"); return false; }
    if ( selgrp_->nrSel() > 0 )
	ctio_.setObj( selgrp_->selected(0) );

    const MultiID id( ctio_.ioobj ? ctio_.ioobj->key() : MultiID("") );
    ODSession::setStartupData( douse, id );

    return true;
}

    CtxtIOObj&		ctio_;

    uiGenInput*		usefld_;
    uiGenInput*		loadnowfld_;
    uiIOObjSelGrp*	selgrp_;
    uiLabel*		lbl_;

};


void uiODMain::autoSession()
{
    uiODMainAutoSessionDlg dlg( this );
    if ( dlg.go() )
    {
	if ( dlg.loadnowfld_->getBoolValue() && dlg.ctio_.ioobj )
	    handleStartupSession(0);
    }
}


void uiODMain::restoreSession( const IOObj* ioobj )
{
    ODSession sess; BufferString bs;
    if ( !ODSessionTranslator::retrieve(sess,ioobj,bs) )
	{ uiMSG().error( bs ); return; }

    cursession = &sess;
    doRestoreSession();
    cursession = &lastsession; lastsession.clear();
    ctabed->updateColTabList();
}


bool uiODMain::updateSession()
{
    cursession->clear();
    applMgr().visServer()->fillPar( cursession->vispars() );
    applMgr().attrServer()->fillPar( cursession->attrpars(true), true );
    applMgr().attrServer()->fillPar( cursession->attrpars(false), false );
    sceneMgr().getScenePars( cursession->scenepars() );
    if ( applMgr().nlaServer()
      && !applMgr().nlaServer()->fillPar( cursession->nlapars() ) ) 
	return false;
    applMgr().mpeServer()->fillPar( cursession->mpepars() );

    sessionSave.trigger();
    return true;
}


void uiODMain::doRestoreSession()
{
    uiCursor::setOverride( uiCursor::Wait );
    sceneMgr().cleanUp( false );
    applMgr().resetServers();

    if ( applMgr().nlaServer() )
	applMgr().nlaServer()->usePar( cursession->nlapars() );
    if ( SI().has2D() )
	applMgr().attrServer()->usePar( cursession->attrpars(true), true );
    if ( SI().has3D() )
	applMgr().attrServer()->usePar( cursession->attrpars(false), false );
    applMgr().mpeServer()->usePar( cursession->mpepars() );
    const bool visok = applMgr().visServer()->usePar( cursession->vispars() );
    sessionRestore.trigger();

    if ( visok )
    {
	sceneMgr().useScenePars( cursession->scenepars() );
	applMgr().visServer()->calculateAllAttribs();
    }
    else
    {
	uiCursor::restoreOverride();
	uiMSG().error( "An error occurred while reading session file.\n"
		       "A new scene will be launched" );	
	uiCursor::setOverride( uiCursor::Wait );
	sceneMgr().cleanUp( true );
    }

    uiCursor::restoreOverride();
}


void uiODMain::handleStartupSession( CallBacker* )
{
    bool douse = false; MultiID id;
    ODSession::getStartupData( douse, id );
    if ( !douse || id == "" )
	return;

    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj ) return;

    restoreSession( ioobj );
    sceneMgr().layoutScenes();
}


bool uiODMain::go()
{
    if ( failed ) return false;

    show();
    uiSurvey::updateViewsGlobal();
    Timer tm( "Auto session restore timer" );
    tm.tick.notify( mCB(this,uiODMain,handleStartupSession) );
    tm.start( 200, true );
    int rv = uiapp.exec();
    delete applmgr; applmgr = 0;
    return rv ? false : true;
}


bool uiODMain::askStore( bool& askedanything )
{
    if ( !applmgr->attrServer() ) return false;

    bool doask = false;
    Settings::common().getYN( "dTect.Ask store session", doask );
    if ( doask && hasSessionChanged() )
    {
	askedanything = true;
	int res = uiMSG().askGoOnAfter( "Do you want to save this session?" );
	if ( res == 0 )
	    saveSession();
	else if ( res == 2 )
	    return false;
    }

    doask = true;
    Settings::common().getYN( "dTect.Ask store picks", doask );
    if ( doask && !applmgr->pickSetsStored() )
    {
	askedanything = true;
	int res = uiMSG().askGoOnAfter( "Pick sets have changed.\n"
					"Store the changes now?");
	if ( res == 0 )
	    applmgr->storePickSets();
	else if ( res == 2 )
	    return false;
    }
    if ( SI().has2D() ) askStoreAttribs( true, askedanything );
    if ( SI().has3D() ) askStoreAttribs( false, askedanything );

    return true;
}


bool uiODMain::askStoreAttribs( bool is2d, bool& askedanything )
{
    bool doask = true;
    Settings::common().getYN( "dTect.Ask store attributeset", doask );
    if ( doask && !applmgr->attrServer()->setSaved( is2d ) )
    {
	askedanything = true;
	int res = uiMSG().askGoOnAfter( "Current attribute set has changed.\n"
					"Store the changes now?" );
	if ( res == 0 )
	    applmgr->attrServer()->saveSet( is2d );
	else if ( res == 2 )
	    return false;
    }

    return true;
}

bool uiODMain::closeOK()
{
    applicationClosing.trigger();

    if ( failed ) return true;

    menumgr->storePositions();
    scenemgr->storePositions();
    ctabwin->storePosition();

    bool askedanything = false;
    if ( !askStore(askedanything) )
	return false;

    if ( !askedanything )
    {
	bool doask = false;
	Settings::common().getYN( "dTect.Ask close", doask );
	if ( doask && !uiMSG().askGoOn( "Do you want to quit?" ) )
	    return false;
    }

    removeDockWindow( ctabwin );
    delete scenemgr;
    delete menumgr;

    return true;
}


void uiODMain::exit()
{
    if ( !closeOK() ) return;

    uiapp.exit(0);
}
