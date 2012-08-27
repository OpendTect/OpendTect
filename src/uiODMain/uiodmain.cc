/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2002
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uiodmain.cc,v 1.162 2012-08-27 13:16:50 cvskris Exp $";

#include "uiodmain.h"

#include "uiattribpartserv.h"
#include "uicmain.h"
#include "uidockwin.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uistatusbar.h"
#include "uilabel.h"
#include "uimpepartserv.h"
#include "uimsg.h"
#include "uinlapartserv.h"
#include "uiodapplmgr.h"
#include "uiodmenumgr.h"
#include "uinotsaveddlg.h"
#include "uiodscenemgr.h"
#include "uiodviewer2dmgr.h"
#include "uipluginsel.h"
#include "uisetdatadir.h"
#include "uisplashscreen.h"
#include "uistrattreewin.h"
#include "uisurvey.h"
#include "uisurvinfoed.h"
#include "uitoolbar.h"
#include "ui2dsip.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"

#include "ctxtioobj.h"
#include "envvars.h"
#include "ioman.h"
#include "ioobj.h"
#include "mousecursor.h"
#include "oddatadirmanip.h"
#include "oddirs.h"
#include "odinst.h"
#include "odsessionfact.h"
#include "odver.h"
#include "moddepmgr.h"
#include "pixmap.h"
#include "plugins.h"
#include "ptrman.h"
#include "odplatform.h"
#include "settings.h"
#include "survinfo.h"
#include "timer.h"
#include "odsysmem.h"

#include "visdata.h"


extern "C" const char* GetSettingsDataDir();
extern void OD_Init_Transf_2DLineGeometry_From_2D_SeisLines();
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
    OD::ModDeps().ensureLoaded( "AllNonUi" );

    char** myargv = argv;
    int myargc = argc;
    for ( int iarg=1; iarg<myargc; iarg++ )
    {
	if ( !strcmp(myargv[iarg],"--osg") )
	{
	    visBase::DataObject::setOsg();
	    break;
	}
    }

    PIM().setArgs( argc, argv );
    PIM().loadAuto( false );

    uiDialog::setTitlePos( -1 );

    uiODMain* odmain = new uiODMain( *new uicMain(argc,argv) );
    ioPixmap pm( mGetSetupFileName("splash") );
  //  uiSplashScreen splash( pm );
  //  splash.show();
  //  splash.showMessage( "Loading plugins ..." );
    manODMainWin( odmain );

    bool dodlg = true;
    Settings::common().getYN( uiPluginSel::sKeyDoAtStartup(), dodlg );
    ObjectSet<PluginManager::Data>& pimdata = PIM().getData();
    if ( dodlg && pimdata.size() )
    {
	uiPluginSel dlg( odmain );
	if ( dlg.nrPlugins() )
	    dlg.go();
    }

    OD::ModDeps().ensureLoaded( "uiODMain" );
    PIM().loadAuto( true );
    if ( !odmain->ensureGoodSurveySetup() )
	return 1;

 //   splash.showMessage( "Initializing Scene ..." );
    odmain->initScene();
   // splash.finish( odmain );

    odmain->go();
    delete odmain;
    return 0;
}


#define mMemStatusFld 4


