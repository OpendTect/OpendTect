/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uiodmain.cc,v 1.16 2004-04-13 08:04:34 nanne Exp $
________________________________________________________________________

-*/

#include "uiodmain.h"
#include "uicmain.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiodmenumgr.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"
#include "uinlapartserv.h"
#include "uiattribpartserv.h"
#include "uidockwin.h"
#include "uisurvey.h"
#include "uiioobjsel.h"
#include "uisetdatadir.h"
#include "uimsg.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "ctxtioobj.h"
#include "filegen.h"
#include "settings.h"
#include "plugins.h"
#include "odsessionfact.h"

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
    manODMainWin( odmain );
    PIM().loadAuto( true );
    if ( !odmain->ensureGoodSurveySetup() )
	return false;

    odmain->initScene();
    odmain->go();
    delete odmain;
    return 0;
}



uiODMain::uiODMain( uicMain& a )
	: uiMainWin(0,"OpendTect Main Window",3,true,true)
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
{
    uiMSG().setMainWin( this );
    uiapp.setTopLevel( this );

    if ( !ensureGoodDataDir() )
	::exit( 0 );

    applmgr = new uiODApplMgr( *this );

    if ( buildUI() )
	failed = false;
}


uiODMain::~uiODMain()
{
    delete ctabed;
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
	cerr << errmsg << endl;
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


bool uiODMain::buildUI()
{
    scenemgr = new uiODSceneMgr( this );
    menumgr = new uiODMenuMgr( this );
    menumgr->initSceneMgrDepObjs();

    ctabwin = new uiDockWin( this, "Color Table" );
    moveDockWindow( *ctabwin, uiMainWin::Left, 0 );
    ctabwin->setResizeEnabled( true );
							    
    ctabed = new uiVisColTabEd( ctabwin );
    ctabed->setPrefHeight( cCTHeight );
    ctabed->attach(hCentered);

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


bool uiODMain::hasSessionChanged()
{
    ODSession sess;
    cursession = &sess;
    updateSession();
    cursession = &lastsession;
    return !( sess == lastsession );
}


void uiODMain::saveSession()
{
    PtrMan<CtxtIOObj> ctio = getUserSessionIOData( false );
    if ( !ctio ) return;
    ODSession sess; cursession = &sess;
    if ( !updateSession() ) return;
    BufferString bs;
    if ( !ODSessionTranslator::store(sess,ctio->ioobj,bs) )
	{ uiMSG().error( bs ); return; }

    lastsession = sess; cursession = &lastsession;
}


void uiODMain::restoreSession()
{
    PtrMan<CtxtIOObj> ctio = getUserSessionIOData( true );
    if ( !ctio ) return;
    ODSession sess; BufferString bs;
    if ( !ODSessionTranslator::retrieve(sess,ctio->ioobj,bs) )
	{ uiMSG().error( bs ); return; }

    cursession = &sess;
    doRestoreSession();
    cursession = &lastsession; lastsession.clear();
}


bool uiODMain::updateSession()
{
    cursession->clear();
    applMgr().visServer()->fillPar( cursession->vispars() );
    applMgr().attrServer()->fillPar( cursession->attrpars() );
    sceneMgr().getScenePars( cursession->scenepars() );
    if ( applMgr().nlaServer()
      && !applMgr().nlaServer()->fillPar( cursession->nlapars() ) ) 
	return false;

    sessionSave.trigger();
    return true;
}


void uiODMain::doRestoreSession()
{
    sceneMgr().cleanUp( false );
    applMgr().resetServers();

    if ( applMgr().nlaServer() )
	applMgr().nlaServer()->usePar( cursession->nlapars() );
    applMgr().attrServer()->usePar( cursession->attrpars() );
    bool visok = applMgr().visServer()->usePar( cursession->vispars() );

    if ( visok ) 
	sceneMgr().useScenePars( cursession->scenepars() );
    else
    {
	uiMSG().error( "An error occurred while reading session file.\n"
		       "A new scene will be launched" );	
	sceneMgr().cleanUp( true );
    }
    sessionRestore.trigger();
}


bool uiODMain::go()
{
    if ( failed ) return false;

    show();
    uiSurvey::updateViewsGlobal();
    int rv = uiapp.exec();
    delete applmgr; applmgr = 0;
    return rv ? false : true;
}


bool uiODMain::closeOK()
{
    if ( failed ) return true;

    menumgr->storePositions();
    scenemgr->storePositions();
    ctabwin->storePosition();

    int res = hasSessionChanged()
	    ? uiMSG().askGoOnAfter( "Do you want to save this session?" )
            : (int)!uiMSG().askGoOn( "Do you want to quit?" ) + 1;

    if ( res == 0 )
	saveSession();
    else if ( res == 2 )
	return false;

    return true;
}


void uiODMain::exit()
{
    if ( !closeOK() ) return;

    uiapp.exit(0);
}