uiODMain::uiODMain( uicMain& a )
    : uiMainWin(0,"OpendTect Main Window",5,true)
    , uiapp_(a)
    , failed_(true)
    , applmgr_(0)
    , menumgr_(0)
    , scenemgr_(0)
    , ctabed_(0)
    , ctabwin_(0)
    , timer_(*new Timer("Session restore timer"))
    , memtimer_(*new Timer("Memory display timer"))
    , lastsession_(*new ODSession)
    , cursession_(0)
    , restoringsess_(false)
    , sessionSave(this)
    , sessionRestoreEarly(this)
    , sessionRestore(this)
    , justBeforeGo(this)
{
    BufferString icntxt( "OpendTect V", GetFullODVersion() );
    setIconText( icntxt.buf() );
    uiapp_.setTopLevel( this );
    uiSurveyInfoEditor::addInfoProvider( new ui2DSurvInfoProvider );

    if ( !ensureGoodDataDir()
      || (IOM().bad() && !ensureGoodSurveySetup()) )
	::exit( 0 );

    // TODO uncomment when transfer to S2DPOS() is complete
    OD_Init_Transf_2DLineGeometry_From_2D_SeisLines();

    applmgr_ = new uiODApplMgr( *this );
    if ( buildUI() )
	failed_ = false;

    IOM().afterSurveyChange.notify( mCB(this,uiODMain,afterSurveyChgCB) );
    timer_.tick.notify( mCB(this,uiODMain,timerCB) );

    statusBar()->setToolTip( mMemStatusFld,
			     "System memory: Free/Available" );
    statusBar()->setTxtAlign( mMemStatusFld, Alignment::HCenter );
    memtimer_.tick.notify( mCB(this,uiODMain,memTimerCB) );
    memtimer_.start( 1000 );
}


uiODMain::~uiODMain()
{
    memtimer_.stop();
    if ( ODMainWin()==this )
	manODMainWin( 0 );

    delete ctabwin_;
    delete &lastsession_;
    delete &timer_;
    delete &memtimer_;

    delete menumgr_;
    delete viewer2dmgr_;
    delete scenemgr_;
}



bool uiODMain::ensureGoodDataDir()
{
    if ( !OD_isValidRootDataDir(GetBaseDataDir()) )
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
	if ( !applmgr_ )
	{
	    BufferString msg( "Data management cannot be started. "
		"Please check your data directory:\n",
		GetBaseDataDir(),
		"\nDoes it look OK, exist, do you have read permission?" );
	    BufferStringSet msgs;
	    msgs.add( "\nData directory in:" );
	    msgs.add( BufferString("$HOME/.od/settings: ",
				   GetSettingsDataDir()) );
	    msgs.add( BufferString("DTECT_DATA variable: ",
				   GetEnvVar("DTECT_DATA")) );
	    uiMSG().errorWithDetails( msgs, msg );
	    return false;
	}

	while ( !applmgr_->manageSurvey() )
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
    scenemgr_ = new uiODSceneMgr( this );
    viewer2dmgr_ = new uiODViewer2DMgr( this );
    menumgr_ = new uiODMenuMgr( this );
    menumgr_->initSceneMgrDepObjs( applmgr_, scenemgr_ );

    const char* s = GetEnvVar( "DTECT_CBAR_POS" );
    bool isvert = s && (*s == 'v' || *s == 'V');
    if ( !s )
    {
	PtrMan<IOPar> iopar = Settings::common().subselect( mCBarKey );
	if ( !iopar ) iopar = new IOPar;

	bool insettings = false;
	insettings |= iopar->getYN( mHVKey, isvert );

	if ( !insettings )
	{
	    iopar->setYN( mHVKey, isvert );
	    Settings::common().mergeComp( *iopar, mCBarKey );
	    Settings::common().write();
	}
    }

    if ( isvert )
    {
	ctabwin_ = new uiDockWin( this, "Color Table" );
	ctabed_ = new uiVisColTabEd( 0, true );
	ctabed_->seqChange().notify( mCB(applmgr_,uiODApplMgr,colSeqChg) );
	ctabed_->mapperChange().notify( mCB(applmgr_,uiODApplMgr,colMapperChg));
//	ctabed_->colTabGrp()->attach( hCentered );
	ctabwin_->setGroup( ctabed_->colTabGrp() );
	addDockWindow( *ctabwin_, uiMainWin::Left );
    }
    else
    {
	uiToolBar* tb = new uiToolBar( this, "Color Table" );
	ctabed_ = new uiVisColTabEd( tb, false );
	ctabed_->seqChange().notify( mCB(applmgr_,uiODApplMgr,colSeqChg) );
	ctabed_->mapperChange().notify( mCB(applmgr_,uiODApplMgr,colMapperChg));
	tb->addObject( ctabed_->colTabGrp()->attachObj() );
    }


    return true;
}


void uiODMain::initScene()
{
    scenemgr_->initMenuMgrDepObjs();
    applMgr().visServer()->showMPEToolbar( false );
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
    ctio->ctxt.forread = restore;
    ctio->setObj( cursessid_ );
    uiIOObjSelDlg dlg( this, *ctio );
    if ( !dlg.go() )
	{ delete ctio->ioobj; delete ctio; ctio = 0; }
    else
    { 
	delete ctio->ioobj; ctio->ioobj = dlg.ioObj()->clone(); 
        const MultiID id( ctio->ioobj ? ctio->ioobj->key() : MultiID("") );
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


#define mDelCtioRet()	{ delete ctio->ioobj; delete ctio; return; }

void uiODMain::saveSession()
{
    CtxtIOObj* ctio = getUserSessionIOData( false );
    if ( !ctio ) { delete ctio; return; }
    ODSession sess; cursession_ = &sess;
    if ( !updateSession() ) mDelCtioRet()
    BufferString bs;
    if ( !ODSessionTranslator::store(sess,ctio->ioobj,bs) )
	{ uiMSG().error( bs ); mDelCtioRet() }

    lastsession_ = sess; cursession_ = &lastsession_;
    mDelCtioRet()
}


void uiODMain::restoreSession()
{
    CtxtIOObj* ctio = getUserSessionIOData( true );
    if ( !ctio ) { delete ctio; return; }
    restoreSession( ctio->ioobj );
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

    usefld_ = new uiGenInput( this, "Auto-load sessions",
	    		      BoolInpSpec(douse,"Enabled","Disabled") );
    usefld_->valuechanged.notify( mCB(this,uiODMainAutoSessionDlg,useChg) );
    doselfld_ = new uiGenInput( this, "Use one for this survey",
	    		      BoolInpSpec(id != "") );
    doselfld_->valuechanged.notify( mCB(this,uiODMainAutoSessionDlg,useChg) );
    doselfld_->attach( alignedBelow, usefld_ );

    ctio_.setObj( id ); ctio_.ctxt.forread = true;
    selgrp_ = new uiIOObjSelGrp( this, ctio_ );
    selgrp_->attach( alignedBelow, doselfld_ );
    lbl_ = new uiLabel( this, "Session to use" );
    lbl_->attach( centeredLeftOf, selgrp_ );

    loadnowfld_ = new uiGenInput( this, "Load selected session now",
	    			  BoolInpSpec(true) );
    loadnowfld_->attach( alignedBelow, selgrp_ );

    postFinalise().notify( mCB(this,uiODMainAutoSessionDlg,useChg) );
}

~uiODMainAutoSessionDlg()
{
    delete ctio_.ioobj; delete &ctio_;
}

void useChg( CallBacker* )
{
    const bool douse = usefld_->getBoolValue();
    const bool dosel = douse ? doselfld_->getBoolValue() : false;
    doselfld_->display( douse );
    selgrp_->display( dosel );
    loadnowfld_->display( dosel );
    lbl_->display( dosel );
}


bool acceptOK( CallBacker* )
{
    const bool douse = usefld_->getBoolValue();
    const bool dosel = douse ? doselfld_->getBoolValue() : false;
    if ( !dosel )
	ctio_.setObj( 0 );
    else
    {
	selgrp_->processInput();
	if ( selgrp_->nrSel() < 1 )
	    { uiMSG().error("Please select a session"); return false; }
	if ( selgrp_->nrSel() > 0 )
	    ctio_.setObj( selgrp_->selected(0) );
    }

    const MultiID id( ctio_.ioobj ? ctio_.ioobj->key() : MultiID("") );
    ODSession::setStartupData( douse, id );

    return true;
}

    CtxtIOObj&		ctio_;

    uiGenInput*		usefld_;
    uiGenInput*		doselfld_;
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
	    handleStartupSession();
    }
}


void uiODMain::restoreSession( const IOObj* ioobj )
{
    ODSession sess; BufferString bs;
    if ( !ODSessionTranslator::retrieve(sess,ioobj,bs) )
	{ uiMSG().error( bs ); return; }

    cursession_ = &sess;
    doRestoreSession();
    cursession_ = &lastsession_; lastsession_.clear();
    //ctabed_->updateColTabList();
    timer_.start( 200, true );
    sceneMgr().setToViewMode( true );
    sceneMgr().updateTrees();
}


bool uiODMain::updateSession()
{
    cursession_->clear();
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
	applMgr().attrServer()->usePar( cursession_->attrpars(false,false),						false, false );
	applMgr().attrServer()->usePar( cursession_->attrpars(false,true),						false, true );
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
	uiMSG().error( "An error occurred while reading session file.\n"
		       "A new scene will be launched" );	
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
    bool douse = false; MultiID id;
    ODSession::getStartupData( douse, id );
    if ( !douse || id == "" ) 
	return;

    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj ) return;
    cursessid_ = id;
    restoreSession( ioobj );
}


void uiODMain::timerCB( CallBacker* )
{
    sceneMgr().layoutScenes();
}


void uiODMain::memTimerCB( CallBacker* )
{
    static bool multiple_access = false;
    if ( multiple_access ) return;
    multiple_access = true;

    float tot, av;
    OD::getSystemMemory( tot, av );

    BufferString txt( "[mem] " );
    const bool ingb = tot > 1070000000;
    const float fac = ingb ? 1073741824 : 1048576;
    tot /= fac; av /=fac;
    int itot = mNINT32(tot*10); int iav = mNINT32(av*10);
    txt			.add( iav/10 ).add( "." ).add( iav%10 )
	.add( "/" )	.add( itot/10 ).add( "." ).add( itot%10 )
	.add( ingb ? "G" : "M" );
    statusBar()->message( txt, mMemStatusFld );

    multiple_access = false;
}


bool uiODMain::go()
{
    if ( failed_ ) return false;

    show();
    Timer tm( "Handle startup session" );
    tm.tick.notify( mCB(this,uiODMain,afterSurveyChgCB) );
    tm.start( 200, true );
    int rv = uiapp_.exec();
    delete applmgr_; applmgr_ = 0;
    return rv ? false : true;
}


bool uiODMain::askStore( bool& askedanything, const char* actiontype )
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
	int res = uiMSG().askSave( "Do you want to save this session?", true );
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
	int res = uiMSG().askSave( "Current attribute set has changed.\n"
				   "Store the changes now?" );
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
    BufferString capt( "OpendTect V", GetFullODVersion() );
    capt.add( "/" ).add( OD::Platform::local().shortName() );

    if ( ODInst::getAutoInstType() == ODInst::InformOnly
	&& ODInst::updatesAvailable() )
	capt.add( " *UPDATE AVAILABLE*" );

    const char* usr = GetSoftwareUser();
    if ( usr && *usr )
	capt.add( " [" ).add( usr ).add( "]" );

    if ( !SI().name().isEmpty() )
	capt.add( ": " ).add( SI().name() );

    setCaption( capt );
}


bool uiODMain::closeOK()
{
    saveSettings();

    bool askedanything = false;
    if ( !askStore(askedanything,"Shutdown") )
    {
	uiMSG().message("Shutdown cancelled");
	return false;
    }

    IOM().applClosing();

    if ( failed_ ) return true;

    if ( !askedanything )
    {
	bool doask = false;
	Settings::common().getYN( "dTect.Ask close", doask );
	if ( doask && !uiMSG().askGoOn( "Do you want to quit?" ) )
	    return false;
    }

    removeDockWindow( ctabwin_ );
    delete scenemgr_;
    delete menumgr_;

    scenemgr_ = 0;
    menumgr_ = 0;

    return true;
}


void uiODMain::exit()
{
    if ( !closeOK() ) return;

    uiapp_.exit(0);
}
